#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#define MAXLINE 4906

// 方法必须要先声明 https://stackoverflow.com/questions/32703675/implicit-declaration-of-function-enterchar-wimplicit-function
int sendHTML(int connectID);

int main(int argc, char** argv) {
    int    socketID, connfd;
    /**
     struct sockaddr_in {
        __uint8_t       sin_len;
        sa_family_t     sin_family;
        in_port_t       sin_port;
        struct  in_addr sin_addr;
        char            sin_zero[8];
    }
     */
    struct sockaddr_in     servaddr;
    char    buff[MAXLINE]; // c语言中字符串需要指定长度
    int     n;
    int listenPort = 6666;

    if (argc >= 2) {
        listenPort = atoi(argv[1]);
    }

    // socket函数接受三个int类型参数，返回int结果
    // socket编程常用函数及参数 https://blog.csdn.net/neverbefat/article/details/75810138
    socketID = socket(AF_INET, SOCK_STREAM, 0);
    /**
     * errno 是记录系统的最后一次错误代码。
     * 代码是一个int型的值，在errno.h中定义。
     * 查看错误代码errno是调试程序的一个重要方法。
     * 当linux C api函数发生异常时,一般会将errno变量(需include errno.h)赋一个整数值,
     * 不同的值表示不同的含义,可以通过查看该值推测出错的原因。
     */
    if( socketID == -1 ){
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    // 快速清空servaddr sizeof返回长度 https://blog.csdn.net/PengPengBlog/article/details/52593353
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET; // ipv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 指定网卡
    servaddr.sin_port = htons(listenPort); // 绑定指定端口

    // scoket函数只是指定协议，bind函数用来调用让系统为socketID分配地址，什么地址呢？
    if( bind(socketID, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    // 10
    /**
     * int listen(int sockfd, int backlog);
        socket绑定地址之后，默认是想要去连接的，liste会将socket的主动连接的默认属性改为被动连接属性。
        然后开始监听可能的连接请求，但是这只能在有可靠数据流保证的时候(TCP)使用,如SOCK_STREAM和SOCK_SEQPACKET
        sockfd 使用socke创建的描述符
        backlog 监听队列的大小，每当有一个连接请求到来，就会进入此监听队列。
            当一个请求被accept()接受，这个请求就会被移除对列。
        当队列满后，新的连接请求就会返回错误，一般发生在同时有大量请求到来，服务器处理不过来的时候。
     */
    if( listen(socketID, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    printf("======waiting for client's request http://localhost:%i ======\n", listenPort);
    while (1) {
        /**
         * 当应用程序接收到来自其它主机的面对数据流的连接时，通过事件通知它(比如unix的 select()系统调用)。
         * 必须用accept()初始化连接。accept为每个连接建立新的套接字，并从监听队列中移除这个连接。
            int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
                sockefd 监听的套接字描述符
                addr 客户端地址结构体信息，一个指向sockaddr的指针
                addrlen客户端地址结构体的大小
         */
        connfd = accept(socketID, (struct sockaddr*)NULL, NULL);
        if (connfd == -1) {
            printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
            continue;
        }

        /**
         * https://www.cnblogs.com/jianqiang2010/archive/2010/08/20/1804598.html
         * int recv( SOCKET s, char FAR *buf, int len, int flags);
            不论是客户还是服务器应用程序都用recv函数从TCP连接的另一端接收数据。该函数的第一个参数指定接收端套接字描述符；
            第二个参数指明一个缓冲区，该缓冲区用来存放recv函数接收到的数据；
            第三个参数指明buf的长度；
            第四个参数一般置0。
            这里只描述同步Socket的recv函数的执行流程。当应用程序调用recv函数时，
            （1）recv先等待s的发送缓冲中的数据被协议传送完毕，如果协议在传送s的发送缓冲中的数据时出现网络错误，那么recv函数返回SOCKET_ERROR，
            （2）如果s的发送缓冲中没有数据或者数据被协议成功发送完毕后，recv先检查套接字s的接收缓冲区，
            如果s接收缓冲区中没有数据或者协议正在接收数据，那么recv就一直等待，直到协议把数据接收完毕。
            当协议把数据接收完毕，recv函数就把s的接收缓冲中的数据copy到buf中（注意协议接收到的数据可能大于buf的长度，
            所以 在这种情况下要调用几次recv函数才能把s的接收缓冲中的数据copy完。
            recv函数仅仅是copy数据，真正的接收数据是协议来完成的），
            recv函数返回其实际copy的字节数。如果recv在copy时出错，那么它返回SOCKET_ERROR；如果recv函数在等待协议接收数据时网络中断了，那么它返回0。
        注意：在Unix系统下，如果recv函数在等待协议接收数据时网络断开了，那么调用recv的进程会接收到一个SIGPIPE信号，进程对该信号的默认处理是进程终止。
         */
        n = recv(connfd, buff, MAXLINE, 0);
        // n表示buff的实际长度，通过赋值\0标识字符串的实际终结位置 https://blog.csdn.net/ljss321/article/details/51195125
        // buff长度有限制，这里面还牵扯到压缩/解码等操作，其实很复杂，不过就简单的服务器来说也就这样子了
        // buff表示服务端收到的数据
        // 可以打印一下post请求处理大文件的情况
        buff[n] = '\0';
        printf("recv msg from client:\n%s\n", buff);
        sendHTML(connfd);
        // 关闭单条连接
        close(connfd);
    }

    // 关闭socket
    close(socketID);
}

int sendHTML(int connectID) {
    char sendC[1000];
    char headerContentLen[255];
    char content[100] = "<h1>hello world</h1>\0";
    strcpy(sendC, "HTTP/1.1 200 OK\n");
    strcat(sendC, "Content-Type: text/html\n");
    // 需要指定content-length
    sprintf(headerContentLen, "Content-Length: %lu\n", strlen(content));
    strcat(sendC, headerContentLen);
    strcat(sendC, "\n");
    strcat(sendC, content);
    send(connectID, sendC, sizeof(sendC), 0);
    return 0;
}
