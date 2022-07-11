#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define SERV_ADR "127.0.0.1"
#define PORT 10000

pthread_t read_tid;
pthread_t write_tid;
FILE *read_fp;
FILE *write_fp;

////////////////////函数声明////////////////////
/**
 * @brief 显示错误信息
 * 
 */
void error_handler(char *error_msg);

/**
 * @brief 初始化服务器地址信息
 * 
 */
void server_addr_init(struct sockaddr_in *serv_adr, char *addr, int port);

/**
 * @brief 判断是否要断开连接
 * 
 * @return 如果要断开连接则返回1，否则返回0
 */
int is_saying_bye(char *message);

/**
 * @brief “读”线程的执行函数
 * 
 */
void *clnt_receiver(void *arg);

/**
 * @brief “写”线程的执行函数
 * 
 */
void *clnt_sender(void *arg);

/**
 * @brief 与服务器通信，创建两个线程分别负责接收和发送消息
 * 
 */
void communicate_with_server();

////////////////////函数定义////////////////////
void error_handler(char *error_msg)
{
    fprintf(stderr, "%s error.\n", error_msg);
    exit(EXIT_FAILURE);
}

void server_addr_init(struct sockaddr_in *serv_adr, char *addr, int port)
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

void *clnt_receiver(void *arg)
{
    char message[BUF_SIZE];

    while(1)
    {
        if(fgets(message, BUF_SIZE, read_fp) != NULL)
        {
            printf("%s", message);
        }
    }
    
    return NULL;
}

void *clnt_sender(void *arg)
{
    char message[BUF_SIZE];
    int err;

    printf("Enter name: ");
    fgets(message, BUF_SIZE, stdin);
    fputs(message, write_fp);
    fflush(write_fp);

    while(1)
    {
        printf("Enter message(BYE to exit):\n");

        fgets(message, BUF_SIZE, stdin);
        fputs(message, write_fp);
        fflush(write_fp);
        if(is_saying_bye(message))
        {
            err = pthread_cancel(read_tid);
            if(err != 0)
            {
                error_handler("pthread_cancel()");
            }

            fclose(read_fp);
            fclose(write_fp);

            break;
        }
    }

    printf("Connection shut down\n");

    err = pthread_join(read_tid, NULL);
    if(err != 0)
    {
        error_handler("pthread_join()");
    }

    return NULL;
}

void communicate_with_server()
{
    int err;

    err = pthread_create(&write_tid, NULL, clnt_sender, NULL);
    if(err != 0)
    {
        error_handler("pthread_create()");
    }

    err = pthread_create(&read_tid, NULL, clnt_receiver, NULL);
    if(err != 0)
    {
        error_handler("pthread_create()");
    }

    err = pthread_join(write_tid, NULL);
    if(err != 0)
    {
        error_handler("pthread_join()");
    }

}

////////////////////主函数入口////////////////////
int main(int argc, char *argv[])
{
    int clnt_sock;
    struct sockaddr_in serv_adr;
    

    int err;

    //创建套接字
    clnt_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(clnt_sock == -1)
    {
        error_handler("socket()");
    }
    //初始化服务器地址信息
    server_addr_init(&serv_adr, SERV_ADR, PORT);
    //建立连接
    err = connect(clnt_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    if(err == -1)
    {
        error_handler("connect()");
    }
    //将线程设置为可立即取消状态
    err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if(err != 0)
    {
        error_handler("pthread_setcancelstate()");
    }

    err = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if(err != 0)
    {
        error_handler("pthread_setcanceltype()");
    }
    //与服务器通信
    read_fp = fdopen(clnt_sock, "r");
    write_fp = fdopen(dup(clnt_sock), "w");

    communicate_with_server();

    return 0;
}