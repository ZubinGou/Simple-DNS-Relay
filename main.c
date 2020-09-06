#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define maxn 2000001
#define maxm 26+10+1
#define maxStr 1001
int tree[maxn][maxm];
bool endFlag[maxn];
int totolNode;

unsigned char toIp[maxn][4];

void insertNode(char* str , char * ip)
{
    int len = strlen(str);
    int root = 0;
    for(int i = 0 ; i < len ; i++)
    {
        int id;
        /*
         * id
         * 'a'~'z' -> 0:25
         * '0'~'9' -> 26:35
         * '-'     -> 36
        */
        if(str[i]>='0'&&str[i]<='9')
            id = 26 + str[i] - '0';
        else if(str[i]>='a'&&str[i]<='z')
            id = str[i]-'a';
        else
            id = 36;

        if(!tree[root][id])
            tree[root][id] = ++totolNode;
        root = tree[root][id];

    }
    int ipLen = strlen(ip);
    unsigned num = 0;
    int cnt = 0;
    for(int i = 0 ; i <= ipLen; i++)
    {
        if(ip[i]=='.'||i == ipLen)
        {
            toIp[root][cnt++] = num;
            num = 0;
        }
        else
        {
            num = num*10 + ip[i]-'0';
        }
    }
    endFlag[root] = true;
}

int findNode(char* str)
{
    int len = strlen(str);
    int root = 0 ;
    for(int i = 0 ; i< len ; i++)
    {
        int id;
        /*
         * id
         * 'a'~'z' -> 0:25
         * '0'~'9' -> 26:35
         * '-'     -> 36
        */
        if(str[i]>='0'&&str[i]<='9')
            id = 26 + str[i] - '0';
        else if(str[i]>='a'&&str[i]<='z')
            id = str[i]-'a';
        else
            id = 36;

        if(!tree[root][id])return 0;
        root = tree[root][id];
    }
    return root;
}

/*
*simplify domain name
* eg: www.ABC.com -> abccom
*/
void simplifyName(char* str)
{
    int len = strlen(str);
    char ss[255];
    int ssLen = 0;
    for(int i = 0 ; i < len ; i++)
    {
//        if(str[i]=='.')
//            continue;
        ss[ssLen++] = str[i];
    }
    ss[ssLen] = '\0';
    char*sss = strlwr(ss);
    memcpy(str,sss,sizeof(ss));
}

char name[maxStr];
char ipAddr[maxStr];
int main()
{
    freopen("C:\\Users\\12530\\Documents\\C++\\in.txt","r",stdin);
    totolNode = 0;
    int cnt = 0 ;
    int flag = 1;
    while(cnt!=909)
    {
        printf("%d:",cnt);
        cnt++;
        scanf("%s",ipAddr);
        scanf("%s",name);
        simplifyName(name);
        printf("%s  %s\n",name,ipAddr);
        insertNode(name,ipAddr);
    }
    while(scanf("%s",ipAddr)!=EOF)
    {
        scanf("%s",name);
        printf("%s  %s   findIp:",name,ipAddr);
        simplifyName(name);
        int num = findNode(name);
        if(num)
        {
            for(int i = 0 ; i < 4; i++)
                printf("%u.",(unsigned int)(toIp[num][i]));
        }
        else
        {
            printf("No Such Domain Name");
            flag = 0;
        }
        printf("\n");
    }
    printf("flag = %d",flag);
    system("pause");
    return 0;
}
