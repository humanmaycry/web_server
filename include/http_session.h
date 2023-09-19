#ifndef _HTTP_SESSION_H_
#define _HTTP_SESSION_H_

#include <map>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <algorithm>
#include <iostream>

#include "http_enums.h"
#include "log.h"

/*http业务层，包括http协议的解析和组装http回复报文*/
class HttpSession
{
public:
    HttpSession();
    ~HttpSession() = default;
    
    /* 返回read_buf解析结果*/
    HTTP_PARSE_RESULT    http_parse(std::string& buf_read);
    /*输出字段内容*/
    void                 print_fields();
    /*根据组装应答报文*/
    bool                 make_response(HTTP_PARSE_RESULT result,iovec* w_iovec,
                                    int& len_iovec, int& should_write);
    /*解除mmap文件映射*/
    bool                 un_mmap();
    /*返回请求方法*/
    const std::string&   get_method() const;
    /*重新将http_session初始化*/
    void                 init();
    /*是否需要长连接*/
    bool                 is_keep_alive() const;

private:
    void set_start_index(size_t index) { m_start_index = index; }
    size_t get_parse_index() { return m_parse_index; }

    /*解析一行，返回行是否完整，是否出错*/
    HTTP_LINE_STATUS        line_parse(std::string& buf_read);
    /**
     * TODO:
     * 暂时只支持get方法
     * 解析请求行
     * */
    HTTP_PARSE_RESULT       request_parse(std::string& buf_read);
    /**
     * TODO:
     * 解析头部字段
     * 暂时只解析Connection,host,Content-Length字段
     * */
    HTTP_PARSE_RESULT       headers_parse(std::string& buf_read);
    /*简单判断是否将数据读取完整*/
    //bool                    r_content_parse();
    /*解析请求资源*/
    HTTP_PARSE_RESULT       resource_parse();

    /**
     * TODO:暂且认为都能添加成功,即都返回true
     * */
    bool                    add_resp(const char* stat_code, const char* stat_message, size_t length, const char* body);
    /*添加状态码*/
    bool                    add_resp_status(const char* stat_code, const char* stat_message);
    /*添加响应头部字段*/
    bool                    add_resp_headers(size_t length);
    /*添加空行*/
    bool                    add_resp_space();
    /*添加响应体*/
    bool                    add_resp_body(const char* body);

private:
    /*buf开始寻找\r\n的位置*/
    size_t                              m_parse_index;
    /*buf开始解析完整行的位置*/
    size_t                              m_start_index;

    /*记录一下当前的解析状态，例如正在解析请求行、头部字段等*/
    HTTP_STATUS_PARSE                   m_parse_status;

    /*保存http请求的请求方法, 头部字段*/
    /**method,url,version*/
    std::map<std::string, std::string>  m_fields;

    /*资源文件根目录地址,默认为../my_project*/
    std::string                         m_root_path;
    /*目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息*/
    struct stat                         m_file_stat;
    /*请求文件指针的起始位置*/
    //std::string                         m_file_ptr;
    char*                               m_file_ptr;
    /*待组装的响应报文*/
    std::string                         m_buf_response;
};

#endif