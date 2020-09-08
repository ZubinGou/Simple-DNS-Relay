// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <WinSock2.h>
#include <windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define BUF_SIZE 1500
#define ID_TABLE_SIZE 128
#define ID_EXPIRE_TIME 4 // 5s
#define SELECT_MODE 1
#define NONBLOCK_MODE 2
#define MODE 2

char PUBLIC_DNS_IP[16] = "10.3.9.4";
char DNS_TABLE_FILE[100] = "./dnsrelay.txt";
int PORT = 53;
bool DEBUG = true;
bool LOG = true;
int requstCount = 0;


int addr_len = sizeof(struct sockaddr_in);

SOCKET clientSock;
SOCKET serverSock;
struct sockaddr_in clientAddr;
struct sockaddr_in serverAddr;

/*
* Masks and constants.
*/

static const uint32_t QR_MASK = 0x8000;
static const uint32_t OPCODE_MASK = 0x7800;
static const uint32_t AA_MASK = 0x0400;
static const uint32_t TC_MASK = 0x0200;
static const uint32_t RD_MASK = 0x0100;
static const uint32_t RA_MASK = 0x8000;
static const uint32_t RCODE_MASK = 0x000F;

/* Response Type */
enum
{
    Ok_ResponseType = 0,
    FormatError_ResponseType = 1,
    ServerFailure_ResponseType = 2,
    NameError_ResponseType = 3,
    NotImplemented_ResponseType = 4,
    Refused_ResponseType = 5
};

/* Resource Record Types */
enum
{
    A_Resource_RecordType = 1,
    NS_Resource_RecordType = 2,
    CNAME_Resource_RecordType = 5,
    SOA_Resource_RecordType = 6,
    PTR_Resource_RecordType = 12,
    MX_Resource_RecordType = 15,
    TXT_Resource_RecordType = 16,
    AAAA_Resource_RecordType = 28,
    SRV_Resource_RecordType = 33
};

/* Operation Code */
enum
{
    QUERY_OperationCode = 0,  /* standard query */
    IQUERY_OperationCode = 1, /* inverse query */
    STATUS_OperationCode = 2, /* server status request */
    NOTIFY_OperationCode = 4, /* request zone transfer */
    UPDATE_OperationCode = 5  /* change resource records */
};

/* Response Code */
enum
{
    NoError_ResponseCode = 0,
    FormatError_ResponseCode = 1,
    ServerFailure_ResponseCode = 2,
    NameError_ResponseCode = 3
};

/* Query Type */
enum
{
    IXFR_QueryType = 251,
    AXFR_QueryType = 252,
    MAILB_QueryType = 253,
    MAILA_QueryType = 254,
    STAR_QueryType = 255
};

/*
* Types.
*/

/* Question Section */
struct Question
{
    char *qName;
    uint16_t qType;
    uint16_t qClass;
    struct Question *next; // for linked list
};

/* Data part of a Resource Record */
union ResourceData
{
    struct
    {
        uint8_t txt_data_len;
        char *txt_data;
    } txt_record;
    struct
    {
        uint8_t addr[4];
    } a_record;
    struct
    {
        char *MName;
        char *RName;
        uint32_t serial;
        uint32_t refresh;
        uint32_t retry;
        uint32_t expire;
        uint32_t minimum;
    } soa_record;
    struct
    {
        char *name;
    } name_server_record;
    struct
    {
        char *name;
    } cname_record;
    struct
    {
        char *name;
    } ptr_record;
    struct
    {
        uint16_t preference;
        char *exchange;
    } mx_record;
    struct
    {
        uint8_t addr[16];
    } aaaa_record;
    struct
    {
        uint16_t priority;
        uint16_t weight;
        uint16_t port;
        char *target;
    } srv_record;
};

// Resource Record Section
// Answer、Authority、Additional部分
struct ResourceRecord
{
    char *name;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rd_length;
    union ResourceData rd_data;
    struct ResourceRecord *next; // for linked list
};

struct Message
{
    uint16_t id; /* Identifier */

    /* Flags */
    uint16_t qr;     /* Query/Response Flag */
    uint16_t opcode; /* Operation Code */
    uint16_t aa;     /* Authoritative Answer Flag */
    uint16_t tc;     /* Truncation Flag */
    uint16_t rd;     /* Recursion Desired */
    uint16_t ra;     /* Recursion Available */
    uint16_t rcode;  /* Response Code */

    uint16_t qdCount; /* Question Count */
    uint16_t anCount; /* Answer Record Count */
    uint16_t nsCount; /* Authority Record Count */
    uint16_t arCount; /* Additional Record Count */

    /* At least one question; questions are copied to the response 1:1 */
    struct Question *questions;

    /*
  * Resource records to be send back.
  * Every resource record can be in any of the following places.
  * But every place has a different semantic.
  */
    struct ResourceRecord *answers;
    struct ResourceRecord *authorities;
    struct ResourceRecord *additionals;
};

typedef struct {
    uint16_t clientId;
    int expireTime; // 传输完也将expireTime设为0
    struct sockaddr_in clientAddr;
} IdConversion;

IdConversion IdTable[ID_TABLE_SIZE]; // ID转换表