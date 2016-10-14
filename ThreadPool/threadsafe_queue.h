#pragma once
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <exception>

template <typename T, typename Container = std::queue<T>>
class threadsafe_queue
{
private:
	Container m_queue;
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
public:
	threadsafe_queue(void)
	{
	}

	~threadsafe_queue(void)
	{
	}

	threadsafe_queue(const threadsafe_queue &_data)
	{
		std::lock(_data.m_mutex, m_mutex);
		std::lock_guard<std::mutex> writelock(m_mutex,std::adopt_lock_t);
		std::lock_guard<std::mutex> writelock1(_data.m_mutex,std::adopt_lock_t);
		m_queue = _data.m_queue;
		m_condition.notify_all();
	}

	void push(const T &_value)
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		m_queue.push(_value);
		m_condition.notify_all();
	}

	void wait_and_pop(T &_value)
	{
		std::unique_lock<std::mutex> writelock(m_mutex);
		m_condition.wait(writelock, [this]{return !m_queue.empty();});
		_value = std::move(m_queue.front());
		m_queue.pop();
	}

	void pop(T &_value)
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		
		if(m_queue.empty()) 
		{
			throw std::exception("Threadsafe queue is empty!");
		}

		_value = m_queue.front();
		m_queue.pop();
	}

	bool try_pop(T &_value)
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		if(m_queue.empty()) 
			return false;

		_value = m_queue.front();
		m_queue.pop();
		return true;
	}

	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> writelock(m_mutex);

		if(m_queue.empty()) 
		{
			throw std::exception("Threadsafe queue is empty!");
		}

		std::shared_ptr<T> const sp_value(std::make_shared<T>(m_queue.front()));
		m_queue.pop();
		return sp_value;
	}

	bool empty()
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		bool emtpy_var = m_queue.empty();
		return emtpy_var;
	}
};

