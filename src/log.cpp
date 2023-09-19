#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>



#include "log.h"


std::unique_ptr<m_logger::Log>      LogHandler::m_logger;
bool                                LogHandler::m_init_ready = false;


bool LogHandler::init(const char* filename)
{
    m_logger.reset(new m_logger::Log(filename));
    if (m_logger->init())
    {
        m_init_ready = true;
    }
    return true;
}

void LogHandler::uinit()
{
    //m_logger->uinit();
    m_init_ready = false;
    m_logger.reset(nullptr);
}

bool LogHandler::output(LogLevel level, const char* message, const char* cpp_filename)
{
    if (!m_init_ready)
    {
        std::cout << "日志未初始化" << std::endl;
        return false;
    }  
    return m_logger->output(level, message, cpp_filename);;
}

bool LogHandler::set_level(LogLevel level)
{
    return m_logger->set_level(level);
}


m_logger::Filebuffer::Filebuffer(): m_buf(""), m_is_buf_full(false)                                    
{

}

void m_logger::Filebuffer::make_buffer(std::string& make_buf, LogLevel level,
                                    const char* message, const char* cpp_filename)
{
    add_time(make_buf);
    add_level(make_buf, level);
    add_message(make_buf, message, cpp_filename);
}

void m_logger::Filebuffer::add_time(std::string& make_buf)
{
    time_t t = time(NULL);
    char temp[32] = { 0 };
    strftime(temp, sizeof(temp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    make_buf += temp;
    make_buf += " ";
}

void m_logger::Filebuffer::add_level(std::string& make_buf, LogLevel level)
{
    if(level == LogLevel::DEBUG)
    {
        make_buf += "[DEBUG]: ";
    } else if(level == LogLevel::INFO) {
        make_buf += "[INFO]: ";
    } else if(level == LogLevel::WARN) {
        make_buf += "[WARN]: ";
    } else if(level == LogLevel::ERROR) {
        make_buf += "[ERROR]: ";
    } else if(level == LogLevel::FATAL) {
        make_buf += "[FATAL]: ";
    }
}

void m_logger::Filebuffer::add_message(std::string& make_buf,
                                        const char* message,
                                        const char* cpp_filename)
{
    make_buf += message;
    make_buf += " ";
    make_buf += "[cpp_filename]: ";
    make_buf += cpp_filename;
    make_buf += "\n";
}

bool m_logger::Filebuffer::append(LogLevel level, const char* message, const char* cpp_filename)
{
    /*待组装的字符串*/
    std::string   make_buf = "";
    /*开始组装字符串*/
    make_buffer(make_buf, level, message, cpp_filename);
    
    /*如果不能继续添加数据，此处将m_is_buf_full设置为true*/
    if (isfull(make_buf.size()))
    {
        /*当前缓冲区无法继续添加数据了*/
        return false;
    }

    m_buf.append(make_buf);

    return true;
}
void m_logger::Filebuffer::set_status(bool status)
{
    m_is_buf_full = status;
    if (!m_is_buf_full)
    {
        m_buf = "";
    }
}

/*public*/

bool m_logger::Filebuffer::isfull() const
{
    return m_is_buf_full;
}

const std::string& m_logger::Filebuffer::get_buf() const
{
    return m_buf;
}

/*private*/
bool m_logger::Filebuffer::isfull(size_t add_count) const
{
    if ((m_buf.size() + add_count) > BUF_MAX_SIZE)
    {
        m_is_buf_full = true;
    }
    return m_is_buf_full;
}


m_logger::Log::Log(const char* filename):                   
                    m_file_name(filename),
                    m_cur_level(LogLevel::DEBUG),
                    m_fp(nullptr), m_running(false)
{

}

//bool m_logger::Log::m_running = false;

m_logger::Log::~Log()
{
    /*结束线程*/
    // m_logger_thread.stop();
    //Log::m_running = false;
    uinit();
}

bool m_logger::Log::init()
{
    if (!get_file_ptr())
    {
        std::cerr << "日志初始化失败" << std::endl;
    }
    Log::m_running = true;
    m_logger_thread.reset(new std::thread(&Log::thread_entry, this));
    //printf("日志线程初始化成功！\n");
    return true;
}

void m_logger::Log::uinit()
{
    m_running = false;
    m_cond.notify_one();
    if (m_logger_thread->joinable())
    {
        m_logger_thread->join();
    }
    if (m_fp!=NULL)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
}

bool m_logger::Log::is_running() const
{
    return m_running;
}

bool m_logger::Log::set_level(LogLevel level)
{
    if (level < LogLevel::DEBUG || level > LogLevel::FATAL)
    {
        return false;
    }
    m_cur_level = level;
    return true;
}

bool m_logger::Log::output(LogLevel level, const char* message, const char* cpp_filename)
{
    if (level < m_cur_level)
    {
        return false;
    }

    Filebuffer buf;
    /*组装日志消息*/
    if (!buf.append(level, message, cpp_filename))
    {
        std::cout << "日志过长！【" << message << "】\n";
        return false;
    }
    /**/
    if (level != LogLevel::FATAL)
    {   /*整个if作用域都是加锁状态*/
        std::unique_lock<std::mutex> lock(m_mutex);
        m_log_list.push_back(buf);
        m_cond.notify_one();
        return true;
    }
    else
    {
        /*如果日志等级是FATAL此时当前线程采用同步写日志的方式*/
        if (!write_file(buf.get_buf()))
        {
            std::cout << "磁盘写入失败" << std::endl;
        }
            
        std::cout << buf.get_buf().c_str() << "[LOG]致命错误,程序主动退出!\n";
        crash();
    }
    return true;
}

void m_logger::Log::crash()
{
    char* p = NULL;
    *p = 0;
    // exit(1);
}

void m_logger::Log::thread_entry()
{
    /**/
    //printf("我进入子线程啦！\n");
    //std::cout << "日志子线程启动\n";
    run();
}
/*消费者*/
void m_logger::Log::run()
{
    while(m_running)
    {
        Filebuffer buf;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (m_log_list.empty())
            {
                m_cond.wait(lock);
                if (!m_running)
                {
                    return ;
                }
            }
            buf = m_log_list.front();
            m_log_list.pop_front();
        }
        /*写入磁盘*/
        if (!write_file(buf.get_buf()))
        {
            std::cout << "日志写入失败\n";
            return ;
        }
        std::cout << buf.get_buf().data();
        fflush(m_fp);
    }
    
}

bool m_logger::Log::write_file(const std::string& logdata)
{
    if (!m_fp)
    {
        std::cout << "文件指针为空" << std::endl;
        return false;
    }
    std::string data(logdata);

    //int size = data.size();
    /*在循环中保证数据完全写入磁盘*/
    while (true)
    {
        int ret = -1;
        if ((ret = (int)fwrite(data.c_str(), 1, data.size(), m_fp)) <= 0)
        {
            return false;
        }
        //m_write_count++;
        data.erase(0, ret);
        if (data.empty())
        {
            break;
        }
    }
    return true;
}

bool m_logger::Log::get_file_ptr()
{
    if (m_file_name.empty())
    {
        return false;
    }
    if ((m_fp = fopen(m_file_name.c_str(), "w+")) == NULL)
    {
        return false;
    }
    return true;
}