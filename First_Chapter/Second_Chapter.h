#pragma once
#include "stdafx.h"
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <exception>

template <typename T, typename Container = std::queue<T>>
class thread_safe_condition_queue
{
private:
	Container		m_data;
	mutable std::mutex		m_mutex;
	std::condition_variable	m_condition_var;
public:
	thread_safe_condition_queue(void)
	{

	}

	~thread_safe_condition_queue(void)
	{
	}

	thread_safe_condition_queue(const thread_safe_condition_queue &data)
	{
		std::lock(data.m_mutex, m_mutex);

		std::lock_guard<std::mutex> writelock(m_mutex,std::adopt_lock_t);
		std::lock_guard<std::mutex> writelock(data.m_mutex,std::adopt_lock_t);
		m_data = data.m_data;
		m_condition_var.notify_all();
	}

	void push(const T &value)
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		m_data.push(value);
		m_condition_var.notify_all();
	}

	void wait_and_pop(T &value)
	{
		std::unique_lock<std::mutex> writelock(m_mutex);
		m_condition_var.wait(writelock, [this]{return !m_data.empty();});

		value = m_data.front();
		m_data.pop();
	}

	void pop(T &value)
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		if(m_data.empty()) throw std::exception("Threadsafe queue is empty!");

		value = m_data.front();
		m_data.pop();
	}

	std::shared_ptr<T>  pop()
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		if(m_data.empty()) throw std::exception("Threadsafe queue is empty!");

		std::shared_ptr<T> const res(std::make_shared<T>(m_data.front()));
		m_data.pop();

		return res;
	}

	bool empty()
	{
		std::lock_guard<std::mutex> writelock(m_mutex);
		bool res = m_data.empty();
		return res;
	}
};


class hierarchical_mutex
{
	std::mutex internal_mutex;
	unsigned long const hierarchy_value;
	unsigned long previous_hierarchy_value;
	static  unsigned long this_thread_hierarchy_value;
	void check_for_hierarchy_violation()
	{
		if(this_thread_hierarchy_value <= hierarchy_value)
		{
			throw std::logic_error("mutex hierarchy violated");
		}
	}
	void update_hierarchy_value()
	{
		previous_hierarchy_value=this_thread_hierarchy_value;
		this_thread_hierarchy_value=hierarchy_value;
	}
public:
	explicit hierarchical_mutex(unsigned long value):
	hierarchy_value(value),
		previous_hierarchy_value(0)
	{}

	void lock()
	{
		check_for_hierarchy_violation();
		internal_mutex.lock();
		update_hierarchy_value();
	}
	void unlock()
	{
		this_thread_hierarchy_value=previous_hierarchy_value;
		internal_mutex.unlock();
	}
	bool try_lock()
	{
		check_for_hierarchy_violation();
		if(!internal_mutex.try_lock())
			return false;
		update_hierarchy_value();
		return true;
	}
};
//unsigned long
//	hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);




