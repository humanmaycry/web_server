#ifndef _SINGLETON_H_
#define _SINGLETON_H_
#include <memory>
#include <mutex>
#include <stdio.h>
#include <stdarg.h>



#include "net_init.h"


/**
 * 单例模式模板
 * 
 * 线程安全的懒汉模式
 * */
template<typename T>
class Singleton
{
public:
    static std::shared_ptr<T>& getInstance()
    {

        if (m_object == nullptr)
        {
            std::mutex m_mutex;
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_object == nullptr)
            {
                m_object.reset(new T());
            }
        }
        return m_object;
    }
    

protected:
    Singleton();
    Singleton(const Singleton& other);
    Singleton& operator=(const Singleton& other);

private:
    static std::shared_ptr<T>    m_object;
};

/*初始化成员变量*/
template<typename T>
typename std::shared_ptr<T> Singleton<T>::m_object;

/*------------------------------------------------------------------------------------*/
/*对Net类全特化*/

using namespace m_netinit;

template<>
class Singleton<Net>
{
public:
    static std::shared_ptr<Net>& getInstance(int arg_count, ...)
    {
        ::va_list v_list;

        if (m_object == nullptr)
        {
            std::mutex m_mutex;
            std::unique_lock<std::mutex> lock(m_mutex);

            ::va_start(v_list, arg_count);
            if (m_object == nullptr)
            {
                m_object.reset(new Net(va_arg(v_list, const char*), 
                            va_arg(v_list, const char*), va_arg(v_list, int)));
            }
            ::va_end(v_list);
        }
        return m_object;
    }
    

protected:
    Singleton();
    Singleton(const Singleton& other);
    Singleton& operator=(const Singleton& other);

private:
    static std::shared_ptr<Net>    m_object;
};

/*初始化成员变量*/
std::shared_ptr<Net> Singleton<Net>::m_object;

#endif