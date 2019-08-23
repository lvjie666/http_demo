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
int main(int argc,char *argv[])
{
    printf("服务端开启\n");
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int server_sock;
    int ret;
    int connfd;
    

    //const char *ip = argv[1];
    int port = atoi(argv[1]);
    bzero(&server_addr,sizeof(server_addr));
    //memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = PF_INET;      //协议族
    server_addr.sin_port = htons(port);    //使用网络字节顺序存储端口号
    //server_addr.sin_addr.s_addr = inet_addr("172.22.211.204");  //按照字节顺序存储胡ip地址
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_sock = socket(AF_INET,SOCK_STREAM,0);   //提供面向连接的稳定数据传输，即TCP
    if(server_sock == -1){
        printf("sock创建失败\n");
        exit(1);
    }
    ret = bind(server_sock, (struct sockaddr*)&server_addr,sizeof(server_addr));  //给sock_server命名
    if(ret == -1){
        printf("bind出错\n");
        exit(1);
    }
    ret = listen(server_sock,5);   //backlog一般小于30,使主动链接变为被动链接，一般在调用bind之后，accept之前调用
    if(ret == -1){
        printf("listen出错\n");
        exit(1);
    }
    printf("--------------------------正在等待客户端连接-------------------------------\n");
    while(1){
        socklen_t client_addr_length = sizeof(client_addr);  //和int具有相同的长度
        connfd = accept(server_sock,(struct sockaddr*)&client_addr,&client_addr_length); //接受客户端连接请求
        if(connfd == -1){
            printf("accept出错\n");
            exit(1);
        }
        else{
            char request[1024];
            recv(connfd,request,1024,0);
            request[strlen(request)+1] = '\0';
            printf("接受到的请求为：\n");
            printf("%s\n",request);
            char buf[520]="HTTP/1.1 200 ok\r\nconnection: close\r\n\r\n";   //HTTP响应
            int s = send(connfd,buf,strlen(buf),0);
            int fd = open("hello.txt",O_RDONLY);
            sendfile(connfd,fd,NULL,2500);     //NULL表示读入文件流默认的启示位置，count指定两个文件描述符之间的字节数
            close(fd);
            close(connfd);
        }
    }
    return 0;
}




