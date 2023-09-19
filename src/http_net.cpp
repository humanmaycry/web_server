

#include "http_net.h"



HttpNet::HttpNet(time_t timeout): m_timeout(timeout),
                                  m_is_register_write(false) 
{

}



bool HttpNet::read(int read_fd, std::string& read_buf)
{
    if (read_fd < 0)
    {
        LOGF("read socket < 0", "http_net.cpp");
    }
    /*一次性读取的缓冲，最多收取4095个有效字符，最后一个必须是\0*/
    char buf[1024] = { 0 };
    int read_index = 0;
    /*在非阻塞式的socket下，epoll边缘模式需要在循环中一次性将文件读取完*/
    while (true)
    {
        int temp_count = ::recv(read_fd, buf + read_index, 1024 - read_index, 0);
        if (temp_count == -1)
        {
            /*如果数据读取完毕，退出循环保存数据*/
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                LOGD("数据读取完毕", "http_net.cpp");
                break;
            }
            /*如果被系统信号打断，则继续尝试读取字节*/
            else if (errno == EINTR)
            {
                LOGD("read被信号打断了", "http_net.cpp");
                continue;
            }
            else
            {   /*其他情况代表出错*/
                LOGD("recv函数出错了", "http_net.cpp");
                return false;
            }
        }
        else if (temp_count == 0)
        {
            /*对端关闭了连接*/
            LOGD("对端关闭了连接", "http_net.cpp");
            return false;
        }
        /*TODO:非阻塞逻辑*/
        else
        {
            /*如果一个网络持续的发送3999个数据还是会出问题*/
            // if (read_index + temp_count >= 4000)
            // {
            //     LOGI("read_index + temp_count >= 4000\n",
            //     "http_net.cpp");
            //     return false;
            // }
            read_index += temp_count;
        }
        /*TODO:阻塞逻辑*/
        //break;
    }
    /*TODO:非阻塞逻辑*/
    /*将读取到的字节放入缓冲中*/
    //buf[read_index] = '\0';
    read_buf += buf;
    if (read_buf.size() > 1024)
    {
        LOGI("read_buf.size() > 1024\n",
                "http_net.cpp");
        return false;
    }
    /*阻塞逻辑*/
    // read_buf += buf;
    // LOGD(read_buf.c_str(), "http_net.cpp");
    return true;
}

bool HttpNet::write(int write_fd, struct iovec* data, int& iov_count, int& should_write_count)
{
    if (write_fd < 0)
    {
        LOGF("m_sock < 0", "http_net.cpp");
    }


    /*重置一下是否需要注册可写事件*/
    m_is_register_write = false;
    /*记录一下当前时间*/
    //time_t now = ::time(NULL);
    /*重置一下已经发送的字节数*/
    // m_actual_write = 0;

    while (true)
    {
        int temp_write = 0;    
        temp_write = ::writev(write_fd, data, iov_count);
        //m_actual_write += temp_write;
        if (temp_write < 0)
        {
            /*如果被信号中断*/
            if (errno == EINTR)
            {
                continue;
            }
            /*如果内核发送缓冲区已满*/
            else if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // if (should_write_count <= 0)
                // {   
                    /* 如果应该发送的字节数小于等于0，则说明发送成功了，此时应该关闭mmpa文件映射 */
                    /*发送成功不需要重新注册可写事件*/
                //     LOGD("数据全部发完(110)", "http_net.cpp");
                //     m_is_register_write = false;
                //     return true;
                // }
                // else
                // {   
                    /* 如果小于，则在超时时间内等待*/
                    //if (::time(NULL) - now < m_timeout)
                    //{
                    //   continue;
                    //}
                    //else
                    //{   
                        /**
                         * TODO:这里的注释？？？？？
                        * 如果超时，则返回false，并且注册可写事件
                        */
                        m_is_register_write = true;
                        LOGD("需要重新注册写事件", "http_net.cpp");
                        return false;
                    //}   
                //}
            }
            /*代表出错*/
            else
            {
                return false;
            }
        }
        else if (temp_write == 0)
        {
            /*对端关闭了连接*/
            LOGD("对端关闭了连接", "http_net.cpp");
            return false;
        }
        else
        {
            /*writev函数返回大于0的情况*/    
            if (temp_write < should_write_count)
            {
                /*TODO:感觉还有问题*/
                /*如果没有将数据发送完，重新调整结构体指针*/
                if (iov_count == 1)
                {
                    data[0].iov_base = (char*)data[0].iov_base + temp_write;
                    data[0].iov_len = data[0].iov_len - temp_write;
                }
                if (iov_count == 2)
                {
                    if (temp_write >= (int)data[0].iov_len)
                    {
                        int count = temp_write;
                        count = count - (int)data[0].iov_len;
                        data[0].iov_base = (char*)data[1].iov_base + count;
                        data[0].iov_len = data[1].iov_len - count;
                        iov_count = 1;
                    }
                    else
                    {
                        data[0].iov_base = (char*)data[0].iov_base + temp_write;
                        data[0].iov_len = data[0].iov_len - temp_write;
                    }
                }
                should_write_count -= temp_write;
            }
            else
            {
                LOGD("数据已经全部发完(175)", "http_net.cpp");
                break;
            }
        }
    }
    /*返回true表示对端没有关闭连接并且write没有发生错误*/
    return true;
}

void HttpNet::init()
{
    m_is_register_write = false;
}