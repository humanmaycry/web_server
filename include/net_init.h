#pragma once
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>


#include "log.h"


namespace m_netinit 
{
    class Net final
    {
    public:
        /*port_reuse设置为true表示开启端口复用,默认开启*/
        /*ip默认为本地ip，端口默认为8080*/
        Net(const char* ip = "127.0.0.1", const char* port = "8080", bool port_reuse = true);
        ~Net();
        /*网络初始化函数*/
        bool initialize();
        /*返回侦听fd*/
        int get_listenfd() const;
    
    private:
        /*开启端口复用*/
        void port_reuse();
        /*打印ip地址和端口号*/
        void print_ip_port() const;
    private:
        /*侦听fd*/
        int                        m_listenfd;
        const char*                m_ip;
        const char*                m_port;   
        bool                       m_port_reuse;
        bool                       m_is_init;
    };

} // namespace net_init

    

