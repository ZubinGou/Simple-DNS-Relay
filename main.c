#include "main.h"
#include "head.h"

/*
*simplify domain name
*/
void simplifyName(char *str)
{
    int len = strlen(str);
    char ss[255];
    int ssLen = 0;
    for (int i = 0; i < len; i++)
    {
        //        if(str[i]=='.')
        //            continue;
        ss[ssLen++] = str[i];
    }
    ss[ssLen] = '\0';
    char *sss = strlwr(ss);
    memcpy(str, sss, sizeof(ss));
}

/*
 * use to insert a word in trie
 * parameter@trie the address of a trie
 * parameter@str domain name
 * parameter@ip  ipAddr
 * eg:insertNode(trie,"www.baidu.com","10.3.8.211");
*/
void insertNode(struct Trie *trie, char *str, unsigned char ip[4])
{
    if(str[0]=='\0')return;
    char ss[300] = {0};
    memcpy(ss,str,sizeof(ss));
    simplifyName(ss);
    int len = strlen(ss);
    int root = 0;
    for (int i = 0; i < len; i++)
    {
        int id;
        if(ss[i]>='0'&&ss[i]<='9')
            id = 26 + ss[i] - '0';
        else if(ss[i]>='a'&&ss[i]<='z')
            id = ss[i]-'a';
        else if(ss[i] == '-')
            id = 36;
        else
            id = 37;

        if (!trie->tree[root][id])
            trie->tree[root][id] = ++trie->totalNode;
        trie->pre[trie->tree[root][id]] = root;
        root = trie->tree[root][id];
    }

    memcpy(trie->toIp[root], ip, sizeof(unsigned char) * 4);
    trie->endFlag[root] = true;
}

/*
 * use to find a word in trie
 * parameter@trie the address of a trie
 * parameter@str domain name
 * return@if domain name exists in the trie,return the nodeNum of the ipAddr of this domain name.
 *        else return 0
 * eg:int nodeNum = findNode(trie,"www.baidu.com");
 *    toIp[nodeNum] is the ipAddr
*/
int findNode(struct Trie *trie, char *str)
{
    if(str[0]=='\0')return 0;
    char ss[300] = {0};
    memcpy(ss,str,sizeof(ss));
    simplifyName(ss);
    int len = strlen(ss);
    int root = 0 ;
    for(int i = 0 ; i< len ; i++)
    {
        int id;
        /*
         * id
         * 'a'~'z' -> 0:25
         * '0'~'9' -> 26:35
         * '-'     -> 36
         * '.'     -> 37
        */
        if(ss[i]>='0'&&ss[i]<='9')
            id = 26 + ss[i] - '0';
        else if(ss[i]>='a'&&ss[i]<='z')
            id = ss[i]-'a';
        else if(ss[i] == '-')
            id = 36;
        else
            id = 37;

        if (!trie->tree[root][id])
            return 0;
        root = trie->tree[root][id];
    }
    if (trie->endFlag[root] == false)
        return 0;
    return root;
}

/*
 * use to delete a word in trie
 * parameter@trie the address of a trie
 * parameter@str the domain name
 * eg: deleteNode(trie,"www.baidu.com");
*/
void deleteNode(struct Trie *trie, char *str)
{
    if(str[0]=='\0')return;
    char ss[300] = {0};
    memcpy(ss,str,sizeof(ss));
    simplifyName(ss);
    int root = findNode(trie,ss);
    if(!root)return;
    trie->endFlag[root] = false;
    //删除节点
    int strNum = strlen(ss)-1;
    while(root!=0){
        int id;
        if(ss[strNum]>='0'&&ss[strNum]<='9')
            id = 26 + ss[strNum] - '0';
        else if(ss[strNum]>='a'&&ss[strNum]<='z')
            id = ss[strNum]-'a';
        else if(ss[strNum] == '-')
            id = 36;
        else
            id = 37;

        bool haveChild = false;
        for (int i = 0; i < maxm; i++)
        {
            if (trie->tree[root][i] != 0)
            {
                haveChild = true;
                break;
            }
        }
        if (haveChild)
            break;
        trie->tree[trie->pre[root]][strNum] = 0;
        int tmp = trie->pre[root];
        trie->pre[root] = 0;
        root = tmp;
        strNum--;
    }
}

void transIp(unsigned char ip[4], char *rowIp)
{
    int ipLen = strlen(rowIp);
    unsigned num = 0;
    int cnt = 0;
    for (int i = 0; i <= ipLen; i++)
    {
        if (rowIp[i] == '.' || i == ipLen)
        {
            ip[cnt++] = num;
            num = 0;
        }
        else
        {
            num = num * 10 + rowIp[i] - '0';
        }
    }
}

void output()
{
    struct Node *p = head->next;
    while (p != NULL)
    {
        printf("%s -> ", p->domain);
        p = p->next;
    }
    printf("\n");
}

void updateCache(unsigned char ipAddr[4], char domain[])
{
    int num = findNode(cacheTrie, domain);
    if (num) //domain exits in DNS
    {
        struct Node *q, *p;
        q = head;
        while (q->next != NULL)
        {
            if (memcmp(q->next->domain, domain, sizeof(domain)) == 0)
            {
                p = q->next;
                q->next = p->next;
                p->next = NULL;
                tail->next = p;
                tail = p;
                break;
            }
            q = q->next;
        }
    }
    else
    {
        struct Node *q = (struct Node *)malloc(sizeof(struct Node));
        memcpy(q->domain, domain, sizeof(q->domain));
        if (cacheSize < maxCacheSize)
        {
            insertNode(cacheTrie, domain, ipAddr);
            cacheSize++;
            q->next = NULL;
            tail->next = q;
            tail = q;
        }
        else //delete lru Node
        {
            insertNode(cacheTrie, domain, ipAddr);
            q->next = NULL;
            tail->next = q;
            tail = q;
            q = head->next;
            head->next = q->next;
            printf("delete %s\n", q->domain);
            deleteNode(cacheTrie, q->domain);
            free(q);
        }
    }
    printf("updata:");
    output();
}

bool findInCache(unsigned char ipAddr[4], const char domain[])
{

    // int num = findNode(cacheTrie,domain);
    int num = 0;
    printf("findInCache num = %d\n", num);

    if (num == 0) //domain name does not exist in the cache
    {
        return false;
    }
    memcpy(ipAddr, cacheTrie->toIp[num], sizeof(unsigned char) * 4);
    updateCache(ipAddr, domain);
    return true;
}

bool findInTable(unsigned char ipAddr[4], const char domain[])
{
    // int num = findNode(tableTrie,domain);
    int num = 13;
    if (num == 0)
    {
        return false;
    }
    memcpy(ipAddr, tableTrie->toIp[num], sizeof(unsigned char) * 4);
    printf("findInTable num = %d\n", num);
    return true;
}

int get_A_Record(uint8_t addr[4], const char domain_name[])
{

    if (findInCache(addr, domain_name))
    { // 在缓存中找到，则使用缓存中的ip地址
        return 0;
    }
    if (findInTable(addr, domain_name))
    {  // 在对照表中找到，则使用对照表中的ip地址
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
    printf("decode_domain_name:\n");
    print_hex(*buffer, 50);

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
        printf("%x\n", *buffer);
        printf("offset = %d\n", offset);
        printf("buf[i] = %d\n", buf[i]);
        printf("buf=\n");
        print_hex(buf, 50);

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
        printf("error\n");
    }
    *buffer += i;
    name[j] = '\0';
    // *oribuffer += i + 1; //also jump over the last 0
    printf("get decode name: %s\n\n", name);
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
    // const uint8_t *baseAddr = oriBuffer;
    printf("==decode_resource_records\n");
    printf("%x\n", buffer);
    printf("%x\n", oriBuffer);

    rr->name = decode_domain_name(buffer, *buffer - oriBuffer);

    // printf("begin");
    // print_hex(*buffer, 200);

    rr->type = get16bits(buffer);
    rr->class = get16bits(buffer);
    rr->ttl = get32bits(buffer);
    rr->rd_length = get16bits(buffer);

    printf(" decode_resource_records then:\n");
    print_hex(*buffer, 200);
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
    printf("0: %x\n", buffer);
    printf("0: %x\n", oriBuffer);

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

        print_resource_record(rr);
        switch (q->qType)
        {
        case A_Resource_RecordType:
            rr->rd_length = 4;
            uint8_t tempAddr[4];
            // rc = get_A_Record(tempAddr, q->qName);

            char *qn;
            // = q->qName;
            // for (int i = 0; i < 4; i ++) {
            //     printf("%d ", tempAddr[i]);
            // }
            // rr->rd_data.a_record.addr = tempAddr;

            rc = get_A_Record(rr->rd_data.a_record.addr, qn);
            printf("sizeof(qn): %d\n", strlen(qn));
            printf("%s\n", qn);
            // for (int i = 0; qn[i] != "\0"; i++) {
            //     q->qName[i] = qn[i];
            // }
            print_resource_record(rr);

            for (int i = 0; i < 4; i++)
                if (i != 3)
                    printf("%u.", rr->rd_data.a_record.addr[i]);
                else
                    printf("%u", rr->rd_data.a_record.addr[i]);
            printf("\n");
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
            printf("if rc == 0: \n");
            print_resource_record(rr);
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
    printf("encode_msg\n");
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
    printf("rc = %d\n", rc);
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
    printf("%d => %d\n", clientId, i);
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
        printf("resolver_process done:\n");

        print_query(&msg);
        if (encode_msg(&msg, &bufferBegin) != 0)
        {
            return;
        }
        int buflen = bufferBegin - buffer;
        printf("sendto");
        sendto(clientSock, buffer, buflen, 0, (struct sockaddr *)&clientAddr, addr_len);
        printf("sendto Done");
    }
    else
    { // 否则，将请求修改ID后转发
        uint16_t nId = newId(clientId, clientAddr);
        if (nId == ID_TABLE_SIZE)
        {
            printf("ID Table if Full\n");
        }
        else
        {
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
    char *domain_name = msg.answers->name;
    uint8_t *addr = msg.answers->rd_data.a_record.addr;
    updateCache(addr, domain_name);
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
    // 初始化字典树
    cacheTrie = (struct Trie *)malloc(sizeof(struct Trie));
    tableTrie = (struct Trie *)malloc(sizeof(struct Trie));
    cacheTrie->totalNode = 0;
    tableTrie->totalNode = 0;
    cacheSize = 0;

    // 读入dnsrelay.txt
    char domain[maxStr] = {0};
    char ipAddr[maxStr] = {0};

    printf("main1");
    FILE *fp = NULL;
    if ((fp = fopen("./dnsrelay.txt", "r")) == NULL)
    {
        printf("找不到dnsrelay.txt");
        return -1;
    }
    unsigned char ip[4];
    printf("main2");
    while (!feof(fp))
    {
        fscanf(fp, "%s", ipAddr);
        fscanf(fp, "%s", domain);
        transIp(ip, ipAddr);
        //printf("%d:%s  %s\n",cnt,name,ipAddr);
        insertNode(tableTrie, domain, ip);
    }
    printf("main3");

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
        printf("\n-------1 receiveFromLocal-------\n");
        receiveFromLocal();
        printf("\n-------2 receiveFromPublic-------\n");
        receiveFromPublic();
        // Sleep(1);
    }

    closesocket(clientSock);
    WSACleanup();
    return 0;
}
