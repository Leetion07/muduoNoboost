/**
 * @File Name: Timestamp.cc
 * @brief  
 * @Author : leetion in hust email:leetion@hust.edu.cn
 * @Version : 1.0
 * @Creat Date : 2022-04-08
 * 
 */
#include "Timestamp.h"
#include <time.h>
Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}

/**
 * @brief  使用time函数获取当前时间
 * @return Timestamp: 
 */
Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}

/**
 * @brief  使用tm结构体将时间转化为字符串
 * @return std::string: 
 */
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    tm* tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);
    return buf;
}

// #include <iostream>
// int main(){
//     std::cout << Timestamp::now().toString()<<std::endl;
//     return 0;
// }

