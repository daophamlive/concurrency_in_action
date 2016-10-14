#pragma once
#include <vector>
#include <thread>

class join_threads
{
private:
	std::vector<std::thread>& m_threads;
public:
	explicit join_threads(std::vector<std::thread> &_threads):m_threads(_threads)
	{

	}

	~join_threads()
	{
		for (size_t i = 0; i< m_threads.size(); i++)
		{
			if(m_threads[i].joinable())
			{
				m_threads[i].join();
			}
		}
	}
};