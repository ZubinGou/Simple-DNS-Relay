#include "main.h"

/*
* This software is licensed under the CC0.
*
* This is a _basic_ DNS Server for educational use.
* It does not prevent invalid packets from crashing
* the server.
*
* To test start the program and issue a DNS request:
*  dig @127.0.0.1 -p 53 foo.bar.com 
*/

// TODO 中继、查表
// domain_name => addr

bool findInCache(uint8_t addr[4], const char domain_name[])
{
    return false;
}

bool findInTable(uint8_t addr[4], const char domain_name[])
{
    return false;
}

void addToCache(uint8_t addr[4], const char domain_name[])
{
}

int get_A_Record(uint8_t addr[4], const char domain_name[])
{
    if (findInCache(addr, domain_name))
    { // 在缓存中找到，则使用缓存中的ip地址
        return 0;
    }
    else if (findInTable(addr, domain_name))
    { // 在对照表中找到，则使用对照表中的ip地址
        return 0;
    }
    return -1;
}

int get_AAAA_Record(uint8_t addr[16], const char domain_name[])
{
    if (strcmp("foo.bar.com", domain_name) == 0)
    {
        addr[0] = 0xfe;
        addr[1] = 0x80;
        addr[2] = 0x00;
        addr[3] = 0x00;
        addr[4] = 0x00;
        addr[5] = 0x00;
        addr[6] = 0x00;
        addr[7] = 0x00;
        addr[8] = 0x00;
        addr[9] = 0x00;
        addr[10] = 0x00;
        addr[11] = 0x00;
        addr[12] = 0x00;
        addr[13] = 0x00;
        addr[14] = 0x00;
        addr[15] = 0x01;
        return 0;
    }
    else
    {
        return -1;
    }
}

int get_TXT_Record(char **addr, const char domain_name[])
{
    if (strcmp("foo.bar.com", domain_name) == 0)
    {
        *addr = "abcdefg";
        return 0;
    }
    else
    {
        return -1;
    }
}

/*
* Debugging functions.
*/

void print_hex(const uint8_t *buf, size_t len)
{
    int i;
    printf("%zu bytes:\n", len);
    for (i = 0; i < len; ++i)
        printf("%02x ", buf[i]);
    printf("\n");
}

void print_resource_record(struct ResourceRecord *rr)
{
    int i;
    while (rr)
    {
        printf("  ResourceRecord { name '%s', type %u, class %u, ttl %u, rd_length %u, ",
               rr->name,
               rr->type,
               rr->class,
               rr->ttl,
               rr->rd_length);

        union ResourceData *rd = &rr->rd_data;
        switch (rr->type)
        {
        case A_Resource_RecordType:
            printf("Address Resource Record { address ");

            for (i = 0; i < 4; ++i)
                printf("%s%u", (i ? "." : ""), rd->a_record.addr[i]);

            printf(" }");
            break;
        case NS_Resource_RecordType:
            printf("Name Server Resource Record { name %s }",
                   rd->name_server_record.name);
            break;
        case CNAME_Resource_RecordType:
            printf("Canonical Name Resource Record { name %u }",
                   rd->cname_record.name);
            break;
        case SOA_Resource_RecordType:
            printf("SOA { MName '%s', RName '%s', serial %u, refresh %u, retry %u, expire %u, minimum %u }",
                   rd->soa_record.MName,
                   rd->soa_record.RName,
                   rd->soa_record.serial,
                   rd->soa_record.refresh,
                   rd->soa_record.retry,
                   rd->soa_record.expire,
                   rd->soa_record.minimum);
            break;
        case PTR_Resource_RecordType:
            printf("Pointer Resource Record { name '%s' }",
                   rd->ptr_record.name);
            break;
        case MX_Resource_RecordType:
            printf("Mail Exchange Record { preference %u, exchange '%s' }",
                   rd->mx_record.preference,
                   rd->mx_record.exchange);
            break;
        case TXT_Resource_RecordType:
            printf("Text Resource Record { txt_data '%s' }",
                   rd->txt_record.txt_data);
            break;
        case AAAA_Resource_RecordType:
            printf("AAAA Resource Record { address ");

            for (i = 0; i < 16; ++i)
                printf("%s%02x", (i ? ":" : ""), rd->aaaa_record.addr[i]);

            printf(" }");
            break;
        default:
            printf("Unknown Resource Record { ??? }");
        }
        printf("}\n");
        rr = rr->next;
    }
}

void print_query(struct Message *msg)
{
    struct Question *q;

    printf("QUERY { ID: %02x", msg->id);
    printf(". FIELDS: [ QR: %u, OpCode: %u ]", msg->qr, msg->opcode);
    printf(", QDcount: %u", msg->qdCount);
    printf(", ANcount: %u", msg->anCount);
    printf(", NScount: %u", msg->nsCount);
    printf(", ARcount: %u,\n", msg->arCount);

    q = msg->questions;
    while (q)
    {
        printf("  Question { qName '%s', qType %u, qClass %u }\n",
               q->qName,
               q->qType,
               q->qClass);
        q = q->next;
    }

    print_resource_record(msg->answers);
    print_resource_record(msg->authorities);
    print_resource_record(msg->additionals);

    printf("}\n");
}

/*
* Basic memory operations.
*/
size_t get8bits(const uint8_t **buffer)
{
    uint8_t value;
    memcpy(&value, *buffer, 1);
    *buffer += 1;
    return value;
}

size_t get16bits(const uint8_t **buffer)
{
    uint16_t value;

    memcpy(&value, *buffer, 2);
    *buffer += 2;

    return ntohs(value);
}

size_t get32bits(const uint8_t **buffer)
{
    uint32_t value;

    memcpy(&value, *buffer, 4);
    *buffer += 4;

    return ntohl(value);
}

void put8bits(uint8_t **buffer, uint8_t value)
{
    memcpy(*buffer, &value, 1);
    *buffer += 1;
}

void put16bits(uint8_t **buffer, uint16_t value)
{
    value = htons(value);
    memcpy(*buffer, &value, 2);
    *buffer += 2;
}

void put32bits(uint8_t **buffer, uint32_t value)
{
    value = htonl(value);
    memcpy(*buffer, &value, 4);
    *buffer += 4;
}

/*
* Deconding/Encoding functions.
*/

// 3foo3bar3com0 => foo.bar.com
// char *decode_domain_name(const uint8_t **buffer)
// {
//     char name[256];
//     const uint8_t *buf = *buffer;
//     int j = 0;
//     int i = 0;

//     while (buf[i] != 0)
//     {
//         if (i != 0)
//         {
//             name[j] = '.';
//             j += 1;
//         }
//         if (buf[i] == 0xc0) {
//             decode_domain_name()
//         }
//         else {
//             int len = buf[i]; // 字符长度
//             i += 1;

//             memcpy(name + j, buf + i, len);
//             i += len;
//             j += len;
//         }
//     }

//     name[j] = '\0';
//     *buffer += i + 1; //also jump over the last 0
//     return strdup(name);
// }

char *decode_domain_name(const uint8_t **oribuffer, int offset)
{
    char name[256];
    const uint8_t *buf = *oribuffer + offset; // 域名开始
    int j = 0;
    int i = 0;

    const uint8_t **base = oribuffer;
    while (buf[i] != 0)
    {
        if (i != 0)
        {
            name[j] = '.';
            j += 1;
        }
        if (buf[i] == 0xc0) {
            i++;
            char * tempName = decode_domain_name(base, buf[i]);
            i++;
            for (int k = 0; tempName[k] != '\0'; k++) {
                name[j] = tempName[k];
                j++;
            }
            // memcpy(name + j, tempName, sizeof(tempName) + 1);
            printf("j=%d, tempName = %s, size=%d\n", j, tempName, sizeof(tempName));
            // j += sizeof(tempName) + 1;
            printf("then Name = %s\n", name);
        }
        else {
            int len = buf[i]; // 字符长度
            i += 1;

            memcpy(name + j, buf + i, len);
            i += len;
            j += len;
        }
    }

    name[j] = '\0';
    *oribuffer += i + 1; //also jump over the last 0
    printf("get decode name ==== %s\n", name);
    return strdup(name);
}


// foo.bar.com => 3foo3bar3com0
void encode_domain_name(uint8_t **buffer, const char *domain)
{
    uint8_t *buf = *buffer;
    const char *beg = domain;
    const char *pos;
    int len = 0;
    int i = 0;

    while ((pos = strchr(beg, '.')))
    {
        len = pos - beg;
        buf[i] = len;
        i += 1;
        memcpy(buf + i, beg, len);
        i += len;

        beg = pos + 1;
    }

    len = strlen(domain) - (beg - domain);
    buf[i] = len;
    i += 1;
    memcpy(buf + i, beg, len);
    i += len;

    buf[i] = 0;
    i += 1;

    *buffer += i;
}

void decode_header(struct Message *msg, const uint8_t **buffer)
{
    msg->id = get16bits(buffer);
    printf("get msg->id = %d\n", msg->id);

    uint32_t fields = get16bits(buffer);
    msg->qr = (fields & QR_MASK) >> 15;
    msg->opcode = (fields & OPCODE_MASK) >> 11;
    msg->aa = (fields & AA_MASK) >> 10;
    msg->tc = (fields & TC_MASK) >> 9;
    msg->rd = (fields & RD_MASK) >> 8;
    msg->ra = (fields & RA_MASK) >> 7;
    msg->rcode = (fields & RCODE_MASK) >> 0;

    msg->qdCount = get16bits(buffer);
    msg->anCount = get16bits(buffer);
    msg->nsCount = get16bits(buffer);
    msg->arCount = get16bits(buffer);
}

void encode_header(struct Message *msg, uint8_t **buffer)
{
    put16bits(buffer, msg->id);

    int fields = 0;
    fields |= (msg->qr << 15) & QR_MASK;
    fields |= (msg->rcode << 0) & RCODE_MASK;
    // TODO: insert the rest of the fields
    put16bits(buffer, fields);

    put16bits(buffer, msg->qdCount);
    put16bits(buffer, msg->anCount);
    put16bits(buffer, msg->nsCount);
    put16bits(buffer, msg->arCount);
}

int decode_resource_records(struct ResourceRecord *rr, const uint8_t **buffer, const uint8_t *oriBuffer)
{
    printf("\ndecode_resource_records\n");

    const uint8_t *baseAddr = oriBuffer;
    printf("*buffer: %p\n", *buffer);
    printf("*Oribuffer: %p\n", oriBuffer);

    print_hex(*buffer, 20);
    rr->name = decode_domain_name(&oriBuffer, *buffer - oriBuffer);

    // if ((unsigned char)**buffer == 0xc0) /* The name field is pointer */ {
    //     *buffer += 1;
    //     // rr->name = "TODO";

    //     // printf("oriBuffer = %p\n",  oriBuffer);
    //     // print_hex(oriBuffer, 20);

    //     rr->name = decode_domain_name(&oriBuffer, **buffer); // 偏移量
    //     printf("decode_resource_records: rr->name = %s\n", rr->name);
    //     *buffer += 1;
    // }
    // else /* The name field is Url */
    // {
    //     // while (*(*buffer) > 0)
    //     //     *buffer += (*(*buffer)) + 1;
    //     // ++(*buffer);

    //     print_hex(*buffer, 20);

    //     printf("offset: %d", (int)(*buffer - *oriBuffer));
    //     rr->name = decode_domain_name(&oriBuffer, *buffer - *oriBuffer);
    //     // rr->name = "hhh";
    //     printf("decode_resource_records: rr->name = %s\n", rr->name);
    // }
    rr->type = get16bits(buffer);
    rr->class = get16bits(buffer);
    rr->ttl = get32bits(buffer);
    rr->rd_length = get16bits(buffer);

    switch (rr->type)
    {
    case A_Resource_RecordType:
        for (int i = 0; i < 4; ++i)
            rr->rd_data.a_record.addr[i] = get8bits(buffer);
        break;
    case AAAA_Resource_RecordType:
        for (int i = 0; i < 16; ++i)
            rr->rd_data.aaaa_record.addr[i] = get8bits(buffer);
        break;
    case TXT_Resource_RecordType:
        rr->rd_data.txt_record.txt_data_len = get8bits(buffer);
        for (int i = 0; i < rr->rd_data.txt_record.txt_data_len; i++)
            rr->rd_data.txt_record.txt_data[i] = get8bits(buffer);
        break;
    default:
        fprintf(stderr, "Unknown type %u. => Ignore resource record.\n", rr->type);
        return 1;
    }
    return 0;
}

int decode_msg(struct Message *msg, const uint8_t *buffer, int size)
{
    const uint8_t *oriBuffer = buffer;

    decode_header(msg, &buffer);
    // if (msg->anCount != 0 || msg->nsCount != 0)
    // {
    //     printf("Only questions expected!\n");
    //     return -1;
    // }

    // 解析Question
    for (uint16_t i = 0; i < msg->qdCount; ++i)
    {
        struct Question *q = malloc(sizeof(struct Question));

        q->qName = decode_domain_name(&buffer, 0);
        q->qType = get16bits(&buffer);
        q->qClass = get16bits(&buffer);

        // 添加到链表前端
        q->next = msg->questions;
        msg->questions = q;
    }

    // 解析Answer
    for (uint16_t i = 0; i < msg->anCount; ++i)
    {
        struct ResourceRecord *rr = malloc(sizeof(struct ResourceRecord));
        decode_resource_records(rr, &buffer, oriBuffer);
        // 添加到链表前端
        rr->next = msg->answers;
        msg->answers = rr;
    }

    // // 解析Authority
    // for (uint16_t i = 0; i < msg->nsCount; ++i)
    // {
    //     struct ResourceRecord *rr = malloc(sizeof(struct ResourceRecord));
    //     decode_resource_records(rr, buffer);
    //     // 添加到链表前端
    //     rr->next = msg->authorities;
    //     msg->authorities = rr;
    // }

    // // 解析Additional
    // for (uint16_t i = 0; i < msg->arCount; ++i)
    // {
    //     struct ResourceRecord *rr = malloc(sizeof(struct ResourceRecord));
    //     decode_resource_records(rr, buffer);
    //     // 添加到链表前端
    //     rr->next = msg->additionals;
    //     msg->additionals = rr;
    // }

    return 0;
}

// For every question in the message add a appropiate resource record
// in either section 'answers', 'authorities' or 'additionals'.
int resolver_process(struct Message *msg)
{
    // struct ResourceRecord *beg;
    struct ResourceRecord *rr;
    struct Question *q;
    int rc;

    // leave most values intact for response
    msg->qr = 1; // this is a response
    msg->aa = 1; // this server is authoritative
    msg->ra = 0; // no recursion available
    msg->rcode = Ok_ResponseType;

    // should already be 0
    msg->anCount = 0;
    msg->nsCount = 0;
    msg->arCount = 0;

    // for every question append resource records
    q = msg->questions;
    while (q)
    {
        rr = malloc(sizeof(struct ResourceRecord));
        memset(rr, 0, sizeof(struct ResourceRecord));

        rr->name = strdup(q->qName);
        rr->type = q->qType;
        rr->class = q->qClass;
        rr->ttl = 60 * 60; // in seconds; 0 means no caching

        printf("Query for '%s'\n", q->qName);

        // We only can only answer two question types so far
        // and the answer (resource records) will be all put
        // into the answers list.
        // This behavior is probably non-standard!
        switch (q->qType)
        {
        case A_Resource_RecordType:
            rr->rd_length = 4;
            rc = get_A_Record(rr->rd_data.a_record.addr, q->qName);
            break;
            // case AAAA_Resource_RecordType:
            //     rr->rd_length = 16;
            //     rc = get_AAAA_Record(rr->rd_data.aaaa_record.addr, q->qName);
            //     break;
            // case TXT_Resource_RecordType:
            //     rc = get_TXT_Record(&(rr->rd_data.txt_record.txt_data), q->qName);
            //     int txt_data_len = strlen(rr->rd_data.txt_record.txt_data);
            //     rr->rd_length = txt_data_len + 1;
            //     rr->rd_data.txt_record.txt_data_len = txt_data_len;
            //     break;
            //
            // case NS_Resource_RecordType:
            // case CNAME_Resource_RecordType:
            // case SOA_Resource_RecordType:
            // case PTR_Resource_RecordType:
            // case MX_Resource_RecordType:
            // case TXT_Resource_RecordType:

        default:
            rc = -1;
            msg->rcode = NotImplemented_ResponseType;
            printf("Cannot answer question of type %d.\n", q->qType);
        }

        if (rc == 0)
        { // ?
            msg->anCount++;
            rr->next = msg->answers;
            msg->answers = rr;
        }
        else
        {
            free(rr->name);
            free(rr);
            return -1;
        }
        q = q->next;
    }
    return 0;
}

int encode_resource_records(struct ResourceRecord *rr, uint8_t **buffer)
{
    int i;
    while (rr)
    {
        // Answer questions by attaching resource sections.
        encode_domain_name(buffer, rr->name);
        put16bits(buffer, rr->type);
        put16bits(buffer, rr->class);
        put32bits(buffer, rr->ttl);
        put16bits(buffer, rr->rd_length);

        switch (rr->type)
        {
        case A_Resource_RecordType:
            for (i = 0; i < 4; ++i)
                put8bits(buffer, rr->rd_data.a_record.addr[i]);
            break;
        case AAAA_Resource_RecordType:
            for (i = 0; i < 16; ++i)
                put8bits(buffer, rr->rd_data.aaaa_record.addr[i]);
            break;
        case TXT_Resource_RecordType:
            put8bits(buffer, rr->rd_data.txt_record.txt_data_len);
            for (i = 0; i < rr->rd_data.txt_record.txt_data_len; i++)
                put8bits(buffer, rr->rd_data.txt_record.txt_data[i]);
            break;
        default:
            fprintf(stderr, "Unknown type %u. => Ignore resource record.\n", rr->type);
            return 1;
        }

        rr = rr->next;
    }
    return 0;
}

/* @return 0 upon failure, 1 upon success */
int encode_msg(struct Message *msg, uint8_t **buffer)
{
    struct Question *q;
    int rc;

    encode_header(msg, buffer);

    q = msg->questions;
    while (q) // 解析所有的question
    {
        encode_domain_name(buffer, q->qName);
        put16bits(buffer, q->qType);
        put16bits(buffer, q->qClass);

        q = q->next;
    }

    rc = 0;
    rc |= encode_resource_records(msg->answers, buffer);
    rc |= encode_resource_records(msg->authorities, buffer);
    rc |= encode_resource_records(msg->additionals, buffer);

    return rc;
}

/* ID转换部分 */
bool isExpired(IdConversion idc)
{
    return idc.expireTime < time(NULL);
}

uint16_t newId(uint16_t clientId, struct sockaddr_in clientAddr)
{
    uint16_t i;
    for (i = 0; i < ID_TABLE_SIZE; ++i)
    {
        if (isExpired(IdTable[i]))
        {
            IdTable[i].clientId = clientId;
            IdTable[i].clientAddr = clientAddr;
            IdTable[i].expireTime = ID_EXPIRE_TIME + time(NULL);
        }
        break;
    }
    printf("%d => %d", clientId, i);
    return i;
}

/* 内存释放部分 */
void free_resource_records(struct ResourceRecord *rr)
{
    struct ResourceRecord *next;
    while (rr)
    {
    printf("free_resource_records\n");
        free(rr->name);
        next = rr->next;
        free(rr);
        rr = next;
    }
}

// 释放questions内存
void free_questions(struct Question *qq)
{
    struct Question *next;

    while (qq)
    {
        free(qq->qName);
        next = qq->next;
        free(qq);
        qq = next;
    }
}

// void free_msg(struct Message msg)
// {
//     // 释放Question
//     for (uint16_t i = 0; i < msgqdCount; ++i)
//     {
//         struct Question *q = malloc(sizeof(struct Question));

//         q->qName = decode_domain_name(&buffer);
//         q->qType = get16bits(&buffer);
//         q->qClass = get16bits(&buffer);

//         // 添加到链表前端
//         q->next = msg->questions;
//         msg->questions = q;
//     }

//     // 解析Answer
//     for (uint16_t i = 0; i < msg->anCount; ++i)
//     {
//         struct ResourceRecord *rr = malloc(sizeof(struct ResourceRecord));
//         decode_resource_records(rr, &buffer);
//         // 添加到链表前端
//         rr->next = msg->answers;
//         msg->answers = rr;
//     }
// }


void receiveFromLocal()
{ // 客户端请求：
    int nbytes = -1;
    uint8_t buffer[BUF_SIZE]; // ? char
    struct Message msg;
    memset(&msg, 0, sizeof(struct Message));

    nbytes = recvfrom(clientSock, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addr_len);

    if (nbytes < 0 || decode_msg(&msg, buffer, nbytes) != 0)
    {
        return;
    }
    uint16_t clientId = msg.id;
    /* Print query */
    print_query(&msg);

    if (resolver_process(&msg) == 0)
    { // 可以在本地找到所有question的answer，则将添加answer的msg编码后发送给客户端
        uint8_t *bufferBegin = buffer;
        if (encode_msg(&msg, &bufferBegin) != 0)
        {
            return;
        }
        int buflen = bufferBegin - buffer;
        sendto(clientSock, buffer, buflen, 0, (struct sockaddr *)&clientAddr, addr_len);
    }
    else
    { // 否则，将请求修改ID后转发
        uint16_t nId = newId(clientId, clientAddr);
        if (nId == ID_TABLE_SIZE) {
            printf("ID Table if Full\n");
        }
        else {
            memcpy(buffer, &nId, sizeof(uint16_t));
            nbytes = sendto(serverSock, buffer, nbytes, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        }
    }
    if (msg.qdCount)
        free_questions(msg.questions);
    if (msg.anCount)
        free_resource_records(msg.answers);
    // free_resource_records(msg.authorities);
    // free_resource_records(msg.additionals);
}

void receiveFromPublic()
{ // 接收 public dns 的 response，映射ID后转发给用户，并存储url-ip到cache
    int nbytes = -1;
    uint8_t buffer[BUF_SIZE]; // ? char
    struct Message msg;
    memset(&msg, 0, sizeof(struct Message));

    // public dns
    nbytes = recvfrom(serverSock, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddr, &addr_len);

    if (nbytes < 0 || decode_msg(&msg, buffer, nbytes) != 0)
    {
        return;
    }

    print_query(&msg);

    // ID映射
    uint16_t nId = msg.id;
    // put16bits(&buffer, IdTable[nId].clientId);
    IdTable[nId].clientId = htons(IdTable[nId].clientId);
    memcpy(buffer, &IdTable[nId].clientId, sizeof(uint16_t));

    // printf("nId = %d, clientId = %d\n", nId, IdTable[nId].clientId);
    struct sockaddr_in ca = IdTable[nId].clientAddr;

    IdTable[nId].expireTime = 0; // 响应完毕后立即过期
    sendto(clientSock, buffer, nbytes, 0, (struct sockaddr *)&ca, sizeof(ca));

    // 存储到cache
    const char *domain_name = msg.answers->name;
    uint8_t *addr = msg.answers->rd_data.a_record.addr;
    addToCache(addr, domain_name);
    if (msg.qdCount)
        free_questions(msg.questions);
    printf("hr1\n");
    printf("%d\n", msg.anCount);
    if (msg.anCount)
        free_resource_records(msg.answers);
    printf("hr2\n");
    // free_resource_records(msg.authorities);
    // free_resource_records(msg.additionals);
}

int main()
{
    // 初始化ID转换表
    for (int i = 0; i < ID_TABLE_SIZE; i++) {
        IdTable[i].clientId = 0;
        IdTable[i].expireTime = 0;
        memset(&(IdTable[i].clientAddr), 0, sizeof(struct sockaddr_in));
    }

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // buffer for input/output binary packet
    clientSock = socket(AF_INET, SOCK_DGRAM, 0);
    serverSock = socket(AF_INET, SOCK_DGRAM, 0);

    // TODO 非阻塞
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(PORT);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(PUBLIC_DNS_IP); // ?
    // InetPton(AF_INET, _T(PUBLIC_DNS_IP), &serverAddr.sin_addr.s_addr);
    serverAddr.sin_port = htons(PORT);

    // 防止PORT被占用
    const int REUSE = 1;
    setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&REUSE, sizeof(REUSE));

    if (bind(clientSock, (SOCKADDR *)&clientAddr, addr_len) < 0)
    {
        printf("Could not bind: %s\n", strerror(errno));
        return 1;
    }

    printf("Listening on port %u.\n", PORT);

    while (1)
    {
        printf("\n1 receiveFromLocal\n");
        receiveFromLocal();
        printf("\n2 receiveFromPublic\n");
        receiveFromPublic();
        // Sleep(1);
    }

    closesocket(clientSock);
    WSACleanup();
    return 0;
}
