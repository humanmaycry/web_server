

#include "http_management.h"

HttpManagement::HttpManagement(int listen_fd): m_is_init(false),
                        m_listen_fd(listen_fd), m_threads_ptr(nullptr)
{

}

bool HttpManagement::init()
{
    if (m_is_init)
    {
        return true;
    }
    
    /*初始化管道*/
    m_pipes.resize(MAX_THREADS_COUNT);
    for (int i = 0; i < MAX_THREADS_COUNT; i++)
    {
        m_pipes[i].reset(new int[2]);
        if (::pipe(m_pipes[i].get()) == -1)
        {
            LOGD("管道创建失败", "http_management.cpp");
            return false;
        }
    }
    /*创建子loop线程*/
    m_threads_ptr.reset(new std::vector<std::thread>());
    for (int i = 0; i < MAX_THREADS_COUNT; i++)
    {
        m_threads_ptr->push_back(std::thread(&HttpManagement::thread_loop, this, i));
    }


    m_is_init = true;
    return  true;
}

bool HttpManagement::main_loop()
{
    if (!m_is_init)
    {
        return false;
    }

    int epoll_fd = -1;
    if ((epoll_fd = ::epoll_create(5)) == -1)
    {
        LOGF("创建epollfd失败！", "http_management.cpp");
        
    }

    /*监听事件数组*/
    epoll_event events[100];
    /*将listenfd注册读事件，listenfd设置为阻塞的*/
    if (!add_epoll_fd(epoll_fd, m_listen_fd, EPOLLIN, false))
    {
        LOGD("listenfd添加事件失败", "http_management.cpp");
        return false;
    }

    int pipe_number = 0;

    LOGI("主线程开始监听", "http_management.cpp");
    while (true)
    {
        int ret = ::epoll_wait(epoll_fd, events, 100, -1);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                LOGF("主线程epoll_wait函数调用失败", "http_management.cpp");
            }
        }
        

        for (int i = 0; i < ret; i++)
        {
            if (events[i].data.fd == m_listen_fd)
            {
                sockaddr_in   addr;
                socklen_t adr_len = sizeof(addr);
                int cln_sock = accept(m_listen_fd, (struct sockaddr *)&addr, &adr_len);
                printf("cln_sock : [%d]\n", cln_sock);
                if (cln_sock == -1)
                {
                    LOGD("Failed to accept", "http_management.cpp");
                    return false;
                }
                /*TODO:如果成功返回一个有效的fd，则把他放在队列里，并通知工作线程获取*/
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_sock_queue.push_back(cln_sock);
                }
                /*TODO:*/
                /*通过管道通知线程*/
                int sock = m_pipes[pipe_number].get()[1];
                char buf_to_pipe[] = "1";
                ::write(sock, buf_to_pipe, 1);
                LOGD("收到新的连接,向管道发送数据通知子线程获取", "http_management");
            }
        }
        ++pipe_number;
        if (pipe_number >= MAX_THREADS_COUNT)
        {
            pipe_number = 0;
        }
    }
    ::close(m_listen_fd);
    ::close(epoll_fd);
    return true;
}

void HttpManagement::timer_start(std::map<int, HttpHandler>& handlers, Timer& timer/*, int argc*/)
{
    //if (argc == 0)
    timer.time_process(handlers);
}

void HttpManagement::thread_loop(int argc)
{
    bool is_running = true;
    /*http_handler对象*/
    std::map<int, HttpHandler> http_handlers;

    int epoll_fd = ::epoll_create(5);
    if (epoll_fd == -1)
    {
        LOGD("子线程epollfd创建失败", "http_management.cpp");
        is_running = false;
    }
    ::printf("epollfd : [thread:[%d]-[%d]\n", argc, epoll_fd);
    //LOGD("子线程epollfd创建成功", "http_management.cpp");
    epoll_event events[MAX_EVENTS];
    /*将管道注册读事件*/
    if(!add_epoll_fd(epoll_fd, m_pipes[argc].get()[0], EPOLLIN, false))
    {
        LOGD("管道注册失败", "http_management.cpp");
        is_running = false;
    }
    /*TODO:测试定时器问题！*/
    /*准备定时器*/
    Timer timer;
    //
    while (true)
    {
        if (is_running == false)
            break;

        timer_start(http_handlers, timer/*, argc*/);
        int ret = ::epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                LOGD("子线程epoll_wait函数出错", "http_management.cpp");
                break;
            }
        }
        for (int i = 0; i < ret; i++)
        {
            int sock = events[i].data.fd;
            
            /*内核检测到对端关闭连接*/
            if (events[i].events & EPOLLRDHUP)
            {
                remove_epoll_fd(epoll_fd, sock);
                HttpHandler handler = http_handlers[sock];
                handler.close();
                pop_handlers(http_handlers, sock);
                
            }
            else if (sock == m_pipes[argc].get()[0])
            {
                /*如果是从管道发来的信息直接略过*/
                LOGD("接收到来自管道的数据", "http_management.cpp");
                continue;
            }
            else if (events[i].events & EPOLLIN)
            {
                /*TODO:*/
                HttpHandler handler = http_handlers[sock];
                /*对端没有关闭连接，read函数没有出错,read_buf的总长度也没有超过限制*/
                if (handler.read())
                {
                    /*如果收到的数据不完整,则继续注册可读事件*/
                    if (!handler.parse())
                    {
                        continue;
                    }
                    /*如果收到的数据完整,根据解析的情况发送报文,包括请求成功的回复和请求失败的回复*/
                    else
                    {
                        /*TODO:如果是get请求（暂时只有get请求）*/
                        if (handler.write_get())
                        {
                            if (handler.is_keep_alive())
                            {
                                /*添加定时器*/
                                TimeOutHandler t_handler(sock, epoll_fd);
                                timer.add_time_out(t_handler, ::time(NULL) + 10);
                                /*如果是长连接,初始化http_handler,继续处理下一次数据*/
                                handler.init();
                            }
                            else
                            {
                                /*否则关闭连接和删除epoll事件*/
                                if (!remove_epoll_fd(epoll_fd, sock))
                                {
                                    LOGD("取消注册epollfd失败", "http_management.cpp");
                                }
                                handler.close();
                                pop_handlers(http_handlers, sock);
                            }
                        }
                        else
                        {
                            /*判断是否需要重新注册写事件*/
                            if (handler.is_reset_write())
                            {
                                if (!add_epoll_fd(epoll_fd, sock, EPOLLOUT, true))
                                {
                                    LOGD("重新注册写事件失败", "http_management.cpp");
                                    handler.close();
                                    pop_handlers(http_handlers, sock);
                                }
                            }
                            else
                            {
                                /*如果不需要表示发送失败了*/
                                LOGD("数据发送失败", "http_management");
                                remove_epoll_fd(epoll_fd, sock);
                                handler.close();
                                pop_handlers(http_handlers, sock);
                            }
                        }
                    }   
                }
                /*对端关闭了连接或者read函数出错,或者read_buf的长度超出了限制*/
                else
                {
                    remove_epoll_fd(epoll_fd, sock);
                    handler.close();
                    pop_handlers(http_handlers, sock);
                    
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                /*只有未发完数据的http_handler才会处理这个信号,
                暂时只重发get请求数据*/
                HttpHandler handler = http_handlers[sock];
                if(handler.write_get())
                {
                    /*如果发送成功根据connection字段分析是否要关闭连接*/
                    if (handler.is_keep_alive())
                    {
                        handler.init();
                        /*将事件重新注册为读事件*/
                        TimeOutHandler t_handler(handler.get_socket(), epoll_fd);
                        timer.add_time_out(t_handler, ::time(NULL) + 10);
                        if (mod_epoll_fd(epoll_fd, sock, EPOLLIN))
                        {
                            LOGD("写事件触发中重新注册读事件失败", "http_management");
                        }
                    }
                    else
                    {
                        /*否则关闭连接和删除epoll事件*/
                        if (!remove_epoll_fd(epoll_fd, sock))
                        {
                            LOGD("写事件触发中移除事件失败", "http_management");
                        }
                        handler.close();
                        pop_handlers(http_handlers, sock);
                    }     
                }
                else
                {
                    /*判断是否需要重新注册写事件，即数据有没有发完*/
                    if (handler.is_reset_write())
                    {
                        //mod_epoll_fd(epoll_fd, sock, EPOLLOUT);
                        continue;
                    }
                    else
                    {
                        /*表示出错*/
                        remove_epoll_fd(epoll_fd, sock);
                        handler.close();
                        pop_handlers(http_handlers, sock);
                    }
                }
            }
            else
            {
                LOGD("未知信号", "http_management.cpp");
            }
        }
        /*从队列中取出fd注册读事件*/
        get_sock_fd(epoll_fd, http_handlers);

    }
    LOGD("线程退出", "http_management.cpp");
    ::close(epoll_fd);
}

bool HttpManagement::add_epoll_fd(int epoll_fd, int sock_fd, 
                int opt, bool is_nonblocking)
{
    epoll_event ev;
    ev.data.fd = sock_fd;
    if (opt == EPOLLIN)
    {
        ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    }
    else if (opt == EPOLLOUT)
    {
        ev.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
    }

    if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1)
    {
        LOGD("epoll_ctl 返回失败", "http_management.cpp");
        return false;
    }
    if (is_nonblocking)
    {
        return set_nonblock(sock_fd);
    }

    return true;     
}   

bool HttpManagement::mod_epoll_fd(int epoll_fd, int sock_fd, 
                int opt)
{
    epoll_event ev;
    ev.data.fd = sock_fd;
    if (opt == EPOLLIN)
    {
        ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    }
    else if (opt == EPOLLOUT)
    {
        ev.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
    }
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock_fd, &ev) == -1)
    {
        LOGD("修改epoll事件失败", "http_management.cpp");
        return false;
    }

    return true;
}

bool HttpManagement::remove_epoll_fd(int epoll_fd, int sock_fd)
{
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, NULL) == -1)
    {
        LOGD("epoll_ctl删除事件失败", "http_management.cpp");
        return false;
    }
    return true;
}


bool HttpManagement::set_nonblock(int fd)
{
    int old_opt = ::fcntl(fd, F_GETFL);
    if (old_opt == -1)
    {
        LOGD("set socket non-blocking failed", "http_management.cpp");
        return false;
    }
    int new_opt = old_opt | O_NONBLOCK;
    if (::fcntl(fd, F_SETFL, new_opt) == -1)
    {
        LOGD("set socket non-blocking failed", "http_management.cpp");
        return false;
    }
    return true;
}

bool HttpManagement::get_sock_fd(int epoll_fd, std::map<int, HttpHandler>& handlers)
{
    int sock_fd = -1;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_sock_queue.empty())
        {
            sock_fd = m_sock_queue.front();
            m_sock_queue.pop_front();
        }
    }
    if (sock_fd != -1)
    {
        HttpHandler handler(sock_fd);
        /*往handlers中添加handler*/
        handlers[sock_fd] = handler;
        if (!add_epoll_fd(epoll_fd, sock_fd, EPOLLIN, true))
        {
            //remove_epoll_fd(epoll_fd, sock_fd);
            handler.close();
            pop_handlers(handlers, sock_fd);
            LOGD("获取失败", "http_management.cpp");
            return false;
        }
        else
        {   
            LOGD("成功获取一个sockfd", "http_management.cpp");
            return true;
        }
    }
    else
    {
        //std::cout << "socket队列为空" << std::endl;
        return false;
    }
}

bool HttpManagement::pop_handlers(std::map<int, HttpHandler>& http_handlers, int key)
{
    if (http_handlers.erase(key) == 0)
    {
        LOGD("未找到该元素,删除http_handler失败", "http_management.cpp");
        return false;
    } 
    return true;
}