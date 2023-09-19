

#include "http_handler.h"

HttpHandler::HttpHandler(int cln_sock): m_rw_socket(cln_sock),
                                        m_len_iovec(0), m_should_write(0)
{
    /*memset是线程安全函数*/
    ::memset(m_w_iovec, 0, sizeof(m_w_iovec));
}

bool HttpHandler::read()
{
    return m_http_net.read(m_rw_socket, m_read_buffer);
}

bool HttpHandler::write_get()
{
    if (m_http_net.write(m_rw_socket, m_w_iovec, m_len_iovec, m_should_write))
    {
        /*如果发送成功了就解除文件映射*/
        m_http_session.un_mmap();
        return true;
    }
    else if (!is_keep_alive())
    {
        LOGD("发送失败", "http_handler.cpp");
        m_http_session.un_mmap();
    }
    return false;
}

bool HttpHandler::parse()
{
    /*获得http协议解析的结果*/
    HTTP_PARSE_RESULT ret;
    ret = m_http_session.http_parse(m_read_buffer);
    if (ret != INCOMPLETE)
    {
        m_http_session.make_response(ret, m_w_iovec, m_len_iovec, m_should_write);
        //m_should_write = w_iovec[0]
        return true;
    }

    /*如果http请求不完整,则返回false*/    
    return false;
}


const std::string&  HttpHandler::get_method() const
{
    return m_http_session.get_method();
}

bool HttpHandler::is_keep_alive()
{
    return m_http_session.is_keep_alive();
}

void HttpHandler::init()
{
    m_read_buffer.clear();
    ::memset(m_w_iovec, 0, sizeof(m_w_iovec));
    m_len_iovec = 0;
    m_should_write = 0;
    /*网络层和业务层分别执行初始化函数*/
    m_http_net.init();
    m_http_session.init();
}

void HttpHandler::close()
{
    if (::close(m_rw_socket) == -1)
    {
        LOGD("close 函数返回-1,调用失败", "http_handler.cpp");
    }
}

bool HttpHandler::is_reset_write() const
{
    return m_http_net.is_register_write();
}

int HttpHandler::get_socket() const
{
    return m_rw_socket;
}