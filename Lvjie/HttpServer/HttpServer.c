#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<string.h>
#include<ctype.h>
#include<pthread.h>
#include<time.h>
const int port = 8888;        //指定端口号

int create_socket();                   //创建socket描述符
int Bind(int port);                    //把地址族中的特定地址赋给socket
int Listen(int server_sock);           //监听socket
void Accept_and_Send(int server_sock); //接收请求并返回请求的东西
void Error(char *message);             //错误消息
void get_url(char *req,int client);    //得到请求中的资源路径
char srcpath[100] ="";                 //全局变量，用来记录资源路径
void src_not_found(int client);        //请求资源路径不存在报错信息
char *get_time();                      //获取系统时间
void judgement();                      //判断服务端是否结束

int main(int argc,char *argv[])
{
    int server_sock;
    int client_sock = -1;
    //int port = atoi(argv[1]);
    server_sock = Bind(port);
    struct sockaddr_in client_addr;
    pthread_t judge_exit;
    pthread_t newthread;
    socklen_t client_addr_length = sizeof(client_addr);  //和int具有相同的长度
    fprintf(stdout,"---------The server is open and waiting for the client to connect.--------------\n");
    fprintf(stdout,"----------------(you can press the enter to close the server.)------------------\n");
//    while(1){
//        Accept_and_Send(server_sock);
//        fprintf(stdout,"--------------------------------------------------------------------------------\n");
//    }
    if(pthread_create(&judge_exit,NULL,judgement,NULL) != 0)
        Error("judge_thread_create error\n");
    while(1){
        client_sock = accept(server_sock,(struct sockaddr*)&client_addr,&client_addr_length); //接受客户端请求
        if(client_sock == -1){
            Error("accept() error!\n");
        }
        if(pthread_create(&newthread,NULL,Accept_and_Send,client_sock) != 0)
            Error("pthread_create error\n");
    }
    close(server_sock);
    return 0;
}
void judgement(){           //用户敲击回车，服务端就断开连接，结束运行
    char c = 0;
    while(c != '\n'){
        c = getchar();
    }
    Error("the server has exit!\n");
}
int create_socket()
{
    int sock;
    if((sock = socket(AF_INET,SOCK_STREAM,0)) == -1){
        Error("socket() error!\n");
    }
    return sock;
}

int Bind(int port)
{
    int server_sock;
    int ret;
    struct sockaddr_in *server_addr;
    server_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in*));
    if(server_addr == NULL){
        Error("malloc() fail!\n");
    }else{
        memset(server_addr,0,sizeof(struct sockaddr_in*));
    }
    server_addr->sin_family = PF_INET;
    server_addr->sin_port = htons(port);
    server_sock = create_socket();
    /*int bind(int socket,const struct sockaddr *addr,socklen_t *addrlen)
     *socket:需要绑定地址的socket
     *addr:需要连接的地址
     *addrlen:地址参数长度
     */
    ret = bind(server_sock,(struct sockaddr *)server_addr,sizeof(struct sockaddr));
    if(ret == -1){
        Error("bind() error!\n");
    }
    Listen(server_sock);
}

int Listen(int server_sock)
{
    int ret = listen(server_sock,5);   //backlog一般小于30,使主动链接变为被动链接，一般在调用bind之后，accept之前调用
    if(ret == -1){
        Error("listen() error!\n");
    }
    return server_sock;
}

void Accept_and_Send(int client_sock)
{
//    struct sockaddr_in client_addr;
//    int connfd;
    struct stat st;
//    socklen_t client_addr_length = sizeof(client_addr);  //和int具有相同的长度
//    connfd = accept(server_sock,(struct sockaddr*)&client_addr,&client_addr_length); //接受客户端请求
//    if(connfd == -1){
//        Error("accept() error!\n");
//    }

    char request[1024]="";
    ssize_t recvlen = recv(client_sock,request,1024,0);
    if(recvlen < 0)
    {
        Error("recive message is error\n");
    }else if(recvlen == 0){
        Error("recive message is falied,connection break\n");
    }
    else{
        printf("the message of request is:\n");
    }
    request[strlen(request)+1] = '\0';
    printf("%s\n",request);
    
    get_url(request,client_sock);
    printf("your request url is %s\n",srcpath);

    int fd = open(srcpath,O_RDONLY);
    if(fd <= 0){
        src_not_found(client_sock);
        fprintf(stdout,"your request information is nonexistent\n");
    }else{
        char *time = get_time();
        char buf[520]="HTTP/1.1 200 ok\r\nContent-Type: text/html\r\nDate:";
        strcat(buf,time);
        strcat(buf,"\r\n\r\n");
        ssize_t sendlen = send(client_sock,buf,strlen(buf),0);
        if(sendlen < 0)
        {
            Error("send_http_header is error\n");
        }else if(sendlen == 0){
            Error("send_http_header is falied,connection break\n");
        }
        else{
            fprintf(stdout,"send_http_header successful\n");
        }

        ssize_t sendfile_len = sendfile(client_sock,fd,NULL,2500);     //NULL表示读入文件流默认的启示位置，count指定两个文件>描述符之间的字节数
        if(sendfile_len == -1){
            Error("sendfile is failed\n");
        }else{
            fprintf(stdout,"send request information successful\n");
        }
    }
    fprintf(stdout,"--------------------------------------------------------------------------------\n");
    close(fd);
    close(client_sock);
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

void get_url(char *req,int client)
{
    int i = 0;
    int j = 0;
    char url[100] = "";
    char *request = req;
    //char path[100] = "";
    struct stat st;
    while(!(isspace(request[i])) && (i < strlen(request))){   //找到第一个空格处
        i++;
    }
    i++;
    while(!(isspace(request[i]))){
        url[j] = request[i];
        i++;
        j++;    
    }
    url[j + 1] = '\0';
    sprintf(srcpath,"documents%s",url);
    if(srcpath[strlen(srcpath) - 1] == '/'){
        strcat(srcpath,"index.html");
    }
    if(stat(srcpath,&st) == -1){
        //src_not_found(client);
        //exit(1);
        fprintf(stderr,"The path is invalid! Please try again!\n");
    }else{
        if((st.st_mode & S_IFMT) == S_IFDIR){
            strcat(srcpath, "/index.html");
        }
    }
}

void src_not_found(int client)
{
    char *time = get_time();
    char buf[1024];
    char buftest[520]="HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n";
    strcat(buftest,time);
    strcat(buftest,"\r\n\r\n");
   // char buftest[520]="HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nDate:Thu Aug 29 13:02:28 2019\r\n\r\n";
    //strcat(buftest,time);
    send(client, buftest, strlen(buftest), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void Error(char *message)
{
    fprintf(stderr,"%s",message);
    exit(1);
}

