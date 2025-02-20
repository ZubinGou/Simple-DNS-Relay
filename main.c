//============================================================================
// @Name : main.cpp
// @Author : Gou Zhibin, Liu Jun
// @Version : 2.0
// @Copyright : BUPT
// @Description : main file of MINI-DNS
//============================================================================
#include "main.h"


//============================================================================
// Debugging functions.
//============================================================================
void print_hex(const uint8_t *buf, size_t len)
{
    int i;
    printf("%zu bytes:\n", len);
    for (i = 0; i < len; ++i)
    {
        printf("%02x ", buf[i]);
        if ((i % 16) == 15)
            printf("\n");
    }
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
            printf("Canonical Name Resource Record { name %s }",
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
    printf(". FLAGS: [ QR: %u, OpCode: %u ]", msg->qr, msg->opcode);
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

//============================================================================
// Basic memory operations.
//============================================================================
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

//============================================================================
// Deconding/Encoding functions.
//============================================================================

// @parameter offset: buffer's offset from start
char *decode_domain_name(const uint8_t **buffer, int offset)
{
    // print_hex(*buffer, 50);
    char name[256];
    const uint8_t *buf = *buffer;
    int i = 0;
    int j = 0;

    if (buf[0] >= 0xc0)
    { // 1. pointer type
        int newOffset = (((int)buf[0] & 0x3f) << 8) + buf[1];
        const uint8_t *nameAddr = *buffer - offset + newOffset;
        *buffer += 2;
        return decode_domain_name(&nameAddr, newOffset);
    }

    while (buf[i] != 0 && buf[i] < 0xc0)
    { // address part
        if (i != 0)
        {
            name[j] = '.';
            j++;
        }
        int len = buf[i];
        i += 1;

        memcpy(name + j, buf + i, len);
        i += len;
        j += len;
    }
    if (buf[i] == 0x00)
    { // 2. pure string type
        i++;
    }
    else if (buf[i] >= 0xc0)
    { // 3. string + pointer
        i++;
        int newOffset = (((int)buf[i - 1] & 0x3f) << 8) + buf[i];
        buf = *buffer - offset + newOffset; // address of pointer
        char *nameRemain = decode_domain_name(&buf, newOffset);
        memcpy(name + j, nameRemain, strlen(nameRemain));
        j += strlen(nameRemain);
        i++;
    }
    else
    {
        printf("ERROR: decode_domain_name\n");
    }

    *buffer += i;
    name[j] = '\0';
    return strdup(name);
}

// Character counting
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

    uint32_t flags = get16bits(buffer);
    msg->qr = (flags & QR_MASK) >> 15;
    msg->opcode = (flags & OPCODE_MASK) >> 11;
    msg->aa = (flags & AA_MASK) >> 10;
    msg->tc = (flags & TC_MASK) >> 9;
    msg->rd = (flags & RD_MASK) >> 8;
    msg->ra = (flags & RA_MASK) >> 7;
    msg->rcode = (flags & RCODE_MASK) >> 0;

    msg->qdCount = get16bits(buffer);
    msg->anCount = get16bits(buffer);
    msg->nsCount = get16bits(buffer);
    msg->arCount = get16bits(buffer);
}

void encode_header(struct Message *msg, uint8_t **buffer)
{
    put16bits(buffer, msg->id);

    int flags = 0;
    flags |= (msg->qr << 15) & QR_MASK;
    flags |= (msg->opcode << 11) & OPCODE_MASK;
    flags |= (msg->aa << 10) & AA_MASK;
    flags |= (msg->tc << 9) & TC_MASK;
    flags |= (msg->rd << 8) & RD_MASK;
    flags |= (msg->ra << 7) & RA_MASK;
    flags |= (msg->rcode << 0) & RCODE_MASK;

    put16bits(buffer, flags);
    put16bits(buffer, msg->qdCount);
    put16bits(buffer, msg->anCount);
    put16bits(buffer, msg->nsCount);
    put16bits(buffer, msg->arCount);
}

int decode_resource_records(struct ResourceRecord *rr, const uint8_t **buffer, const uint8_t *oriBuffer)
{
    // print_hex(*buffer, 50);
    rr->name = decode_domain_name(buffer, *buffer - oriBuffer);

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
    {
        rr->rd_data.txt_record.txt_data_len = get8bits(buffer);
        uint8_t txt_len = rr->rd_data.txt_record.txt_data_len;
        char txtData[txt_len + 1];
        for (int i = 0; i < txt_len; ++i)
        {
            printf("%d ", i);
            txtData[i] = get8bits(buffer);
        }
        txtData[txt_len] = '\0';
        rr->rd_data.txt_record.txt_data = strdup(txtData);
        break;
    }
    case CNAME_Resource_RecordType:
        rr->rd_data.cname_record.name = decode_domain_name(buffer, *buffer - oriBuffer);
        break;
    case PTR_Resource_RecordType:
        rr->rd_data.ptr_record.name = decode_domain_name(buffer, *buffer - oriBuffer);
        break;
    case SOA_Resource_RecordType:
        rr->rd_data.soa_record.MName = decode_domain_name(buffer, *buffer - oriBuffer);
        rr->rd_data.soa_record.RName = decode_domain_name(buffer, *buffer - oriBuffer);
        rr->rd_data.soa_record.serial = get32bits(buffer);
        rr->rd_data.soa_record.refresh = get32bits(buffer);
        rr->rd_data.soa_record.retry = get32bits(buffer);
        rr->rd_data.soa_record.expire = get32bits(buffer);
        rr->rd_data.soa_record.minimum = get32bits(buffer);
        break;
    case MX_Resource_RecordType:
        rr->rd_data.mx_record.preference = get16bits(buffer);
        rr->rd_data.mx_record.exchange = decode_domain_name(buffer, *buffer - oriBuffer);
        break;
    case NS_Resource_RecordType:
        rr->rd_data.name_server_record.name = decode_domain_name(buffer, *buffer - oriBuffer);
        break;
    case SRV_Resource_RecordType:
        rr->rd_data.srv_record.priority = get16bits(buffer);
        rr->rd_data.srv_record.weight = get16bits(buffer);
        rr->rd_data.srv_record.port = get16bits(buffer);
        rr->rd_data.srv_record.target = decode_domain_name(buffer, *buffer - oriBuffer);
        break;
    default:
        fprintf(stderr, "ERROR @decode_resource_records: Unknown type %u. => Ignore resource record.\n", rr->type);
        return -1;
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
            fprintf(stderr, "ERROR @encode_resource_records: Unknown type %u. => Ignore resource record.\n", rr->type);
            return 1;
        }

        rr = rr->next;
    }
    return 0;
}

int decode_msg(struct Message *msg, const uint8_t *buffer, int size)
{
    const uint8_t *oriBuffer = buffer;

    decode_header(msg, &buffer);

    // print_hex(buffer, 200);
    // decode Question
    for (uint16_t i = 0; i < msg->qdCount; ++i)
    {
        struct Question *q = malloc(sizeof(struct Question));

        q->qName = decode_domain_name(&buffer, buffer - oriBuffer);

        q->qType = get16bits(&buffer);
        q->qClass = get16bits(&buffer);

        // 添加到链表前端
        q->next = msg->questions;
        msg->questions = q;
    }
    // decode Answer
    for (uint16_t i = 0; i < msg->anCount; ++i)
    {
        struct ResourceRecord *rr = malloc(sizeof(struct ResourceRecord));
        decode_resource_records(rr, &buffer, oriBuffer);
        // if (decode_resource_records(rr, &buffer, oriBuffer) == -1)
        // return -1;
        // 添加到链表前端
        rr->next = msg->answers;
        msg->answers = rr;
    }
    // decode Authority
    for (uint16_t i = 0; i < msg->nsCount; ++i)
    {
        struct ResourceRecord *rr = malloc(sizeof(struct ResourceRecord));
        decode_resource_records(rr, &buffer, oriBuffer);
        // 添加到链表前端
        rr->next = msg->authorities;
        msg->authorities = rr;
    }

    // decode Additional
    // for (uint16_t i = 0; i < msg->arCount; ++i)
    // {
    //     struct ResourceRecord *rr = malloc(sizeof(struct ResourceRecord));
    //     decode_resource_records(rr, &buffer, oriBuffer);
    //     // 添加到链表前端
    //     rr->next = msg->additionals;
    //     msg->additionals = rr;
    // }

    return 0;
}

// @return 0: failure, 1: success
int encode_msg(struct Message *msg, uint8_t **buffer)
{
    struct Question *q;
    int rc;

    encode_header(msg, buffer);
    q = msg->questions;
    while (q) // encode questions
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

//============================================================================
// Local search founctions.
//============================================================================

// @return 0 found, -1 not found
int get_A_Record(uint8_t addr[4], const char domain_name[])
{
    if (findInCache(addr, domain_name))
    { // first find domain name in cache
        if (DEBUG)
            printf("--Find '%s' in Cache.\n", domain_name);
        return 0;
    }
    if (findInTable(addr, domain_name))
    { // then find domain name in table
        if (DEBUG)
            printf("--Find '%s' in Table.\n", domain_name);
        return 0;
    }
    if (DEBUG)
        printf("--Can't find '%s' in Cache or Table.\n", domain_name);
    return -1;
}

int get_AAAA_Record(uint8_t addr[16], const char domain_name[])
{ // TODO: cache for AAAA
    return -1;
}

// @return 0: found, 1 mask, -1 not found
int search_local(struct Message *msg)
{
    // struct ResourceRecord *beg;
    struct ResourceRecord *rr;
    struct Question *q;
    int rc;

    // leave most values intact for response
    msg->qr = 1; // this is a response
    msg->aa = 1; // this server is authoritative
    msg->ra = 1; // no recursion available TODO
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

        switch (q->qType)
        {
        case A_Resource_RecordType:
            rr->rd_length = 4;
            rc = get_A_Record(rr->rd_data.a_record.addr, q->qName);
            if (rc == -1) // not found in local
                break;
            int i;
            for (i = 0; i < 4; ++i)
            {
                if (rr->rd_data.a_record.addr[i] != 0)
                    break;
            }
            if (i == 4)
            { // if ip is 0.0.0.0 is table => mask
                if (DEBUG)
                    printf("Blocked Sites: %s\n", q->qName);
                msg->rcode = NameError_ResponseType; // name not exist
                rc = 1;
                return rc;
            }
            break;

            // case AAAA_Resource_RecordType:
            //     rr->rd_length = 16;
            //     rc = get_AAAA_Record(rr->rd_data.aaaa_record.addr, q->qName);
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
            // msg->rcode = NotImplemented_ResponseType;
            // printf("Cannot answer question of type %d.\n", q->qType);
        }

        if (rc == 0)
        {
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
        q = q->next; // for mutiple Question
    }
    return 0;
}

//============================================================================
// ID conversion funcitons.
//============================================================================
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
            IdTable[i].expireTime = ID_EXPIRE_TIME + time(NULL); // expire time
        }
        break;
    }
    return i;
}

//============================================================================
// Memory release functions.
//============================================================================
void free_resource_records(struct ResourceRecord *rr)
{
    struct ResourceRecord *next;
    while (rr)
    {
        free(rr->name);
        next = rr->next;
        free(rr);
        rr = next;
    }
}

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

//============================================================================
// Communication functions.
//============================================================================
void receive_from_client()
{
    int nbytes = -1;
    uint8_t buffer[BUF_SIZE];
    struct Message msg;
    memset(&msg, 0, sizeof(struct Message));

    nbytes = recvfrom(clientSock, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addr_len);

    if (nbytes < 0 || decode_msg(&msg, buffer, nbytes) != 0)
    {
        return;
    }
    if (DEBUG)
    {
        printf("\n>>> RECV from Client\n");
    }
    if (LOG || DEBUG)
    {
        time_t t;
        struct tm *p;
        time(&t);
        p = localtime(&t);
        printf("@%3d:  ", requstCount++);
        printf("%d-%02d-%02d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
        printf("%02d:%02d:%02d  ", p->tm_hour, p->tm_min, p->tm_sec);
        printf("Client %15s : %-5d   ", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        printf("%s\n", msg.questions->qName);
    }
    if (DEBUG)
    {
        printf("(%d bytes)\n", sizeof(buffer));
        print_query(&msg);
    }

    uint16_t clientId = msg.id;
    int rc = search_local(&msg);
    if (rc == 0 || rc == 1)
    { // 0: Found answers in local: encode msg then send
        // 1: Mask: encode mgs then send
        uint8_t *bufferBegin = buffer;

        if (encode_msg(&msg, &bufferBegin) != 0)
        {
            return;
        }
        int buflen = bufferBegin - buffer;
        if (DEBUG)
        {
            printf("<<< SEND to Client %s:%d ", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            printf("(%d bytes)\n", buflen);
            // print_query(&msg);
        }
        sendto(clientSock, buffer, buflen, 0, (struct sockaddr *)&clientAddr, addr_len);
    }
    else
    { // -1: send to server
        uint16_t nId = newId(clientId, clientAddr);
        if (nId == ID_TABLE_SIZE)
        {
            if (DEBUG)
                printf("ID Table is Full!\n"); // TODO: deal with full
        }
        else
        {
            memcpy(buffer, &nId, sizeof(uint16_t));
            if (DEBUG)
            {
                printf("<<< SEND to Server %s:%d ", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
                printf("(%d bytes) [ID %x=>%x]\n", nbytes, clientId, nId);
            }
            nbytes = sendto(serverSock, buffer, nbytes, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        }
    }
    if (msg.qdCount)
        free_questions(msg.questions);
    if (msg.anCount)
        free_resource_records(msg.answers);
}

void receive_from_server()
{ // receive response from server dns, conversion Id then sent to client, and save to cache
    int nbytes = -1;
    uint8_t buffer[BUF_SIZE];
    struct Message msg;
    memset(&msg, 0, sizeof(struct Message));

    // server dns
    nbytes = recvfrom(serverSock, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddr, &addr_len);
    if (nbytes < 0 || decode_msg(&msg, buffer, nbytes) != 0)
    {
        return;
    }
    if (DEBUG)
    {
        printf("\n>>> RECV from Server\n");
    }
    if (LOG || DEBUG)
    {
        time_t t;
        struct tm *p;
        time(&t);
        p = localtime(&t);
        printf("@%3d:  ", requstCount++);
        printf("%d-%02d-%02d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
        printf("%02d:%02d:%02d  ", p->tm_hour, p->tm_min, p->tm_sec);
        printf("Server %15s : %-5d   ", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
        printf("%s\n", msg.questions->qName);
    }
    if (DEBUG)
    {
        printf("(%d bytes)\n", sizeof(buffer));
        print_query(&msg);
    }

    // ID conversion
    uint16_t nId = msg.id;
    uint16_t clientId = htons(IdTable[nId].clientId);
    memcpy(buffer, &clientId, sizeof(uint16_t));

    struct sockaddr_in ca = IdTable[nId].clientAddr;

    IdTable[nId].expireTime = 0; // set to expired
    if (DEBUG)
    {
        printf("<<< SEND to Client %s:%d ", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        printf("(%d bytes) [ID %x=>%x]\n", nbytes, nId, ntohs(clientId));
    }
    sendto(clientSock, buffer, nbytes, 0, (struct sockaddr *)&ca, sizeof(ca));

    // save A type to cache
    if (msg.anCount)
    {
        struct ResourceRecord *rr = msg.answers;
        while (rr)
        {
            if (rr->type == A_Resource_RecordType)
            {
                char *domain_name = rr->name;
                uint8_t *addr = rr->rd_data.a_record.addr;
                updateCache(addr, domain_name);
                if (DEBUG)
                    printCache();
            }
            rr = rr->next;
        }
    }

    // free memory
    if (msg.qdCount)
        free_questions(msg.questions);
    if (msg.anCount)
        free_resource_records(msg.answers);
}

//============================================================================
// Initial functions.
//============================================================================
void print_program_info()
{
    printf("\n\
███╗   ███╗██╗███╗   ██╗██╗      ██████╗ ███╗   ██╗███████╗\n\
████╗ ████║██║████╗  ██║██║      ██╔══██╗████╗  ██║██╔════╝\n\
██╔████╔██║██║██╔██╗ ██║██║█████╗██║  ██║██╔██╗ ██║███████╗\n\
██║╚██╔╝██║██║██║╚██╗██║██║╚════╝██║  ██║██║╚██╗██║╚════██║\n\
██║ ╚═╝ ██║██║██║ ╚████║██║      ██████╔╝██║ ╚████║███████║\n\
╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝      ╚═════╝ ╚═╝  ╚═══╝╚══════╝\n\
                                                           \n");
    printf("MINI-DNS 2.0 LTS\n");
    printf("@Auther: Gou Zhibin, Liu Jun\n");
    printf("@Teacher: Wu Qifan\n");
    printf("@Version: 2.0\n");
    printf("@Copyright: BUPT\n");
    printf("@Description: A simple DNS server, course design of computer network\n");
    printf("@Usage: minidns [-d] [-h] [-l] [-p <port>] [-f <db-file>] [-s <dns-server>]\n");
    printf("---------------------------------------------------------------------------\n");
}

void parsing_parameters(int argc, char *argv[])
{
    int o;
    const char *optstring = "dhlf:p:s:"; // 有三个选项-abc，其中c选项后有冒号，所以后面必须有参数
    while ((o = getopt(argc, argv, optstring)) != -1)
    {
        switch (o)
        {
        case 'd':
            DEBUG = true;
            printf("DEBUG is on.\n");
            break;
        case 'l':
            LOG = true;
            printf("LOG is on.\n");
            break;
        case 'f':
            strcpy(DNS_TABLE_FILE, optarg);
            break;
        case 'p':
            PORT = atoi(optarg);
            break;
        case 's':
            strcpy(PUBLIC_DNS_IP, optarg);
            break;
        case 'h':
        default:
            printf("Usage:  dnsrelay [-d] [-h] [-l] [-p <port>] [-f <db-file>] [-s <dns-server>]\n");
            printf("Where:  -d                     (print debug information)\n");
            printf("        -h                     (print help)\n");
            printf("        -l                     (print log)\n");
            printf("        -f db-file             (specify DNS table file)\n");
            printf("        -p port                (specify port number)\n");
            printf("        -s dns-server          (specify DNS server)\n");
            exit(-1);
        }
    }
}

void init_data_structure()
{
    // init Trie
    cacheTrie = (struct Trie *)malloc(sizeof(struct Trie));
    tableTrie = (struct Trie *)malloc(sizeof(struct Trie));
    cacheTrie->totalNode = 0;
    tableTrie->totalNode = 0;
    cacheSize = 0;

    // read dnsrelay.txt
    char domain[maxStr] = {0};
    char ipAddr[maxStr] = {0};

    FILE *fp = NULL;
    if ((fp = fopen(DNS_TABLE_FILE, "r")) == NULL)
    {
        printf("ERROR: Can't find file '%s'\n", DNS_TABLE_FILE);
        exit(-1);
    }
    unsigned char ip[4];
    while (!feof(fp))
    {
        fscanf(fp, "%s", ipAddr);
        fscanf(fp, "%s", domain);
        transIp(ip, ipAddr);
        insertNode(tableTrie, domain, ip);
    }

    // init LRU
    head = (struct Node *)malloc(sizeof(struct Node));
    head->next = NULL;
    tail = head;

    // init ID table
    for (int i = 0; i < ID_TABLE_SIZE; i++)
    {
        IdTable[i].clientId = 0;
        IdTable[i].expireTime = 0;
        memset(&(IdTable[i].clientAddr), 0, sizeof(struct sockaddr_in));
    }
}

void init_socket()
{
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // buffer for input/output binary packet
    clientSock = socket(AF_INET, SOCK_DGRAM, 0);
    serverSock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(PORT);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(PUBLIC_DNS_IP);
    serverAddr.sin_port = htons(PORT);

    // reuse port
    const int REUSE = 1;
    setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&REUSE, sizeof(REUSE));

    if (bind(clientSock, (SOCKADDR *)&clientAddr, addr_len) < 0)
    {
        printf("ERROR: Could not bind: %s\n", strerror(errno));
        exit(-1);
    }
    printf("DNS server: %s\n", PUBLIC_DNS_IP);
    printf("Listening on port %u\n", PORT);
}

//============================================================================
// Main functions.
//============================================================================
int main(int argc, char *argv[])
{
    print_program_info();
    parsing_parameters(argc, argv);

    init_data_structure();
    init_socket();
    
    if (MODE == NONBLOCK_MODE) // nonblock
    {
        int nonBlock = 1;
        ioctlsocket(clientSock, FIONBIO, (u_long FAR *)&nonBlock);
        ioctlsocket(serverSock, FIONBIO, (u_long FAR *)&nonBlock);
        while (1)
        {
            receive_from_client();
            receive_from_server();
        }
    }
    else if (MODE == SELECT_MODE) // select
    {
        fd_set fdread;
        while (1)
        {
            FD_ZERO(&fdread);
            FD_SET(clientSock, &fdread);
            FD_SET(serverSock, &fdread);
            TIMEVAL tv; //设置超时等待时间
            tv.tv_sec = 0;
            tv.tv_usec = 1;
            int ret = select(0, &fdread, NULL, NULL, &tv);
            if (SOCKET_ERROR == ret)
            {
                printf("ERROR SELECT:%d.\n", WSAGetLastError());
            }
            if (ret > 0)
            {
                if (FD_ISSET(clientSock, &fdread))
                {
                    receive_from_client();
                }
                if (FD_ISSET(serverSock, &fdread))
                {
                    receive_from_server();
                }
            }
        }
    }

    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
    return 0;
}
