#ifndef _HTTP_ENUMS_H
#define _HTTP_ENUMS_H


/**
 * 当前解析http服务所处的状态
*/
enum HTTP_STATUS_PARSE{
    REQUEST_LINE,           /*当前正在解析请求行*/
    HEADERS,                /*当前正在解析头部字段*/
    RESOURCE_PARSE          /*当前正在分析所请求的资源文件*/
};

/**
 * 当前行是否完整，出错
 */
enum HTTP_LINE_STATUS{
    LINE_OK,             /*完整行*/
    LINE_OPEN,           /*行不完整*/
    LINE_BAD,            /*行错误*/
    BUF_EMPTY           /*buf为空*/
};

/**
 * http解析后的结果
  */
enum HTTP_PARSE_RESULT{
    INCOMPLETE,             /*http请求不完整*/
    GOOD_LINE,              /*行合法*/
    BAD_REQUEST,            /*http请求有语法错误*/
    GOOD_RESOURSE,          /*请求的资源合法*/
    GET_REQUEST,            /*合法的HTTP请求(即一次完整的http请求)*/
    NO_SOURCE,              /*请求的资源不存在*/
    NO_PERMISSION,          /*请求资源没有足够的访问权限*/
};

#endif