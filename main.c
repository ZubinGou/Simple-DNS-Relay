//============================================================================
// @Name : main.cpp
// @Author : Gou Zhibin, Liu Jun
// @Version : 2.0
// @Copyright : BUPT
// @Description : main file of Mini-Dns
//============================================================================
#include "main.h"
#include "storage.h"

int get_A_Record(uint8_t addr[4], const char domain_name[])
{

    if (findInCache(addr, domain_name))
    { // 在缓存中找到，则使用缓存中的ip地址
        if (DEBUG)
            printf("Find %s in Cache.\n", domain_name);
        return 0;
    }
    if (findInTable(addr, domain_name))
    { // 在对照表中找到，则使用对照表中的ip地址
        if (DEBUG)
            printf("Find %s in Table.\n", domain_name);
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
char *decode_domain_name(const uint8_t **buffer, int offset)
{
    // printf("decode_domain_name:\n");
    // print_hex(*buffer, 50);

    char name[256];
    const uint8_t *buf = *buffer;
    int i = 0;
    int j = 0;

    if (buf[0] == 0xc0)
    { // 1. 指针类型
        const uint8_t *nameAddr = *buffer - offset + buf[1];
        *buffer += 2;
        return decode_domain_name(&nameAddr, buf[1]);
    }

    while (buf[i] != 0 && buf[i] != 0xc0)
    { // 地址部分
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
    { // 2. 纯地址类型
        i++;
    }
    else if (buf[i] == 0xc0)
    { // 3. 地址 + 指针
        i++;
        buf = *buffer - offset + buf[i]; // 指针指向地址
        int k = 0;
        while (buf[k] != 0)
        { // 数字部分
            name[j] = '.';
            j++;
            int len = buf[k];
            k++;

            memcpy(name + j, buf + k, len);
            k += len;
            j += len;
        }
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
        rr->rd_data.txt_record.txt_data_len = get8bits(buffer);
        for (int i = 0; i < rr->rd_data.txt_record.txt_data_len; i++)
            rr->rd_data.txt_record.txt_data[i] = get8bits(buffer);
        break;
    case CNAME_Resource_RecordType:
        rr->rd_data.cname_record.name = decode_domain_name(buffer, *buffer - oriBuffer);
    default:
        fprintf(stderr, "Unknown type %u. => Ignore resource record.\n", rr->type);
        return -1;
    }
    return 0;
}

int decode_msg(struct Message *msg, const uint8_t *buffer, int size)
{
    const uint8_t *oriBuffer = buffer;

    decode_header(msg, &buffer);

    // printf("decode header: ");
    // print_hex(buffer, 200);

    // 解析Question
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

    // 解析Answer
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
// 返回-1代表本地找不到，1表示屏蔽，0表示本地找到
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

        switch (q->qType)
        {
        case A_Resource_RecordType:
            rr->rd_length = 4;
            rc = get_A_Record(rr->rd_data.a_record.addr, q->qName);
            if (rc == -1) // 本地没有找到
                break;
            int i;
            for (i = 0; i < 4; ++i)
            {
                if (rr->rd_data.a_record.addr[i] != 0)
                    break;
            }
            if (i == 4)
            { // 本地查到地址为0.0.0.0，需要屏蔽
                if (DEBUG)
                    printf("Blocked Sites: %s\n", q->qName);
                msg->rcode = NameError_ResponseType; // 只在权威DNS服务器的响应中有意义，表示请求中的域名不存在。
                rc = 1;
                return rc;
            }
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
        q = q->next; // 多个Question的情况
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
            IdTable[i].expireTime = ID_EXPIRE_TIME + time(NULL); // 失效时间
        }
        break;
    }
    return i;
}

/* 内存释放部分 */
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
    if (DEBUG) {
        printf("\nRECV ");
    }
    if (LOG || DEBUG) {
        time_t t;
        struct tm *p;
        time(&t);
        p = localtime(&t); //取得当地时间
        printf("@%3d:  ", requstCount++);
        printf ("%d-%02d-%02d ", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday);
        printf("%02d:%02d:%02d  ", p->tm_hour, p->tm_min, p->tm_sec);
        printf("Client %15s:%-5d   ", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        printf("%s\n", msg.questions->qName);
    }
    if (DEBUG) {
        printf("(%d bytes)\n", sizeof(buffer));
        print_query(&msg);
    }

    uint16_t clientId = msg.id;
    int rc = resolver_process(&msg);
    if (rc == 0 || rc == 1)
    {   // 0: 可以在本地找到所有question的answer，则将添加answer的msg编码后发送给客户端
        // 1: 屏蔽功能
        uint8_t *bufferBegin = buffer;

        if (encode_msg(&msg, &bufferBegin) != 0)
        {
            return;
        }
        int buflen = bufferBegin - buffer;
        if (DEBUG) {
            printf("SEND to %15s:%-5d ", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            printf("(%d bytes)\n", buflen);
            // print_query(&msg);
        }
        sendto(clientSock, buffer, buflen, 0, (struct sockaddr *)&clientAddr, addr_len);
    }
    else
    { // 否则，将请求修改ID后转发
        uint16_t nId = newId(clientId, clientAddr);
        if (nId == ID_TABLE_SIZE)
        {
            if (DEBUG)
                printf("ID Table is Full!\n"); // TODO 满了之后的处理
        }
        else
        {
            memcpy(buffer, &nId, sizeof(uint16_t));
            if (DEBUG) {
                printf("SEND to %s:%d ", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
                printf("(%d bytes) [ID %x=>%x]\n", nbytes, clientId, nId);
            }
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
    if (LOG || DEBUG) {
        time_t t;
        struct tm *p;
        time(&t);
        p = localtime(&t); //取得当地时间
        printf("@%3d:  ", requstCount++);
        printf ("%d-%02d-%02d ", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday);
        printf("%02d:%02d:%02d  ", p->tm_hour, p->tm_min, p->tm_sec);
        printf("Server %15s:%-5d   ", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
        printf("%s\n", msg.questions->qName);
    }
    if (DEBUG)
        print_query(&msg);

    // ID映射
    uint16_t nId = msg.id;
    IdTable[nId].clientId = htons(IdTable[nId].clientId);
    memcpy(buffer, &IdTable[nId].clientId, sizeof(uint16_t));

    struct sockaddr_in ca = IdTable[nId].clientAddr;

    IdTable[nId].expireTime = 0; // 响应完毕后立即过期
    sendto(clientSock, buffer, nbytes, 0, (struct sockaddr *)&ca, sizeof(ca));

    // 存储answer中的A型记录到cache
    if (msg.anCount)
    {
        struct ResourceRecord *rr = msg.answers;
        while (rr)
        {
            if (rr->type == A_Resource_RecordType) {
                char *domain_name = rr->name;
                uint8_t *addr = rr->rd_data.a_record.addr;
                updateCache(addr, domain_name);
                if (DEBUG)
                    printCache();
            }
            rr = rr->next;
        }
    }

    // 释放内存
    if (msg.qdCount)
        free_questions(msg.questions);
    if (msg.anCount)
        free_resource_records(msg.answers);
}

void printInfo()
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

int main(int argc, char *argv[])
{
    printInfo();
    // 解析参数
    int o;
    const char *optstring = "dhlf:p:s:"; // 有三个选项-abc，其中c选项后有冒号，所以后面必须有参数
    while ((o = getopt(argc, argv, optstring)) != -1) {
        switch (o) {
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
                return 0;
        }
    }

    // 初始化字典树
    cacheTrie = (struct Trie *)malloc(sizeof(struct Trie));
    tableTrie = (struct Trie *)malloc(sizeof(struct Trie));
    cacheTrie->totalNode = 0;
    tableTrie->totalNode = 0;
    cacheSize = 0;

    // 读入dnsrelay.txt
    char domain[maxStr] = {0};
    char ipAddr[maxStr] = {0};

    FILE *fp = NULL;
    if ((fp = fopen(DNS_TABLE_FILE, "r")) == NULL)
    {
        printf("ERROR: Can't find file '%s'\n", DNS_TABLE_FILE);
        return -1;
    }
    unsigned char ip[4];
    while (!feof(fp))
    {
        fscanf(fp, "%s", ipAddr);
        fscanf(fp, "%s", domain);
        transIp(ip, ipAddr);
        //printf("%d:%s  %s\n",cnt,name,ipAddr);
        insertNode(tableTrie, domain, ip);
    }

    // LRU算法初始化
    head = (struct Node *)malloc(sizeof(struct Node));
    head->next = NULL;
    tail = head;

    // 初始化ID转换表
    for (int i = 0; i < ID_TABLE_SIZE; i++)
    {
        IdTable[i].clientId = 0;
        IdTable[i].expireTime = 0;
        memset(&(IdTable[i].clientAddr), 0, sizeof(struct sockaddr_in));
    }

    WSADATA wsaData;
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
    serverAddr.sin_addr.s_addr = inet_addr(PUBLIC_DNS_IP); // ?
    // InetPton(AF_INET, _T(PUBLIC_DNS_IP), &serverAddr.sin_addr.s_addr);
    serverAddr.sin_port = htons(PORT);

    // 防止PORT被占用
    const int REUSE = 1;
    setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&REUSE, sizeof(REUSE));

    if (bind(clientSock, (SOCKADDR *)&clientAddr, addr_len) < 0)
    {
        printf("ERROR: Could not bind: %s\n", strerror(errno));
        return 1;
    }
    printf("DNS server: %s\n", PUBLIC_DNS_IP);
    printf("Listening on port %u\n", PORT);

    if(MODE == NONBLOCK_MODE)
    {
        int nonBlock = 1;
        ioctlsocket(clientSock, FIONBIO, (u_long FAR *)&nonBlock);
        ioctlsocket(serverSock, FIONBIO, (u_long FAR *)&nonBlock);
        while (1)
        {
            receiveFromLocal();
            receiveFromPublic();
        }
    }
    else if(MODE == SELECT_MODE)
    {
        fd_set fdread;
        while (1)
        {
            FD_ZERO(&fdread);
            FD_SET(clientSock, &fdread);
            FD_SET(serverSock, &fdread);
            TIMEVAL tv;//设置超时等待时间
            tv.tv_sec = 0;
            tv.tv_usec = 10;
            int ret = select(0, &fdread, NULL, NULL, &tv);
            if (SOCKET_ERROR == ret)
            {
                printf("ERROR SELECT:%d.\n", WSAGetLastError());
            }
            if (ret > 0)
            {
                if (FD_ISSET(clientSock, &fdread))
                {
                    receiveFromLocal();
                }
                if (FD_ISSET(serverSock, &fdread))
                {
                    receiveFromPublic();
                }
            }
        }
    }

    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
    return 0;
}
