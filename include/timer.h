#ifndef _TIMER_H
#define _TIMER_H


#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include <vector>
#include <time.h>
#include <map>
#include <set>
//#include <utility>
// 

#include "http_handler.h"
#include "log.h"

#define MAP
#ifdef PILER
//#define PILER

namespace m_timer
{
    struct client_data;
    /*用户数据包括socket， epollfd*/
    struct client_data
    {   
        int m_cln_sock = -1;
        int m_epoll_fd = -1;
    };

    class Timer
    {
    public:

        typedef bool (*time_out_func)(client_data& data);
        /*构造函数*/
        Timer(client_data& cln_data, time_out_func time_out_handler, time_t time_out);
        /*测试调用函数*/
        void time_out_handler();
        /*返回定时时间*/
        time_t get_time_out() const;
        /*返回socket*/
        int get_sock() const { return m_client_data.m_cln_sock; }

    private:
        //typedef void (*time_out_func)(int epfd, int clnfd);
        /*用户数据*/
        client_data                 m_client_data;
        /*超时处理函数*/
        time_out_func               m_time_out_handler;
        /*设置超时时间*/
        time_t                      m_time_out;

    };

    /*时间堆*/
    class Timepiler
    {
    public:
        const static std::size_t MAX_TIMER = 1024;


    public:
        Timepiler();
        /*添加定时器类*/
        bool                add_timer(const Timer& timer);
        /*判断最小堆是否为空*/
        bool                is_empty() const;
        /*删除定时器*/
        bool                pop_timer(int root = 0);
        /*返回根节点的定时时间*/
        time_t              get_root_time() const;
        /*时间堆执行超时函数*/
        void                timer_process(time_t time_out, std::map<int, HttpHandler> &http_handlers);
        /*按照存储顺序打印时间堆的元素定时时间*/
        void                ergodic_print();
        /*返回时间堆的元素个数*/
        std::size_t         get_size() const;

    private:
        /*最小堆的上虑操作*/
        bool    upper();
        /*最小堆的下滤操作*/
        bool    lower(std::size_t ele_index = 0);

    private:
        /*使用一维数组模拟最小堆*/
        std::vector<Timer>          m_piler;
        const std::size_t           m_max_timer;
    };
}
#endif

#ifdef MAP

class TimeOutHandler
{
public:
    TimeOutHandler(int sock = -1, int epollfd = -1);
    void handler_func(std::map<int, HttpHandler>& handlers);
    int get_socket() const  { return m_sock; }
    int get_epollfd() const  { return m_epollfd; }
private:
    int m_sock;
    int m_epollfd;
};

class Timer
{
public:
    void add_time_out(const TimeOutHandler& handler, time_t timeout);
    void remove_time_out(int sock);
    void time_process(std::map<int, HttpHandler>& handlers);
    /*返回根结点的定时时间,map为空则返回0*/
    time_t get_root_time() const;
private:
    /*判断socket是否重复, 如果重复则更新超时时间,true表示重复,false表示不重复*/
    bool is_sock_repeat(const TimeOutHandler& handler, time_t timout) ;
    /*获取根节点的定时时间*/


private:
    std::map<int, time_t> m_socks;
    std::multimap<time_t, TimeOutHandler>  m_timers;
};

#endif

#endif
