/**
 * @File Name: Timestamp.h
 * @brief 时间戳类的接口
 * @Author : leetion in hust email:leetion@hust.edu.cn
 * @Version : 1.0
 * @Creat Date : 2022-04-08
 * 
 */
#pragma once

#include <iostream>
#include <string>
class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch); //明确拒绝隐式转换

    static Timestamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};