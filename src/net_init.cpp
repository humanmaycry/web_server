




#include "net_init.h"




/*port_reuse设置为true表示开启端口复用*/
m_netinit::Net::Net(const char* ip, const char* port, bool port_reuse):
                    m_listenfd(-1), m_ip(ip),
                    m_port(port), m_port_reuse(port_reuse),
                    m_is_init(false)
{
    
}

m_netinit::Net::~Net()
{
    m_is_init = false;
    close(m_listenfd);
}


/*网络初始化函数*/
bool m_netinit::Net::initialize()
{
    if (m_is_init)
    {
        LOGI ("网络已经初始化了！", "net_init.cpp");
        return false;
    }

    if (m_ip == NULL || m_port == NULL)
    {
        print_ip_port();
        LOGI("ip地址或者端口号错误", "net_init.cpp");
        return false;
    }

    if ((m_listenfd = ::socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        LOGI("socket error", "net_init.cpp");
        return false;
    }

    /*设置ip地址，端口号，协议族*/
    sockaddr_in ser_addr;
    ::memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(m_ip);
    ser_addr.sin_port = htons(atoi(m_port));

    /*绑定指定端口号*/
    if (::bind(m_listenfd, (sockaddr*)&ser_addr, sizeof(ser_addr)) != 0)
    {
        LOGF("bind() error", "net_init.cpp");
        return false;
    }

    /*是否开启端口复用*/
    if (m_port_reuse)
    {
        port_reuse();
        LOGI("启动端口复用", "net_init.cpp");
    }
    else
    {
        LOGI("没有启动端口复用", "net_init.cpp");
    }

    /*开启监听*/
    if (::listen(m_listenfd, 10) != 0)
    {
        LOGI("listen() error", "net_init.cpp");
        return false;
    }
    
    /*设置启动成功标志*/
    m_is_init = true;
    return true;
}

int m_netinit::Net::get_listenfd() const
{
    if (m_is_init)
    {
        return m_listenfd;
    }
    else
    {
        LOGI("网络为初始化，返回监听socket失败", "net_init.cpp");
        return -1;
    }
}

void m_netinit::Net::print_ip_port() const
{
    printf("ip:[%s] port:[%s]\n", m_ip, m_port);
}

void m_netinit::Net::port_reuse()
{
    int opt = 1;
    if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) != 0)
    {
        LOGF("port reuse error", "net_init.cpp");
    }
}