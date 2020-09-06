#include "head.h"

char domain[maxStr];
char ipAddr[maxStr];

void test1()
{
    freopen("C:\\Users\\12530\\Documents\\C++\\in.txt","r",stdin);
    //freopen("C:\\Users\\12530\\Documents\\C++\\out.txt","w",stdout);
    int cnt = 0 ;
    head = (struct Node*)malloc(sizeof(struct Node));
    head->next = NULL;
    tail = head;
    while(cnt!=909)
    {
        unsigned char ip[4];
        scanf("%s",ipAddr);
        scanf("%s",domain);
        transIp(ip,ipAddr);
        //printf("%d:%s  %s\n",cnt,name,ipAddr);
        insertNode(tableTrie,domain,ip);
        cnt++;
    }
    while(scanf("%s",domain)!=EOF)
    {
        unsigned char ipAddr[4];
        bool flag1 = findInCache(ipAddr,domain);
        if(flag1 == true)
        {
            printf("%s\t in cache\n",domain);
            for(int i = 0 ; i < 4; i++)
                if(i!=3)
                    printf("%u.",ipAddr[i]);
                else
                    printf("%u",ipAddr[i]);
            printf("\n");
        }
        else
        {
            bool flag2 = findInTable(ipAddr,domain);
            if(flag2 == true)
            {
                printf("%s\t in table\n",domain);
                for(int i = 0 ; i < 4; i++)
                    if(i!=3)
                        printf("%u.",ipAddr[i]);
                    else
                        printf("%u",ipAddr[i]);
                printf("\n");
                updateCache(ipAddr,domain);
            }
            else
            {
                printf("No such domain.");
            }
        }

    }
}
void test2()
{
    head = (struct Node*)malloc(sizeof(struct Node));
    head->next = NULL;
    tail = head;
    char ss[255]="";
    while(scanf("%s",ss)==1)
    {
        updateCache("10.3.8.211",ss);
        int num = findNode(cacheTrie,ss);
        if(num)
        {
            for(int i = 0 ; i < 4; i++)
                if(i!=3)
                    printf("%u.",(unsigned int)(cacheTrie->toIp[num][i]));
                else
                    printf("%u",(unsigned int)(cacheTrie->toIp[num][i]));
        }
        else
        {
            printf("No Such Domain Name");
        }
        printf("\n");
        output();
    }
}
int main()
{
    cacheTrie = (struct Trie*)malloc(sizeof(struct Trie));
    tableTrie = (struct Trie*)malloc(sizeof(struct Trie));
    cacheTrie->totalNode = 0;
    tableTrie->totalNode = 0;
    cacheSize = 0;
    test1();
    system("pause");
    return 0;
}
