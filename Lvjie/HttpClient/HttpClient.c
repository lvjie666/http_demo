#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
int create_socket();
char *get_ip(char *host);
int Connect(int port,char *ip);
char *build_get_query(char *host,char *path);
void sendmessage(char *get,int sock);
void usage();
void Error(char *message);

#define PAGE "/"
#define USERAGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.114 Safari/537.36"
#define ACCEPTLANGUAGE "zh-CN,zh;q=0.8,en;q=0.6,en-US;q=0.4,en-GB;q=0.2"
int main(int argc,char **argv){
    int sock;
    char *ip;
    char *get;
    char *host;
    char *path;
    int port;
    if(argc==1 || argc==2)
    {
        usage();
        exit(2);
    }
    host = argv[1];       //ip地址
    port = atoi(argv[2]);   //端口号
    if(argc == 3){
        path = PAGE;
    }else{
        path = argv[3];      //文件路径
    }
    printf("hostName:%s,port:%d,path:%s\n",host,port,path);
    ip=get_ip(host);
    sock = Connect(port,ip);
    get = build_get_query(host,path);
    sendmessage(get,sock);
    free(ip);
    close(sock);
    return 0;
}

int Connect(int port,char *ip)
{
    int client_sock;
    int ret;
    struct sockaddr_in *client_addr;
    client_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in*));
    if(client_addr == NULL){
        Error("malloc() fail!\n");
    }else{
        memset(client_addr,0,sizeof(struct sockaddr_in*));
    }
    client_addr->sin_family = AF_INET;
    client_addr->sin_port = htons(port);
    client_sock = create_socket();
    ret = inet_pton(AF_INET,ip,(void *)(&(client_addr->sin_addr.s_addr)));  //将点分十进制转为二进制
    if(ret < 0){
        Error("Can't set remote->sin_addr.s_addr");
    }else if(ret == 0){
        Error("it is not a valid IP address\n");
    }
    if(connect(client_sock,(struct sockaddr *)client_addr,sizeof(struct sockaddr)) < 0){
        Error("Could not connect!\n");
    }
    free(client_addr);
    return client_sock;
}
void sendmessage(char *get,int sock)
{
    char buf[BUFSIZ + 1];
    FILE *filerecv = NULL;
    printf("send GET require\n<start>\n%s\n<end>\n",get);
    int flag = 0;
    int ret;
    while(flag < strlen(get)){
        ret = send(sock,get+flag,strlen(get) - flag,0);
        if(ret == -1){
            Error("Can't send query!");
        }
        flag += ret;
    }
    memset(buf,0,sizeof(buf));
    int htmlstart=0;
    char *htmlcontent;
    while((flag = recv(sock,buf,BUFSIZ,0)) > 0){
        if(htmlstart == 0){
            htmlcontent = strstr(buf,"\r\n\r\n");   //定位至服务器发送的内容位置
            if(htmlcontent != NULL){
                htmlstart = 1;
                htmlcontent += 4;
            }
        }else{
            htmlcontent = buf;
        }
        if(htmlstart){
            filerecv = fopen("recvice.txt","w+");
            if(NULL == filerecv){
                Error("open error\n");
            }
            fprintf(filerecv,"%s",htmlcontent);
            fclose(filerecv);
            fprintf(stdout,"%s",htmlcontent);
        }
        memset(buf,0,flag);
    }
    printf("receive data over!\n");
    if(flag < 0){
        Error("Error receiving data!\n");
    }
    fclose(filerecv);
    free(get);
}

void usage(){
    printf("USAGE:example:127.0.0.1 8888\n");
}

int create_socket(){
    int sock;
    if((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
        perror("Can't create TCP socket!\n");
        exit(1);
    }
    return sock;
}

char *get_ip(char *host){                    //进行域名解析，返回ip地址
    struct hostent *hent;
    int iplen=15;
    char *ip=(char *)malloc(iplen+1);
    memset(ip,0,iplen+1);
    if((hent=gethostbyname(host))==NULL){
        Error("Can't get ip");
    }
    if(inet_ntop(AF_INET,(void *)hent->h_addr_list[0],ip,iplen)==NULL){
        Error("Can't resolve host!\n");
    }
    printf("the host's ip is:%s\n",ip); 
    return ip;
}

char *build_get_query(char *host,char *path){
    char *query;
    char *getpath=path;
    char *tpl="GET %s HTTP/1.1\r\nHost:%s\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent:%s\r\nAccept-Language:%s\r\n\r\n";
    query = (char *)malloc(strlen(host)+strlen(getpath)+strlen(USERAGENT)+strlen(tpl)+strlen(ACCEPTLANGUAGE) - 5);
    if(query == NULL){
        Error("query malloc() fail!\n");
    }else{
        sprintf(query,tpl,getpath,host,USERAGENT,ACCEPTLANGUAGE);
        return query;
    }
}

void Error(char *message)
{
    fprintf(stderr,"%s",message);
    exit(1);
}
