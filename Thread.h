#pragma once

#include "noncopyable.h"
#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <atomic>
#include <string>
class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void join();
    void start();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string name() const { return name_; }

    static int numCreated() { return numCreated_;}
    
private:
    bool started_;
    bool joined_;
    
    void setDefaultName();
    // std::thread thread_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_;
};