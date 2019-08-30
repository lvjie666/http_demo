#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<netdb.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<time.h>

char *get_time();                               //获取当前系统时间
int create_socket();                            //创建一个表示socket的文件描述符
char *get_ip(char *host);                       //将域名转换为IP地址
int Connect(int port,char *ip);                 //进行三次握手
char *build_get_query(char *host,char *path);   //组装http报头信息
void sendmessage(char *get,int sock);           //发送报文
void usage();                                   //提醒用户输入信息
void Error(char *message);                      //报错信息

#define PAGE "/"
#define USERAGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.114 Safari/537.36"
#define ACCEPTLANGUAGE "zh-CN,zh;q=0.8,en;q=0.6,en-US;q=0.4,en-GB;q=0.2"
const int PORT = 8888;

int main(int argc,char **argv)
{
    int sock;
    char *ip;
    char *get;
    char *host;
    char *path;
    int port;
    if(argc == 1)             //只有./httpclient
    {
        usage();
        exit(2);
    }
    host = argv[1];
    if((strcmp(host,"127.0.0.1") == 0) || (strcmp(host,"localhost") == 0)){  //当访问本地地址时      
        port = 8888;
        if(argc == 2)            //若参数为2个，则采用默认路径
            path = PAGE;
        else if(argc == 3)       //若为三个参数，则寻找用户输入的路径是否存在
            path = argv[2];
        else{                    //否则报错
            usage();
            exit(2);
        }
    }else{                        //访问外网
        port = 80;
        if(argc == 2){            //若2个参数，则采用默认路径
            path = PAGE;
        }else if(argc == 3){
            path = argv[2];
        }else{
            usage();
            exit(2);
        }
    } 
    printf("you input hostName is :%s,port is :%d,path is :%s\n",host,port,path);
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
    int ret1;
    int i = 0;
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
    ret1 = inet_pton(AF_INET,ip,(void *)(&(client_addr->sin_addr.s_addr)));  //将点分十进制转为二进制
    if(ret1 < 0){
        Error("Can't set remote->sin_addr.s_addr");
    }else if(ret1 == 0){
        Error("it is not a valid IP address\n");
    }
    /*int connect(int sockfd,const struct sockaddr* addr,socklen_t addrlen)
     * sockfd:需要连接到服务端的socket
     * addr:服务端地址
     * addrlen:地址长度
     */
    int ret2 = connect(client_sock,(struct sockaddr *)client_addr,sizeof(struct sockaddr));
    while(ret2 < 0){
        i++;
        printf("server can not connect! retry %d times\n",i);
        sleep(3);
        ret2 = connect(client_sock,(struct sockaddr *)client_addr,sizeof(struct sockaddr));
        if(i == 4)
            Error("Could not connect!\n");
    }  //如果首次连接没有成功，则尝试四次后关闭客户端
    free(client_addr);
    return client_sock;
}
void sendmessage(char *get,int sock)
{
    char buf[BUFSIZ + 1];
    FILE *filerecv = NULL;
    //printf("send GET require\n<start>\n%s\n<end>\n",get);
    int flag = 0;
    int ret;
    while(flag < strlen(get)){
        /*ssize_t send(int sock,const void *buf,size_t len,int flags);
         * sock:将要发送数据的socket
         * buf:发送数据的缓冲区
         * len:发送数据的长度
         * flags:一般设置为0
         */
        ret = send(sock,get+flag,strlen(get) - flag,0);
        if(ret == -1){
            Error("Can't send query!");
        }
        flag += ret;
    }
    memset(buf,0,sizeof(buf));
    int htmlstart=0;
    char *htmlcontent;
    char *receive_info;
    fprintf(stdout,"---------------------------receive http_information is:---------------------\n");

    while((flag = recv(sock,buf,BUFSIZ,0)) > 0){
        buf[strlen(buf) + 1] = '\0';
        fprintf(stdout,"%s",buf);                 //输出接收到的内容

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
            fprintf(filerecv,"%s",htmlcontent);   //将接收到的除去报文头后保存至本地
            fclose(filerecv);
            //fprintf(stdout,"%s",htmlcontent);
        }
        memset(buf,0,flag);
    }
    //receive_info[strlen(receive_info) + 1] = '\0';
    fprintf(stdout,"------------------------receive data over!-----------------------------------\n");
    if(flag < 0){
        Error("Error receiving data!\n");
    }
    fclose(filerecv);
    free(get);
}

void usage(){
    fprintf(stdout,"if you want to visit localhost,please input ./HttpClient 127.0.0.1 or ./HttpClient 127.0.0.1 /Lvjie/index.html\n");
    fprintf(stdout,"if you want to visit www,please input ./HttpClient www.baidu.com or ./HttpClient www.baidu.com /index.html\n");
}

int create_socket(){
    int sock;
    /*int socket(int domain,int type,int protocol)
     * domain:协议域，AF_INET IPv4协议
     * type:指定socket类型，SOCK_STREAM即TCP连接
     * protocol:指定某个协议的特定类型
     */
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
    if((hent=gethostbyname(host))==NULL){    //用域名或者主机名获取IP地址
        Error("Can't get ip\n");
    }
    if(inet_ntop(AF_INET,(void *)hent->h_addr_list[0],ip,iplen)==NULL){   //将二进制转换为点分十进制
        Error("Can't resolve host!\n");
    }
    printf("the host ip is:%s\n",ip); 
    return ip;
}

char *get_time(){
    time_t rawtime;
    struct tm * timeinfo;
    char *now_time;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    now_time = asctime(timeinfo);
    now_time[strlen(now_time) - 1] = '\0';
    return now_time;
}

char *build_get_query(char *host,char *path){
    char *query;
    char *getpath = path;
    char *time = get_time();
    char *tpl="GET %s HTTP/1.1\r\nHost:%s\r\nDate:%s\r\nAccept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent:%s\r\nAccept-Language:%s\r\n\r\n";
    query = (char *)malloc(strlen(host) + strlen(getpath) + strlen(time) + strlen(USERAGENT) + strlen(tpl) + strlen(ACCEPTLANGUAGE) - 5);
    if(query == NULL){
        Error("query malloc() fail!\n");
    }else{
        sprintf(query,tpl,getpath,host,time,USERAGENT,ACCEPTLANGUAGE);
        return query;
    }
}

void Error(char *message)
{
    fprintf(stderr,"%s",message);
    exit(1);
}
