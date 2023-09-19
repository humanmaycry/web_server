#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>


#include "timer.h"
#include "log.h"

#ifdef PILER
m_timer::Timer::Timer(client_data& cln_data, time_out_func time_out_handler, 
                            time_t time_out):
                        m_client_data(cln_data), m_time_out_handler(time_out_handler),
                        m_time_out(::time(NULL) + time_out)
{

}


void m_timer::Timer::time_out_handler()
{
    std::cout << "定时时间：" << m_time_out << std::endl;
    m_time_out_handler(m_client_data);
}

time_t m_timer::Timer::get_time_out() const
{
    return m_time_out;
}


m_timer::Timepiler::Timepiler():m_max_timer(MAX_TIMER)
{

}

bool m_timer::Timepiler::add_timer(const m_timer::Timer& timer)
{
    if (m_piler.size() >= m_max_timer)
    {
        /*最小堆满，添加失败*/
        return false;
    }
    m_piler.push_back(timer);
    /*将添加新的定时器后的最小堆进行上滤操作，保证新的vector容器元素排列顺序满足最小堆*/
    upper();
    return true;
}

bool m_timer::Timepiler::is_empty() const
{
    return m_piler.empty();
}

bool m_timer::Timepiler::pop_timer(int root)
{
    if(is_empty())
    {
        /*如果堆为空，则删除失败，返回false*/
        return false;
    }
    /*将数组最后一个元素置为第一个即根结点，然后将该元素进行下滤操作*/
    int last_index = m_piler.size() - 1;
    m_piler[0] = m_piler[last_index];
    /*将最后一个元素删除*/
    m_piler.pop_back();
    /*释放多余的内存*/
    //m_piler.shrink_to_fit();
    /*将第一个元素进行下滤操作*/
    lower();
    return true;
}

time_t m_timer::Timepiler::get_root_time() const
{
    /*如果最小堆为空则返回0*/
    return is_empty() ? 0 : m_piler[0].get_time_out();
}

void m_timer::Timepiler::timer_process(time_t time_out, std::map<int, 
                                    HttpHandler>& handlers)
{   
    time_t cond = get_root_time();
    while (cond !=0 && cond <= time_out)
    {
        try{
            m_piler[0].time_out_handler();

        } catch (const char* err)
        {
            throw err;
        }
        if(handlers.erase(m_piler[0].get_sock()) == 0)
        {
            LOGD("定时器删除http_handler失败", "timer.cpp");
        }
        pop_timer();
        LOGD("删除了一个定时器", "timer.cpp");
        cond = get_root_time();
    }
}

void m_timer::Timepiler::ergodic_print() 
{   
    if (is_empty())
    {
        std::cout << "timer_piler is empty" << std::endl;
        return ;
    }
    for (auto x : m_piler)
        std::cout << x.get_time_out() << " ";
    std::cout << std::endl;
}

std::size_t m_timer::Timepiler::get_size() const
{
    return m_piler.size();
}

bool m_timer::Timepiler::upper()
{
    if (m_piler.size() > m_max_timer)
    {
        /*如果堆元素数量超过了定时器最大数量则直接返回*/
        //throw std::exception();
        return false;
    }
    
    int temp   = m_piler.size() - 1;
    int parent = 0;

    //Timer m_timer = m_piler[temp];

    for ( ; (temp - 1) / 2 >= 0; temp = parent)
    {
        parent = (temp - 1) / 2;
        if (m_piler[temp].get_time_out() >= m_piler[parent].get_time_out())
        {
            break;
        }
        else
        {
            //m_piler[temp] = m_piler[parent];
            std::swap(m_piler[temp], m_piler[parent]);
        }
    }
    //m_piler[temp] = m_timer;
    return true;
}

bool m_timer::Timepiler::lower(std::size_t ele_index)
{
    if (is_empty())
    {
        return false;
    }

    /*初始化子结点下标*/
    std::size_t son = 0;
    for( ; (ele_index * 2 + 1) <= (m_piler.size() - 1); ele_index = son)
    {
        son = ele_index * 2 + 1;
        if ((son + 1) <= (m_piler.size() - 1) && m_piler[son + 1].get_time_out()< 
                            m_piler[son].get_time_out())
        {
            ++son;
        }
        if (m_piler[ele_index].get_time_out() > m_piler[son].get_time_out())
        {
            std::swap(m_piler[son], m_piler[ele_index]);
        }
        else
        {
            break;
        }
    }

    return true;
}
#endif

#ifdef MAP

TimeOutHandler::TimeOutHandler(int sock, int epollfd):m_sock(sock), m_epollfd(epollfd)
{}

void TimeOutHandler::handler_func(std::map<int, HttpHandler>& handlers)
{
    if (::epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sock, NULL) == -1)
    {
        ::printf("epollfd:[%d]\n", m_epollfd);
        LOGD("定时器删除epoll事件失败", "timer.cpp");
    }
    else
    {
        LOGD("定时器删除epoll事件成功！", "timer.cpp");
    }
    if (::close(m_sock) == -1)
    {
        ::printf("sock:[%d]\n", m_sock);
        LOGD("定时器关闭socket失败", "timer.cpp");
    }
    else
    {
        LOGD("定时器关闭socket成功", "timer.cpp");
    }
    if (handlers.erase(m_sock) == 0)
    {
        LOGD("定时器中http_handlers未找到该元素", "timer.cpp");
    }
    else
    {
        LOGD("定时器中http_handlers成功删除一个handler", "timer.cpp");
    }
}

void Timer::add_time_out(const TimeOutHandler& handler, time_t time_out)
{
    if (is_sock_repeat(handler, time_out))
    {
        //time_t time = m_socks.at(handler.get_socket());
        /*先将重复的timerhandler删除*/
        for (auto iter = m_timers.begin(); iter != m_timers.end(); ++iter)
        {
            if (iter->second.get_socket() == handler.get_socket())
            {
                LOGD("将旧的定时器删除", "timer.cpp");
                m_timers.erase(iter);
                break;
            }
        }
        //return ;
    }
    std::pair<time_t, TimeOutHandler> pair2 = {time_out, handler};
    m_timers.insert(pair2);
    LOGD("定时器添加成功!", "timer.cpp");
}

void Timer::remove_time_out(int sock)
{
    auto iter = m_timers.begin();
    if (!m_socks.empty())
    {
        /*删除sock和time的键值对*/
        m_socks.erase(iter->second.get_socket());
    }

    if (!m_timers.empty())
    {
        for (auto iter = m_timers.begin(); iter != m_timers.end(); ++iter)
        {
            if (iter->second.get_socket() == sock)
            {
                m_timers.erase(iter);
                break;
            }
        }
    }
}

void Timer::time_process(std::map<int, HttpHandler>& handlers)
{
    if(m_timers.empty())
    {
        return ;
    }
    time_t now = ::time(NULL);
    time_t time_out = get_root_time();

    while (time_out != 0 && time_out <= now)
    {
        /*TODO:执行到期函数*/
        auto timer_handler = m_timers.begin()->second;
        timer_handler.handler_func(handlers);
        remove_time_out(timer_handler.get_socket());
        time_out = get_root_time();
    }

}

bool Timer::is_sock_repeat(const TimeOutHandler& handler, time_t time_out)
{   
    
    auto iter = m_socks.find(handler.get_socket());
    if (iter == m_socks.end())
    {
        m_socks[handler.get_socket()] = time_out;
        return false;
    }
    else
    {
        m_socks[handler.get_socket()] = time_out;
        return true;
    }
}

time_t Timer::get_root_time() const
{
    if (!m_timers.empty())
    {
        auto iter = m_timers.begin();
        return iter->first;
    }
    return 0;
}
#endif