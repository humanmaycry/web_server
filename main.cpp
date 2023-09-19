#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "log.h"
#include "net_init.h"
#include "singleton.h"
#include "http_management.h"

#include "mysql/include/mysql.h"



/*设置信号处理函数*/
void sig_handler(int sig, void (*handler) (int sig))
{
    struct sigaction m_sa;
    ::memset(&m_sa, 0, sizeof(m_sa));
    m_sa.sa_handler = handler;
    if (::sigaction(sig, &m_sa, NULL) != 0)
    {
        ::puts("sigaction error");
        ::exit(-1);
    }
}



/*阻塞进程函数*/
void blocking(int time = -1)
{
    //printf("blocking\n");
    if (time == -1)
    {
        while(1){}
    }
    else
    {
        while(time)
        {
            ::sleep(time);
            time = 0;
        }
    }
}

int main()
{
/*---------------------------------------------------------------------------------*/
    /**
     * 屏蔽SIGPIPE信号，如果往关闭的socket写入数据，系统将会产生SIGPIPE信号，缺省处理方式是
     * 退出当前进程
     */
    sig_handler(SIGPIPE, SIG_IGN);
/*---------------------------------------------------------------------------------*/    
    /*初始化日志*/
    if(LogHandler::init())
    {
        LOGI("日志初始化成功", "main.cpp");
    }
    else
    {
        std::cout << "日志初始化失败!\n";
        return -1;
    }
    //LogHandler::set_level(LogLevel::INFO);
/*----------------------------------------------------------------------------------*/
/*连接数据库*/
    /*mysql连接*/
    MYSQL mysql;
    /*结果集集合体*/
    MYSQL_RES* res;
    /*char* 二维数组,存放记录*/
    MYSQL_ROW row;
    /*初始化并连接数据库，获得操作数据库的句柄*/
    mysql_init(&mysql); //初始化
    LOGD("数据库句柄初始化成功!", "main.cpp");
    if (!(mysql_real_connect(&mysql, "127.0.0.1", "root", "123", "web_server", 0, NULL, 0)))
    {
        ::printf("%s\n", mysql_error(&mysql));
        LOGF("数据库连接失败", "main.cpp");
    }
    else
    {
        LOGI("数据库连接成功", "main.cpp");
    }
/*---------------------------------------------------------------------------------*/
    /**
     * 初始化网络（哦耶！）
    */
    auto net = Singleton<m_netinit::Net>::getInstance(3, "127.0.0.1", "8080", true);
    if (!net->initialize())
    {
        LOGF("网络初始化失败！", "main.cpp");
    }
    else
    {
        LOGI("网络初始化成功", "main.cpp");
    }
/*---------------------------------------------------------------------------------*/
/*TODO:测试http_management*/
    HttpManagement management(net->get_listenfd());
    management.init();
    //management.threads_stop();
    management.main_loop();
/*---------------------------------------------------------------------------------*/
    return 0;
}