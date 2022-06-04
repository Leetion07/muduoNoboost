/*
 * @Author: your name
 * @Date: 2022-04-08 19:28:46
 * @LastEditTime: 2022-04-08 19:28:46
 * @LastEditors: Please set LastEditors
 * @Description: 
 * @FilePath: /mymuduo/Logger.cc
 */
#include "Logger.h"
#include "Timestamp.h"

#include <string>
#include <iostream>
Logger &Logger::instance()
{
    //static静态成员变量,只会生成一次实例，单例模式
    static Logger logger;
    return logger;
}
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}
void Logger::log(std::string msg)
{
    //根据不同的消息类别打印不同的消息
    //bad smell in switch case
    //如果需要扩展可以更改为策略模式
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEUG]";
        break;
    }
    // 打印时间和msg
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}

// int main(){
//     LOG_INFO("%d,%d",2,3);
//     return 0;
// }