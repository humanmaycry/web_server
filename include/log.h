#ifndef _LOG_H
#define _LOG_H

#include <iostream>
#include <thread>
#include <list>
#include <string>
#include <time.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <sys/time.h>


#define LOGD(log_message, cpp_file_name) \
LogHandler::output(LogLevel::DEBUG, log_message, cpp_file_name);

#define LOGF(log_message, cpp_file_name) \
LogHandler::output(LogLevel::FATAL, log_message, cpp_file_name);

#define LOGI(log_message, cpp_file_name) \
LogHandler::output(LogLevel::INFO, log_message, cpp_file_name);


/*日志的等级从低到高*/
        /**
         * DEBUG（调试）：输出细粒度的信息，主要用于输出调试信息
         * INFO （信息）：输出粗粒度信息，主要用于输出程序运行的一些重要信息
         * WARN （警告）：表示可能会出现潜在的错误
         * ERROR（错误）：虽然是错误信息，但不会停止程序的运行
         * FATAL（致命）：属于是重大错误，此时应该停止程序的运行
        */
enum class LogLevel{ DEBUG, INFO, WARN, ERROR, FATAL };

namespace m_logger
{

    class Filebuffer
    {
    public:
        /*缓冲区字节流的最大长度*/
        static const uint32_t         BUF_MAX_SIZE = 1024;
        
        
        /**/
    public:
        Filebuffer();
        ~Filebuffer() = default;
        /*往缓冲区添加新的内容,如果不能继续添加，将is_buf_full设置为true*/
        bool append(LogLevel level, const char* message, const char* cpp_filename);        
        /*返回当前缓冲区状态*/
        /*返回true表示满，返回false表示未满*/
        bool isfull() const;
        /*返回当前缓冲对象*/
        const std::string& get_buf() const;
        /*重新设置状态,true表示将当前缓冲设置为满，false表示将当前缓冲设置为空闲,
        如果设置为空闲将清空当前缓冲内容*/
        void set_status(bool status);
    private:
        /*组装字符串*/
        void make_buffer(std::string& make_buf, LogLevel level,
                    const char* message, const char* cpp_filename);
        /*添加时间*/
        void add_time(std::string& make_buf);
        /*添加日志等级*/
        void add_level(std::string& make_buf, LogLevel level);
        /*添加附带信息*/
        void add_message(std::string& make_buf, const char* message,
                    const char* cpp_filename);
        /*判断是否可以继续添加信息*/
        /*true表示当前缓冲区满，false表示当前缓冲区空闲*/
        bool isfull(size_t add_count) const;

    private:
        std::string                         m_buf;
        /*标志当前缓冲区是否可以继续添加数据,false表示空闲，true表示满*/
        mutable bool                        m_is_buf_full;
    };

    class Log
    {
    public:
        /*构造函数需要指定日志文件名字*/
        Log(const char* file_name = "../server.log");
        ~Log();
        
        /*初始化*/
        bool init();
        /*日志入口文件*/
        bool output(LogLevel level, const char* message, const char* cpp_filename);
        
        bool is_running() const;
        /*设置日志等级*/
        bool set_level(LogLevel level);
        /*获取写入文件的次数*/
        //size_t get_write_count() const;


    private:
    /**/
        void uinit();
        /*线程入口函数*/
        void thread_entry();
        /*消费者函数*/
        void run();
        /*获取日志文件fd*/
        //bool get_file_fd();
        /*获取文件指针*/
        bool get_file_ptr();
        /*将日志写入磁盘*/
        bool write_file(const std::string& log_data);
        /*程序主动崩溃*/
        void crash();
    private:
        
        /*日志缓冲区队列*/
        std::list<m_logger::Filebuffer>   m_log_list;
        /*目标日志文件名*/
        const std::string                 m_file_name;
        /*线程对象*/
        std::shared_ptr<std::thread>      m_logger_thread;
        /*当前日志等级*/
        LogLevel                          m_cur_level;
        /*目标日志文件文件指针*/
        FILE*                             m_fp;
        /*互斥锁*/
        std::mutex                        m_mutex;
        /*条件变量*/
        std::condition_variable           m_cond;
        /*线程运行条件*/
        bool                              m_running;
        /*成功写入文件的次数*/
        //std::atomic<size_t>               m_write_count;
    };
}

class LogHandler
{
public:
    LogHandler() = delete;
    ~LogHandler() = delete;

    LogHandler(const LogHandler& rhs) = delete;
    LogHandler& operator=(const LogHandler& rhs) = delete;        
    
    static bool init(const char* filename = "../server.log");
    static void uinit();
    static bool output(LogLevel level, const char* message, const char* cpp_filename);
    static bool set_level(LogLevel level);

private:
    static std::unique_ptr<m_logger::Log>    m_logger;
    /*true 表示日志已经初始化*/
    static bool                              m_init_ready;
};


#endif