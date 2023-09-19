#ifndef _HTTP_MANAGEMENT_H_
#define _HTTP_MANAGEMENT_H_


/*管理所有的http_handler*/
#include <mutex>
#include <sys/epoll.h>
#include <thread>
#include <memory>
#include <vector>
#include <fcntl.h>
#include <netinet/in.h>
#include <chrono>
#include <unistd.h>

#include "http_handler.h"
#include "timer.h"


class HttpManagement
{
public:
    static const time_t TIME_OUT = 2;
    /*线程数量*/
    static const int MAX_THREADS_COUNT = 8;
    /*epoll最大监听的事件数量*/
    static const int MAX_EVENTS = 1000;

public:
    HttpManagement(int listen_fd = -1);
    ~HttpManagement() = default;

    /*创建主线程epollfd*/
    bool init();
    bool main_loop();

private:
    /*定时器逻辑*/
    void timer_start(std::map<int, HttpHandler>& handlers, Timer& timer/*, int argc*/);
    /*线程入口函数*/
    void thread_loop(int argc);
    /*往epollfd中注册事件,默认将该fd设置为非阻塞,true表示注册成功*/
    bool add_epoll_fd(int epoll_fd, int sock_fd, int opt, bool is_nonblocking = true);
    /*更改epollfd中注册的事件,默认将fd设置为非阻塞,true表示更改成功*/
    bool mod_epoll_fd(int epoll_fd, int sock_fd, int opt);
    /*往epollfd中移除事件*/
    bool remove_epoll_fd(int epoll_fd, int sock_fd);
    /*将socket设置为非阻塞,true表示添加成功*/
    bool set_nonblock(int sock_fd);
    /*队列中获取fd往epoll中注册事件*/
    bool get_sock_fd(int epoll_fd, std::map<int, HttpHandler>& http_handlers);
    /*在map中删除某个数据*/
    bool pop_handlers(std::map<int, HttpHandler>& http_handlers, int key);

private:
    /*标志是否初始化*/
    bool                                        m_is_init;
    /*listenfd*/
    int                                         m_listen_fd;
    /*子线程数组*/
    std::unique_ptr<std::vector<std::thread> >  m_threads_ptr;
    /*互斥体保证线程安全*/
    std::mutex                                  m_mutex;
    /*cln sock队列*/
    std::list<int>                              m_sock_queue;
    /*管道数组*/
    std::vector<std::unique_ptr<int> >          m_pipes;
};


#endif 