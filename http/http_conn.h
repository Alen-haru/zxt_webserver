#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;//读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;//写缓冲区的大小
    //HTTP请求方法，
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    //解析客户端请求时，主状态机的状态
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,//当前正在分析请求行
        CHECK_STATE_HEADER,//当前正在分析头部字段
        CHECK_STATE_CONTENT//当前正在解析请求体
    };
    //服务器处理HTTP请求的可能结果，报文解析的结果
    enum HTTP_CODE
    {
        NO_REQUEST,//请求不完整，需要继续读取客户数据
        GET_REQUEST,//表示获得了一个完成的客户请求
        BAD_REQUEST,//表示客户请求语法错误
        NO_RESOURCE,//表示服务器没有资源
        FORBIDDEN_REQUEST,//表示客户对资源没有充足的访问权限
        FILE_REQUEST,//文件请求，获取文件成功
        INTERNAL_ERROR,//表示服务器内部错误
        CLOSED_CONNECTION//表示客户端已经关闭连接了
    };
    //从状态机的三种可能状态，即行的读取状态，分别表示
    enum LINE_STATUS
    {
        LINE_OK = 0,//读取到一个完整的行
        LINE_BAD,//行出错
        LINE_OPEN//行数据尚且不完整
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    //处理客户端请求
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv;


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;//所有的socket上的事件被注册到同一个epoll对象中
    static int m_user_count;//统计用户数量
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:
    int m_sockfd;//该HTTP连接的socket
    sockaddr_in m_address;//通信的socket地址
    char m_read_buf[READ_BUFFER_SIZE];//读缓冲区
    long m_read_idx;//标识读缓冲区中以及读入的客户端数据的最后一个字节的下一个位置
    long m_checked_idx;//当前正在分析的字符在读缓冲区的位置
    int m_start_line;//当前正在解析的行的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;//主状态机当前所处的状态
    METHOD m_method;//请求方法
    char m_real_file[FILENAME_LEN];//客户请求的目标文件的完整路径，其内容等于doc_root+m_url,doc_root是网址根目录
    char *m_url;//请求目标文件的文件名
    char *m_version;//协议版本，只支持HTTP1.1
    char *m_host;//主机名
    long m_content_length;
    bool m_linger;//HTTP请求是否要保持连接
    char *m_file_address;//客户请求的目标文件被mmap到内存中的起始位置
    struct stat m_file_stat;
    struct iovec m_iv[2];//iovec类型包括：内存起始位置，内存长度；采用writev来执行写操作，所以下定义下面两个成员
    int m_iv_count;//表示被写内存块的数量
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;//将要发送的字节
    int bytes_have_send;//已经发送的数据
    char *doc_root;//网站的根目录

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
