/**
 * @File Name: noncopyable.h
 * @brief
 * @Author : leetion in hust email:leetion@hust.edu.cn
 * @Version : 1.0
 * @Creat Date : 2022-04-08
 *
 */
#pragma once
/**
 * @brief:继承该类的子类不允许拷贝构造和拷贝赋值
 *
 */
class noncopyable
{
public:
	noncopyable(const noncopyable &) = delete;
	noncopyable &operator=(const noncopyable &) = delete;

protected:
	noncopyable() = default;
	~noncopyable() = default;
};