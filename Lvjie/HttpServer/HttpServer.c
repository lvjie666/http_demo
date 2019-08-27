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

//const int port = 8888;        //指定端口号

int create_socket();
int Bind(int port);
int Listen(int server_sock);
void Accept_and_Send(int server_sock);
void Error(char *message);

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

void Accept_and_Send(int server_sock)
{
    struct sockaddr_in client_addr;
    int connfd;
    socklen_t client_addr_length = sizeof(client_addr);  //和int具有相同的长度
    connfd = accept(server_sock,(struct sockaddr*)&client_addr,&client_addr_length); //接受客户端请求
    if(connfd == -1){
        Error("accept() error!\n");
    }

    char request[1024]="";
    ssize_t recvlen = recv(connfd,request,1024,0);
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

    char buf[520]="HTTP/1.1 200 ok\r\nconnection: close\r\n\r\n";   //HTTP响应
    ssize_t sendlen = send(connfd,buf,strlen(buf),0);
    if(sendlen < 0)
    {
        Error("send message is error\n");
    }else if(sendlen == 0){
        Error("send message is falied,connection break\n");
    }
    else{
        printf("send message successful\n");
    }

    int fd = open("hello.txt",O_RDONLY);
    if(fd <= 0){
        Error("open error\n");
    }
    ssize_t sendfile_len = sendfile(connfd,fd,NULL,2500);     //NULL表示读入文件流默认的启示位置，count指定两个文件>描述符之间的字节数
    if(sendfile_len == -1){
        Error("sendfile is failed\n");
    }
    close(fd);
    close(connfd);
}

void Error(char *message)
{
    fprintf(stderr,"%s",message);
    exit(1);
}

int main(int argc,char *argv[])
{
    int server_sock;
    int port = atoi(argv[1]);
    server_sock = Bind(port);
    printf("---------The server is open and waiting for the client to connect.--------------\n");
    while(1){
        Accept_and_Send(server_sock);
        printf("--------------------------------------------------------------------------------\n");
    }
    return 0;
}
