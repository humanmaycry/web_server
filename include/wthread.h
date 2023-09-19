#ifndef _WTHREAD_H_
#define _WTHREAD_H_

#include <thread>
#include <memory>


/*线程对象基类*/
class Wthread
{
public:
    Wthread();
    virtual ~Wthread() = default;
    /*线程开始*/
    void start();
    /*线程停止运行*/
    bool stop();
    /*阻塞到线程执行完毕*/
    bool join();
    /*脱离thread对象的管理，在进程结束时自动销毁调用线程*/
    void detach();
    /*判断线程是否被销毁*/
    bool is_alive();
    
protected:
    /*线程入口函数*/
    void thread_entry();
    /*子类必须自己实现线程函数逻辑*/
    virtual void run() = 0;    


protected:
    std::shared_ptr<std::thread>   m_thread;
    /*标志线程是否在运行*/
    bool                           m_running;

};


#endif

