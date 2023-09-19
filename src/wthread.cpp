#include <iostream>
#include <stdio.h>
#include "wthread.h"


Wthread::Wthread():m_thread(nullptr), m_running(false)
{

}

void Wthread::start()
{
    m_running = true;
    /*创建thread对象*/
    try{
        m_thread.reset(new std::thread(&Wthread::thread_entry, this),
                        [](std::thread* thread){
                            if (thread->joinable())
                            {
                                thread->detach();
                            }
                            delete thread;
                            //printf("我被释放了\n");
                        }); 
    } catch(std::bad_alloc& e) {
        /*此处可以打印一条日志！*/
        std::cerr << "Memory allocation error" << e.what() << std::endl;
    }
    
}

bool Wthread::stop()
{
    if (m_running)
    {
        m_running = false;
        return true;
    }

    return false;
}

bool Wthread::join()
{
    if(m_thread->joinable())
    {
        m_thread->join();
        return true;
    }

    return false;
}

void Wthread::detach()
{
    m_thread->detach();
}

bool Wthread::is_alive()
{
    return m_running;
}

void Wthread::thread_entry()
{
    while (m_running)
    {
        run();
    }
    /*此处可以打一条日志*/
    printf("线程退出\n");
}