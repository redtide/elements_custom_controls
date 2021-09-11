/*=============================================================================
   Copyright (c) 2021 Johann Philippe

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#ifndef CONCURRENT_BUFFERS_H
#define CONCURRENT_BUFFERS_H

#include<iostream>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>

using namespace std;

template<template<typename, typename> typename Container, typename T>
class concurrent_buffer_base
{
public:
    concurrent_buffer_base() {}
    concurrent_buffer_base(size_t size) : buf_(size) {}
    concurrent_buffer_base(size_t size, size_t init) : buf_(size, init) {}
    concurrent_buffer_base(const Container<T, std::allocator<T>> &cont) : buf_(cont) {}

    void lock() {mutex_.lock();}

    bool try_lock() {return mutex_.try_lock();}

    void unlock() {
        mutex_.unlock();
        condition_variable_.notify_one();
    }

    bool apply_to_data(std::function<void(Container<T, std::allocator<T> >& buf)> func)
    {
        if(mutex_.try_lock()) {
            func(buf_);
        } else {
            return false;
        }
    }

    // needs to lock and unlock to be safe
    Container<T, std::allocator<T>> & get_data() {return buf_;}

    // needs to unlock after to be safe
    Container<T, std::allocator<T>> & wait_and_get_data() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_variable_.wait(lock);
        return buf_;
    }

    size_t size() {return buf_->size();}
    bool empty() {return buf_->empty();}


protected:
    std::mutex mutex_;
    std::condition_variable condition_variable_;
    Container<T, std::allocator<T> > buf_;
};


template<typename T>
class concurrent_vector :
        public concurrent_buffer_base<vector, T>
{
public:
    concurrent_vector() : concurrent_buffer_base<vector, T>()
    {}
    concurrent_vector(size_t size) : concurrent_buffer_base<vector, T>(size)
    {}
    concurrent_vector(size_t size, size_t init) : concurrent_buffer_base<vector, T>(size, init)
    {}
    concurrent_vector(const vector<T> &vec) : concurrent_buffer_base<vector, T>(vec)
    {}

    void push_back(T val)
    {
       std::unique_lock lock(this->mutex_);
       this->buf_.push_back(val);
       lock.unlock();
       this->condition_variable_.notify_one();
    }

    void set(size_t index, T val)
    {
       std::unique_lock lock(this->mutex_);
       this->buf_[index] = val;
       lock.unlock();
       this->condition_variable_.notify_one();
    }

    T get(size_t index)
    {
       std::unique_lock lock(this->mutex_);
       T val = this->buf_[index];
       lock.unlock();
       this->condition_variable_.notify_one();
       return val;
    }

    void wait_and_set(size_t index, T val)
    {
       std::unique_lock lock(this->mutex_);
       this->condition_variable_.wait(lock);
       this->buf_[index] = val;
       lock.unlock();
       this->condition_variable_.notify_one();
    }

    void wait_and_push_back(T val)
    {
       std::unique_lock lock(this->mutex_);
       this->condition_variable_.wait(lock);
       this->buf_->push_back(val);
       lock.unlock();
       this->condition_variable_.notify_one();
    }

    T wait_and_get(size_t index)
    {
       std::unique_lock lock(this->mutex_);
       this->condition_variable_.wait(lock);
       T val = this->buf_[index];
       lock.unlock();
       this->condition_variable_.notify_one();
       return val;
    }

private:
};


//Concurrent queue from CsoundThreaded by Michael Gogins
template<typename Data>
class concurrent_queue
{
private:
    std::queue<Data> queue_;
    std::mutex mutex_;
    std::condition_variable condition_variable_;
public:

    bool try_lock()
    {
        return mutex_.try_lock();
    }

    void unlock()
    {
        mutex_.unlock();
        condition_variable_.notify_one();
    }

    void push(Data const& data)
    {

        std::unique_lock<std::mutex> lock(mutex_);
            queue_.push(data);
            lock.unlock();
            condition_variable_.notify_one();
    }
    bool empty()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    bool try_pop(Data& popped_value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            lock.unlock();
            return false;
        }
            popped_value = queue_.front();
            queue_.pop();
            lock.unlock();
            return true;
    }

    void wait_and_pop(Data& popped_value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            condition_variable_.wait(lock);
        }
        popped_value = queue_.front();
        queue_.pop();
    }

    void notify() {
        condition_variable_.notify_all();
    }

    std::size_t size() {
        std::unique_lock<std::mutex> lock(mutex_);
        int s=queue_.size();
        condition_variable_.notify_all();
        return s;
    }

};



#endif // CONCURRENT_BUFFERS_H
