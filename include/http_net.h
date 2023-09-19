#ifndef _HTTP_NET_H_
#define _HTTP_NET_H_

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/uio.h>
#include <time.h>


#include "http_enums.h"
#include "log.h"

/*httpserver的网络层，负责数据的收发*/
class HttpNet
{
public:
    /*超时时间缺省时为3秒*/
    HttpNet(time_t timeout = 1);
    ~HttpNet() = default;
    
    /*返回flase表示对端关闭了连接，数据包太大，read函数出错,
    返回投入表示成功收取了数据*/
    bool    read(int read_fd, std::string& read_buf);
    /*返回false需要判断是否需要重新注册可写事件，如果不需要才代表真正的出错*/
    /*只有将buf全部发送才会返回true*/
    bool    write(int write_fd, struct iovec* data, int& iov_count, int& should_write_count);
    bool    is_register_write() const { return m_is_register_write; }
    /*重新将http_net初始化*/
    void    init();

private:
    /*发送数据的超时时间*/
    time_t      m_timeout;
    /*标志是否需要注册可写事件,true为需要注册,false为不需要注册*/
    bool        m_is_register_write;
    
};

#endif