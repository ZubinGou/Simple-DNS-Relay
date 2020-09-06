#include "head.h"

/*
*simplify domain name
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

/*
 * use to insert a word in trie
 * parameter@trie the address of a trie
 * parameter@str domain name
 * parameter@ip  ipAddr
 * eg:insertNode(trie,"www.baidu.com","10.3.8.211");
*/
void insertNode(struct Trie* trie , char* str ,unsigned char ip[4])
{
    if(str[0]=='\0')return;
    simplifyName(str);
    int len = strlen(str);
    int root = 0;
    for(int i = 0 ; i < len ; i++)
    {
        int id;
        if(str[i]>='0'&&str[i]<='9')
            id = 26 + str[i] - '0';
        else if(str[i]>='a'&&str[i]<='z')
            id = str[i]-'a';
        else if(str[i] == '-')
            id = 36;
        else
            id = 37;

        if(!trie->tree[root][id])
            trie->tree[root][id] = ++trie->totalNode;
        trie->pre[trie->tree[root][id]] = root;
        root = trie->tree[root][id];
    }

    memcpy(trie->toIp[root],ip,sizeof(unsigned char)*4);
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
int findNode(struct Trie* trie ,char* str)
{
    if(str[0]=='\0')return 0;
    simplifyName(str);
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
         * '.'     -> 37
        */
        if(str[i]>='0'&&str[i]<='9')
            id = 26 + str[i] - '0';
        else if(str[i]>='a'&&str[i]<='z')
            id = str[i]-'a';
        else if(str[i] == '-')
            id = 36;
        else
            id = 37;

        if(!trie->tree[root][id])return 0;
        root = trie->tree[root][id];
    }
    if(trie->endFlag[root]==false)return 0;
    return root;
}

/*
 * use to delete a word in trie
 * parameter@trie the address of a trie
 * parameter@str the domain name
 * eg: deleteNode(trie,"www.baidu.com");
*/
void deleteNode(struct Trie* trie ,char* str)
{
    if(str[0]=='\0')return;
    simplifyName(str);
    int root = findNode(trie,str);
    if(!root)return;
    trie->endFlag[root] = false;
    //删除节点
    int strNum = strlen(str)-1;
    while(root!=0){
        int id;
        if(str[strNum]>='0'&&str[strNum]<='9')
            id = 26 + str[strNum] - '0';
        else if(str[strNum]>='a'&&str[strNum]<='z')
            id = str[strNum]-'a';
        else if(str[strNum] == '-')
            id = 36;
        else
            id = 37;

        bool haveChild = false;
        for(int i = 0 ; i < maxm ; i++)
        {
            if(trie->tree[root][i]!=0){
                haveChild = true;
                break;
            }
        }
        if(haveChild)break;
        trie->tree[trie->pre[root]][strNum] = 0;
        int tmp = trie->pre[root];
        trie->pre[root] = 0;
        root = tmp;
        strNum--;
    }
}

void transIp(unsigned char ip[4] , char *rowIp)
{
    int ipLen = strlen(rowIp);
    unsigned num = 0;
    int cnt = 0;
    for(int i = 0 ; i <= ipLen; i++)
    {
        if(rowIp[i]=='.'||i == ipLen)
        {
            ip[cnt++] = num;
            num = 0;
        }
        else
        {
            num = num*10 + rowIp[i]-'0';
        }
    }
}
