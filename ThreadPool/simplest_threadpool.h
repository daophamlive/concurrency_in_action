#pragma once
#include "threadsafe_queue.h"
#include <atomic>
#include <thread>
#include "Join_Threads.h"
class simplest_threadpool
{
private:
	/*both the done flag and the worker_queue must be declared before the threads vector,
	which must in turn be declared before the joiner.*/
	std::atomic<bool> m_done;
	threadsafe_queue<std::function<void()> > m_work_queue;
	std::vector<std::thread> m_threads;
	Join_Threads m_joiner_threads;

	void worker_thread()
	{
		while (!m_done)
		{
			std::function<void()>task;
			if(m_work_queue.try_pop(task))
			{
				task();
			}
			else
			{
				/*to take a small break and give another thread a chance to put some work
				on the queue before it tries to take some off again the next time around.*/
				std::this_thread::yield();
			}
		}
	}
public:
	simplest_threadpool(): m_done(false), m_joiner_threads(m_threads)
	{
		unsigned const thread_count = std::thread::hardware_concurrency();
		try
		{
			for (unsigned i = 0; i < thread_count; i ++)
			{
				m_threads.push_back(std::thread(&simplest_threadpool::worker_thread, this));
			}
		}
		catch (...)
		{
			m_done = true;
			throw std::exception("simplest_threadpool can't create thread.");
		}
		
	}
	~simplest_threadpool()
	{
		m_done = true;
	}

	template<typename FunctionType>
	void submit(FunctionType f)
	{
		m_work_queue.push(std::function<void()> (f));
	}
	
};
