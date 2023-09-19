#ifndef _HTTP_HANDLER_H
#define _HTTP_HANDLER_H

#include <string>
#include <sys/uio.h>
#include <string.h>

#include "http_enums.h"
#include "http_net.h"
#include "http_session.h"

class HttpHandler
{
public:
    HttpHandler(int cln_sock = -1);
    ~HttpHandler() = default;


    /*收取数据*/
    /*返回flase表示对端关闭了连接，数据包太大，read函数出错,
    返回true表示成功收取了数据*/
    bool read();
    /*TODO:暂时只考虑get请求*/
    /*发送get请求的数据,返回flase需要判断是否需要注册可写事件,如果不需要表示发送出错
    反之表示数据为全部发送*/
    bool write_get();
    /*协议的解析, 返回解析后的结果,true表示报文完整,false表示报文不完整*/
    bool parse();
    /*返回请求方法*/
    const std::string& get_method() const;
    /*返回是否需要长连接*/
    bool is_keep_alive();
    /*重新将HttpHandler初始化,主要用于keep-alive场景*/
    void init();
    /*关闭socket*/
    void close();
    /*是否需要重新注册写事件*/
    bool is_reset_write() const;
    /*返回socket*/
    int get_socket() const;

private:
    /*读写数据的socket*/
    int                     m_rw_socket;
    /*读取缓冲区*/
    std::string             m_read_buffer;
    
    /*使用iovec结构体一并发送数据(包括http应答和资源文件)*/
    iovec                   m_w_iovec[2];
    /*待发送iovec数组的长度*/
    int                     m_len_iovec;
    /*应当发送的字节数*/
    int                     m_should_write;

private:
    /*网络层*/
    HttpNet                 m_http_net;
    /*业务层*/
    HttpSession             m_http_session;
};

#endif