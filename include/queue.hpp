#ifndef THREAD_SAFE_QUEUE_
#define THREAD_SAFE_QUEUE_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <iostream>
#include <vector>
#include <string>


template <typename T>
class Queue{
public:
	Queue(){}

	void push(const T&node){
		{
			std::lock_guard<std::mutex> lock(mutex_);
			queue_.push(node);
		}
		cond_.notify_one();
	}

	T& pop(){
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this](){
			return !queue_.empty();
		});
		auto& value = queue_.front();
		queue_.pop();
		lock.unlock();
		return value;
	}

	bool empty(){
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.empty();
	}

//private:
	std::mutex mutex_;
	std::condition_variable cond_;
	std::queue<T> queue_;
};
#endif
