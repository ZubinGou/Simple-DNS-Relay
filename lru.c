#include "storage.h"

void printCache()
{
    struct Node *p = head->next;
    while (p != NULL)
    {
        printf("%s -> \n", p->domain);
        p = p->next;
    }
    printf("\n");
}

void updateCache(unsigned char ipAddr[4], const char domain[])
{
    int num = findNode(cacheTrie, domain);
    if (num) //domain exits in DNS
    {
        struct Node *q, *p;
        q = head;
        while (q->next != NULL)
        {
            if (memcmp(q->next->domain, domain, strlen(domain)) == 0)
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
    printCache();
}

bool findInCache(unsigned char ipAddr[4], const char domain[])
{

    int num = findNode(cacheTrie,domain);
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
    int num = findNode(tableTrie,domain);
    printf("findInTable num = %d\n", num);
    if (num == 0)
    {
        return false;
    }
    memcpy(ipAddr, tableTrie->toIp[num], sizeof(unsigned char) * 4);
    return true;
}