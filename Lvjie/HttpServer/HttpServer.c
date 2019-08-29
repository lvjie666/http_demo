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
//const int port = 8888;        //指定端口号

int create_socket();
int Bind(int port);
int Listen(int server_sock);
void Accept_and_Send(int server_sock);
void Error(char *message);
void get_url(char *req,int client);
char srcpath[100] ="";           //record the url path
void src_not_found(int client);
char *get_time();

int main(int argc,char *argv[])
{
    int server_sock;
    int client_sock;
    int port = atoi(argv[1]);
    server_sock = Bind(port);
    struct sockaddr_in client_addr;
    pthread_t newthread;
    socklen_t client_addr_length = sizeof(client_addr);  //和int具有相同的长度
    fprintf(stdout,"---------The server is open and waiting for the client to connect.--------------\n");
//    while(1){
//        Accept_and_Send(server_sock);
//        fprintf(stdout,"--------------------------------------------------------------------------------\n");
//    }
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

