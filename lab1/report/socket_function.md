1. `socket()`：创建一个 socket  
`int socket(int domain, int type, int protocol);`  
- `domain`: socket addr type e.g. AF_INET(ipv4), AF_INET6(ipv6)  
- `type`: socket 类型，e.g. SOCK_STREAM(TCP), SOCK_DGRAM(UDP)  
- `protocol`: e.g. IPPROTO_TCP, IPPTOTO_UDP, default when =0

返回一个 socket descripter

2. `bind()`：把 ip地址+端口号赋给 socket  
`int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`  
- `sockfd`: socket descripter
- `addr`: protocol addr
- `addrlen`: addr length

```c++
struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
struct in_addr {
        union {
                struct { Uunsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { unsigned short s_w1,s_w2; } S_un_w;
                unsigned long S_addr;
        } S_un;
}；
```

server 会调用 `bind()`，server 就可以通过它连接；后者不调用，在 `connect()` 时随机生成一个

3. `listen(), connect()`  
`int listen(int sockfd, int backlog);`  
- `backlog`: socket 可以排队的最大连接个数  
`int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`  

server 调用 `listen()` 用于监听，如果 client 调用 `connect()` 发出连接请求，前者就会收到  
成功返回0，出错返回-1

4. `accept()`  
`int accept(int sockfd, struct sockaddr *addr, socklen_t addrlen);`  

server 监听到 client 的请求之后，调用 `accept()` 接受请求，建立连接  
注意：参数 `sockfd` 是 server 的监听 socket descripter，返回的是已经连接的 socket descripter；前者只有一个，后者是对每个进程接受的 client 连接都创建一个，完成对该 client 的服务后就关闭  

5. `read(), write()`  
- `read(), write()`
- `readv(), writev()`
- `recvmsg(), sendmsg()`
- `recv(), send()`  
  - `int recv(int sock, char *buf, int len, int flags);`
  - `int send(int sock, const char *buf, int len, int flags);`
- `recvfrom(), sendto()`
  - `int recvfrom(int sock, const char *buf, int len, int flags, const struct sockaddr* from, int* fromlen)`
  - `int sendto(int sock, const char *buf, int len, int flags, const struct sockaddr* to, int tolen)`

返回实际发送/接收的字节数，<0 是报错

6. `close()`  
`int close(int fd);`  

`close()` 是使相应 socket descripter 的引用计数-1，只有计数 =0 时，才会触发 client 向 server 发出终止连接请求