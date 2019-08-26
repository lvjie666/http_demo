#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>

int create_tcp_socket();
char *get_ip(char *host);
char *build_get_query(char *host);
//char *build_get_query(char *host,char *page);
void usage();

#define HOST "www.baidu.com"
//#define PAGE "/"
//#define PORT 9999
#define USERAGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.114 Safari/537.36"
#define ACCEPTLANGUAGE "zh-CN,zh;q=0.8,en;q=0.6,en-US;q=0.4,en-GB;q=0.2"
#define ACCEPTENCODING "gzip,deflate,sdch"
int main(int argc,char **argv){
    struct sockaddr_in *remote;
    int sock;
    int tmpres;
    char *ip;
    char *get;
    char buf[BUFSIZ+1];
    char *host;
    //char *page;
    int port;
    FILE *filerecv;

    if(argc!=3)
    {
        usage();
        exit(2);
    }
    host=argv[1];       //ip地址
    port = atoi(argv[2]);   //端口号
    //page=PAGE;
    printf("hostname:%s,port:%d\n",host,port);
    //printf("hostName:%s,port:%d,page:%s\n",ip,port,page);
    sock=create_tcp_socket();
    ip=get_ip(host);
    fprintf(stderr,"IP is %s\n",ip);
    remote=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in*));
    remote->sin_family=AF_INET;
    tmpres=inet_pton(AF_INET,ip,(void *)(&(remote->sin_addr.s_addr)));  //将点分十进制转为二进制
    if(tmpres<0){
        printf("Can't set remote->sin_addr.s_addr");
        exit(1);
    }else if(tmpres==0){
        printf("%s is not a valid IP address\n",ip);
        exit(1);
    }
    remote->sin_port=htons(port);
    if(connect(sock,(struct sockaddr *)remote,sizeof(struct sockaddr))<0){
        perror("Could not connect!\n");
        exit(1);
    }
    get = build_get_query(host);
    //get =build_get_query(host,page);
    printf("发送GET请求\n<start>\n%s\n<end>\n",get);
    int sent=0;
    while(sent<strlen(get)){
        tmpres=send(sock,get+sent,strlen(get)-sent,0);
        if(tmpres==-1){
            perror("Can't send query!");
            exit(1);
        }
        sent+=tmpres;
    }
    memset(buf,0,sizeof(buf));
    int htmlstart=0;    
    char *htmlcontent;
    while((tmpres=recv(sock,buf,BUFSIZ,0))>0){
        if(htmlstart==0){
            htmlcontent=strstr(buf,"\r\n\r\n");
            if(htmlcontent!=NULL){
                htmlstart=1;
                htmlcontent+=4;
            }
        }else{
            htmlcontent=buf;
        }
        if(htmlstart){
            filerecv = fopen("recvice.txt","w+");
            fprintf(filerecv,"%s",htmlcontent);
            fprintf(stdout,htmlcontent);
        }
        memset(buf,0,tmpres);
        //fprintf(stdout,"\n\n\ntmpres Value:%d\n",tmpres);
    }
    fprintf(stdout,"receive data over!\n");
    if(tmpres<0){
        perror("Error receiving data!\n");
    }
    fclose(filerecv);
    free(get);  
    free(remote);
    free(ip);
    close(sock);
    return 0;
}

void usage(){
    fprintf(stderr,"USAGE:example:127.0.0.1 8888\n");
}

int create_tcp_socket(){
    int sock;
    if((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
        perror("Can't create TCP socket!\n");
        exit(1);
    }
    return sock;
}

char *get_ip(char *host){
    struct hostent *hent;
    int iplen=15;
    char *ip=(char *)malloc(iplen+1);
    memset(ip,0,iplen+1);
    if((hent=gethostbyname(host))==NULL){
        perror("Can't get ip");
        exit(1);
    }
    if(inet_ntop(AF_INET,(void *)hent->h_addr_list[0],ip,iplen)==NULL){
        perror("Can't resolve host!\n");
        exit(1);
    }
    return ip;
}

//char *build_get_query(char *host,char *page){
char *build_get_query(char *host){
    char *query;
    //char *getpage=page;
    char *tpl="GET / HTTP/1.1\r\nHost:%s\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent:%s\r\nAccept-Language:%s";//Accept-Encoding:%s\r\n
    query=(char *)malloc(strlen(host)+strlen(USERAGENT)+strlen(tpl)+strlen(ACCEPTLANGUAGE)-5);
    //query=(char *)malloc(strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(tpl)+strlen(ACCEPTLANGUAGE)-5);//+strlen(ACCEPTENCODING)
    sprintf(query,tpl,host,USERAGENT,ACCEPTLANGUAGE);//ACCEPTENCODING
    return query;
}
