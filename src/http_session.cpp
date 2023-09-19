#include <algorithm>
#include <cctype>
#include <iomanip>
#include <functional>

#include "http_session.h"


/*定义HTTP响应的一些状态信息*/
const std::string ok_200_title    = "OK";
const std::string error_400_title = "Bad Request";
const std::string error_400_form  = "Your request has bad synatx or is inherently impossible to satisfy.\n";
const std::string error_403_title = "Forbidden";
const std::string error_403_form  = "You don't have permission to get file from this server.\n";
const std::string error_404_title = "Not Found";
const std::string error_404_form  = "The requested file is not found.\n";
const std::string error_500_title = "Internal Server Error";
const std::string error_500_form  = "There was an unusual problem serving the requested file.\n";



HttpSession::HttpSession(): m_parse_index(0),
                            m_start_index(0),
                            m_parse_status(REQUEST_LINE),
                            m_root_path("../my_project")
{
    ::memset(&m_file_stat, 0, sizeof(m_file_stat));
}




HTTP_PARSE_RESULT HttpSession::http_parse(std::string& buf_read)
{

    HTTP_LINE_STATUS line_stat;
    HTTP_PARSE_RESULT result;

    /*保存一下开始解析行内容的位置*/
    //m_start_index = m_parse_index;

    while((m_parse_status == RESOURCE_PARSE) || 
            ((line_stat = line_parse(buf_read)) == LINE_OK))
    {
        switch(m_parse_status)
        {
            /*TODO:先不考虑Content内容*/
            case REQUEST_LINE:
            {
                result = request_parse(buf_read); 
                if (result == BAD_REQUEST)
                {
                    LOGI("主状态机, http请求行出错!(32)", "http_session");
                    return BAD_REQUEST;
                }
                m_start_index = m_parse_index;
                break;
            }
            case HEADERS:
            {
                result = headers_parse(buf_read);
                if (result == BAD_REQUEST)
                {
                    LOGI("主状态机, http头部字段出错!(32)", "http_session");
                    return BAD_REQUEST;
                }
                break;
            }
            case RESOURCE_PARSE:
            {
                /*TODO:暂时不分析请求body*/
                return resource_parse();
                break;
            }
            
        }
        m_start_index = m_parse_index;
    }

    if (line_stat == LINE_BAD)
    {
        return BAD_REQUEST;
    }
    
    return INCOMPLETE;
}

void HttpSession::print_fields()
{
    for (auto iter:m_fields)
    {
        std::cout << "[" << iter.first << "]" << ":[" << iter.second << "]" << std::endl;
        
    }
}

bool HttpSession::make_response(HTTP_PARSE_RESULT result, iovec* w_iovec,
                         int& len_iovec, int& should_write)
{
    if (result == INCOMPLETE)
    {
        LOGD("不应该将INCOMPLETE作为参数传入make_response函数中", "http_session.cpp");
        return false;
    }

    switch (result)
    {
        case BAD_REQUEST:
        {
            add_resp("400", "Bad Request", error_400_form.size(), error_400_form.c_str());
            break;
        }
        case GET_REQUEST:
        {
            add_resp("200", "OK", m_file_stat.st_size, NULL);

            w_iovec[0].iov_base = (char*)m_buf_response.c_str();
            w_iovec[0].iov_len = m_buf_response.size();
            should_write += m_buf_response.size();

            //w_iovec[1].iov_base = (char*)m_file_ptr.c_str();
            w_iovec[1].iov_base = m_file_ptr;
            w_iovec[1].iov_len = m_file_stat.st_size;
            should_write += m_file_stat.st_size;

            len_iovec = 2;
            return true;
        }
        default:
            break;
    }
    /*TODO:会引发bug吗*/
    w_iovec[0].iov_base = (char*)m_buf_response.c_str();
    w_iovec[0].iov_len  = m_buf_response.size();
    len_iovec = 1;
    should_write += m_buf_response.size();
    return true;
}

bool HttpSession::un_mmap()
{
    if (/*!m_file_ptr.empty()*/!m_file_ptr)
    {
        LOGD("接触指针文件映射!(139)", "http_session.cpp");
        //::munmap((char*)m_file_ptr.c_str(), m_file_stat.st_size);
        ::munmap(m_file_ptr, m_file_stat.st_size);
        return true;
    }
    return false;
}

const std::string& HttpSession::get_method() const
{
    return m_fields.at("method");
}

void HttpSession::init()
{
    m_parse_index = 0;
    m_start_index = 0;
    m_parse_status = REQUEST_LINE;
    m_fields.clear();
    ::memset(&m_file_stat, 0, sizeof(m_file_stat));
    m_file_ptr = NULL;
    m_buf_response.clear();
}

bool HttpSession::is_keep_alive() const
{
    auto it = m_fields.find("connection");
    if (it != m_fields.end())
    {
        if (it->second == "keep-alive")
        {
            LOGD("该连接为长连接", "http_session.cpp");
            return true;
        }
        LOGD("该连接为短连接", "http_session.cpp");
        return false;
    }
    LOGD("没有找到connection字段(小写)", "http_session.cpp");
    return false;
}

HTTP_LINE_STATUS HttpSession::line_parse(std::string& buf_read)
{   
    if (buf_read.size() == m_parse_index)
    {
        //LOGD("当前buffer全部解析完毕(73)", "http_session.cpp");
        return BUF_EMPTY;
    }
    //m_parse_index = 0;
    for (; m_parse_index < buf_read.size(); m_parse_index++)
    {
        /*如果找到了\r，则判断下一个字节*/
        if (buf_read[m_parse_index] == '\r')
        {
            /*如果下一个字节没有越界，继续判断*/
            if (m_parse_index + 1 < buf_read.size())
            {
                /*如果下一个字节是\n，说明找到了一个完整行*/
                if (buf_read[m_parse_index + 1] == '\n')
                {
                    ++m_parse_index;
                    ++m_parse_index;
                    return LINE_OK;
                }
                /*如果不是\n,说明行出错*/
                else
                {
                    /*TODO:有必要设置m_parse_index吗*/
                    m_parse_index = 0;
                    //LOGD("\\r后面的字符不是\\n,行有语法错误", "http_session.cpp");
                    return LINE_BAD;
                }
            }
            /*如果下一个字节越界，说明行不完整*/
            else
            {
                ++m_parse_index;
                return LINE_OPEN;
            }
        }
        else if (buf_read[m_parse_index] == '\n')
        {
            if (m_parse_index - 1 >= 0)
            {
                if (buf_read[m_parse_index - 1] == '\r')
                {
                    /*行完整*/
                    ++m_parse_index;
                    return LINE_OK;
                }
                else
                {
                    /**TODO:同上
                     * 行错误*/
                    m_parse_index = 0;

                    return LINE_BAD;
                }
            }
            else
            {
                //LOGD("第一个字节是\\n，行错误(135)", "http_session.cpp");
                m_parse_index = 0;
                return LINE_BAD;
            }
        }
    }
    //LOGD("行不完整，没有遇到\\r\\n", "http_session.cpp");
    return LINE_OPEN;
}

HTTP_PARSE_RESULT HttpSession::request_parse(std::string& buf)
{
/*-----------------------------------*/
    //m_parse_index = buf.size();
/*-----------------------------------*/
    std::string temp;
    temp.assign(buf, m_start_index, m_parse_index);
    std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);

    /*如果找不到空格或者\t字符则返回 bad request*/
    size_t ret1 = temp.find(" ");
    size_t ret2 = temp.find("\t");
    if (ret1 == std::string::npos && ret2 == std::string::npos)
    {
        LOGD("请求行中找不到空格或者'\t'字符，返回错误的请求(149)", "http_session.cpp");
        return BAD_REQUEST;
    }

    std::string method;
    /*遍历请求行*/
    for (auto element:temp)
    {  
        if (element == ' ' || element == '\t')
        {
            break;
        }
        method += element;
    }
    m_fields["method"] = method;

    temp.erase(0, method.size());
    //std::cout << "after method erase:[" << temp << "] " << std::endl;
    for(auto element:temp)
    {
        if (element == ' ' || element == '\t')
        {
            temp.erase(0, 1);
        }
        else
        {
            break;
        }
        //std::cout << "temp:[" << temp <<"]" << std::endl;
    }
    //std::cout << "after method erase and space erase:[" << temp << "] " << std::endl;

    /*再次判断有没有空格或者\t符号*/
    ret1 = temp.find(" ");
    ret2 = temp.find("\t");
    if (ret1 == std::string::npos && ret2 == std::string::npos)
    {
        LOGD("请求行中找不到空格或者\\t字符，返回错误的请求(183)", "http_session.cpp");
        return BAD_REQUEST;
    }

    //std::cout << "after method: [" << temp << "]" <<std::endl;
    
    std::string url;
    for(auto element:temp)
    {
        if (element == ' ' || element == '\t')
        {
            break;
        }
        url += element;
    }
    m_fields["url"] = url;
    temp.erase(0, url.size());

    for(auto element:temp)
    {
        if (element == ' ' || element == '\t')
        {
            temp.erase(0, 1);
        }
    }
    /*版本号仅支持1.1   ===>   HTTP/1.1*/
    std::string version;
    for (auto iter = temp.begin(); iter != temp.end(); ++iter)
    {
        *iter = toupper(*iter);
    }
    version.assign(temp, 0, 8);

    if (version != "HTTP/1.1")
    {
        std::cout << version << std::endl;
        LOGD("HTTP版本号错误(212)", "http_session.cpp");
        return BAD_REQUEST;
    }

    m_fields["version"] = version;

    /*调整下次开始解析的位置*/
    //m_start_index = m_parse_index - 1;

    
    m_parse_status = HEADERS;
    return GOOD_LINE;
}

HTTP_PARSE_RESULT HttpSession::headers_parse(std::string& buf)
{
    std::string temp;
    temp.assign(buf, m_start_index, m_parse_index);
    std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);

    if (temp[0] == '\r' && temp[1] == '\n')
    {
        m_parse_status = RESOURCE_PARSE;
        LOGD("遇到空行了,此时可以分析请求资源文件(286)", "http_session.cpp");
        //buf.erase(0, m_parse_index);
        return GOOD_LINE;
    }

    /*开始寻找':'*/
    std::string h_key;
    std::string h_value;

    size_t ret = 1;
    ret = temp.find(':');
    if (ret == std::string::npos)
    {
        LOGD("头部字段没有找到[:]冒号(294)", "http_session.cpp");
        return BAD_REQUEST;
    }
    else
    {
        /*获取头部字段的key*/
        for (auto e : temp)
        {
            if (e == ':')
            {
                break;
            }
            if (e == ' ' || e == '\t')
            {
                continue;
            }
            h_key += e;
        }
        temp.erase(0, ret);
        /*获取头部字段的value*/
        for (auto e : temp)
        {
            if (e == '\r')
            {
                break;
            }
            if (e == ':' || e == '\t' || e == ' ')
            {
                continue;
            }
            h_value += e;
        }
        m_fields[h_key] = h_value;
    }

    //buf.erase(0, m_parse_index);
    //m_start_index = m_parse_index - 1;
    return GOOD_LINE;
}

HTTP_PARSE_RESULT HttpSession::resource_parse()
{
    std::string f_file_path = m_root_path + m_fields["url"];
    if (::stat(f_file_path.c_str(), &m_file_stat) < 0)
    {
        LOGD("文件不存在，可能出现文件路径错误(357)", "http_session.cpp");
        return NO_SOURCE;
    }
    /*如果目标文件是目录则返回错误*/
    if (S_ISDIR(m_file_stat.st_mode))
    {
        LOGD("请求资源为目录，返回错误(363)", "http_session.cpp");
        return BAD_REQUEST;
    }
    /*判断客户对于请求文件是否有权限访问*/
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        LOGD("请求资源无权访问(369)", "http_session.cpp");
        return BAD_REQUEST;
    }
    /**TODO:到时候记得munmap
     * 如果目标文件存在且有权限访问,映射该文件到进程的地址空间中*/
    LOGD(f_file_path.c_str(), "http_session.cpp");
    int fd = ::open(f_file_path.c_str(), O_RDONLY);
    //m_file_ptr += (char*)(::mmap(0, m_file_stat.st_size, PROT_READ,
    //                                MAP_PRIVATE, fd, 0));
    m_file_ptr = (char*)::mmap(0, m_file_stat.st_size, PROT_READ,
                                    MAP_PRIVATE, fd, 0);
    ::close(fd);

    return GET_REQUEST;
}

bool HttpSession::add_resp(const char* stat_code, 
                        const char* stat_message, size_t length, const char* body)
{
    add_resp_status(stat_code, stat_message);
    add_resp_headers(length);
    add_resp_space();
    if (body)
    {
        add_resp_body(body);
    }
    return true;
}

bool HttpSession::add_resp_status(const char* stat_code, const char* stat_message)
{
    m_buf_response += m_fields["version"];
    m_buf_response += " ";
    m_buf_response += stat_code;
    m_buf_response += " ";
    m_buf_response += stat_message;
    m_buf_response += "\r\n";
    return true;
}

bool HttpSession::add_resp_headers(size_t length)
{
    /*TODO:暂时只添加Content-Length和Connection字段*/

    m_buf_response += "Content-Length: ";
    char len[50] = {0};
    ::snprintf(len, 50, "%ld", length);
    m_buf_response += len;
    m_buf_response += "\r\n";
    
    if (is_keep_alive())
    {
        m_buf_response += "Connection: keep-alive\r\n";
    }
    else
    {
        m_buf_response += "Connection: close\r\n";
    }

    return true;
}

bool HttpSession::add_resp_space()
{
    m_buf_response += "\r\n";
    return true;
}

bool HttpSession::add_resp_body(const char* body)
{
    m_buf_response += body;
    return true;
}