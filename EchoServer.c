#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define BUF_SIZE 1024
#define SERV_ADR "127.0.0.1"
#define PORT 10000
#define MAX_CLNT 5
#define NAME_LEN 10

int listen_sock, conn_sock;
struct sockaddr_in serv_adr;
struct sockaddr_in clnt_adr;
socklen_t clnt_adr_sz;

typedef struct {
    char *name;
    FILE *read_fp;
    FILE *write_fp;
} ClntInfo;

struct pollfd clnts[FOPEN_MAX];
ClntInfo clnts_info[FOPEN_MAX];
int maxi;
int nready;

////////////////////函数声明////////////////////
/**
 * @brief 显示错误信息
 * 
 */
void error_handler(char *error_msg);

/**
 * @brief 初始化服务器地址并绑定套接字
 * 
 */
void server_addr_init(struct sockaddr_in *erv_adr, char *addr, int port, int serv_sock);

/**
 * @brief 初始化描述符数组和客户端信息数组
 * 
 */
void clnt_array_init();

/**
 * @brief 将客户端加入客户端数组
 * 
 */
void add_clnt_sock(int clnt_sock);

/**
 * @brief 将客户端从数组中移除，并关闭文件指针
 * 
 */
void remove_clnt_sock(int index);

/**
 * @brief 向除了当前客户端以外的其他客户端发送消息
 * 
 */
void send_message_to_clnts(char *message, int clnt_sock);

/**
 * @brief 判断是否需要断开连接
 * 
 * @return 如果是则返回1，否则返回0
 */
int is_saying_bye(char *message);

/**
 * @brief 客户端读写的执行函数
 * 
 */
void clnt_handler(int index);

////////////////////函数定义////////////////////
void error_handler(char *error_msg)
{
    fprintf(stderr, "%s error.\n", error_msg);
    exit(EXIT_FAILURE);
}

void server_addr_init(struct sockaddr_in *serv_adr, char *addr, int port, int serv_sock)
{
    int err;

    memset(serv_adr, 0, sizeof(*serv_adr));
    serv_adr->sin_family = AF_INET;
    serv_adr->sin_port = htons(port);
    err = inet_pton(AF_INET, addr, &(serv_adr->sin_addr));
    if(err != 1)
    {
        error_handler("inet_pton()");
    }

    err = bind(serv_sock, (struct sockaddr *)serv_adr, sizeof(*serv_adr));
    if(err == -1)
    {
        error_handler("bind()");
    }
}

void clnt_array_init()
{
    clnts[0].fd = listen_sock;
    clnts[0].events = POLLRDNORM;

    for(int i = 1; i < FOPEN_MAX; i++)
    {
        clnts[i].fd = -1;
        clnts_info[i].name = NULL;
    }

    maxi = 0;
}

void add_clnt_sock(int clnt_sock)
{
    int i;

    for(i = 1; i < FOPEN_MAX  ; i++)
    {
        if(clnts[i].fd < 0)
        {
            clnts[i].fd = clnt_sock;
            clnts_info[i].read_fp = fdopen(clnt_sock, "r");
            clnts_info[i].write_fp = fdopen(clnt_sock, "w");

            break;
        }
    }

    clnts[i].events = POLLRDNORM;

    if(i > maxi)
    {
        maxi = i;
    }
}

void remove_clnt_sock(int index)
{
    fclose(clnts_info[index].read_fp);
    fclose(clnts_info[index].write_fp);

    free(clnts_info[index].name);
    clnts_info[index].name = NULL;

    clnts[index].fd = -1;
}

void send_message_to_clnts(char *message, int clnt_sock)
{
    for(int i = 1; i < FOPEN_MAX; i++)
    {
        if(clnts[i].fd > 0 && clnts[i].fd != clnt_sock)
        {
            fputs(message, clnts_info[i].write_fp);
            fflush(clnts_info[i].write_fp);
        }
    }
}

int is_saying_bye(char *message)
{
    if(strcmp(message, "BYE\n") == 0)
    {
        return 1;
    } else
    {
        return 0;
    }
}

void clnt_handler(int index)
{
    char read_message[BUF_SIZE];
    char send_message[BUF_SIZE];
    char *name;

    //从客户端获得消息并发送给其他客户端
    fgets(read_message, BUF_SIZE, clnts_info[index].read_fp);
    if(clnts_info[index].name == NULL)
    {
        name = strtok(read_message, "\n");
        clnts_info[index].name = (char *)calloc(NAME_LEN + 1, sizeof(char));
        strcpy(clnts_info[index].name, name);

        snprintf(send_message ,BUF_SIZE, "%s has joind\n", clnts_info[index].name);
    } else
    {
        snprintf(send_message, BUF_SIZE, "Message from %s: %s", clnts_info[index].name, read_message);
    }
    
    printf("%s", send_message);
    send_message_to_clnts(send_message, clnts[index].fd);

    if(is_saying_bye(read_message))
    {
        printf("Closing down connection ...\n");
        remove_clnt_sock(index);
    }
}

////////////////////主函数入口////////////////////
int main(int argc, char *argv[])
{
    int err;

    //创建套接字
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock == -1)
    {
        error_handler("socket()");
    }
    //初始化服务器地址并绑定套接字
    server_addr_init(&serv_adr, SERV_ADR, PORT, listen_sock);
    //监听连接
    err = listen(listen_sock, MAX_CLNT);
    if(err != 0)
    {
        error_handler("listen()");
    }
    //初始化监听集和客户端数组
    clnt_array_init();

    //接受新连接
    clnt_adr_sz = sizeof(clnt_adr);
    printf("Listening for connection ...\n");
    while(1)
    {
        nready = poll(clnts, maxi + 1, -1);

        if(nready == -1)
        {
            error_handler("poll()");
        }
        
        if(clnts[0].revents & POLLRDNORM)
        {
            conn_sock = accept(listen_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
            printf("New client accepted\n");
            add_clnt_sock(conn_sock);
            printf("Connection successful\n");
            printf("Listening for input ...\n");
            printf("Listening for connection ...\n");

            if(--nready <= 0)
            {
                continue;
            }
        }

        for(int i = 1; i <= maxi; i++)
        {
            if(clnts[i].fd < 0)
            {
                continue;
            }

            if(clnts[i].revents & POLLRDNORM)
            {
                clnt_handler(i);
                
                if(--nready <= 0)
                {
                    break;
                }
            }
        }
    }

    return 0;
}