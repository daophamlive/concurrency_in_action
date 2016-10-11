#pragma once
#include <vector>
#include <thread>

class Join_Threads
{
private:
	std::vector<std::thread>& m_threads;
public:
	explicit Join_Threads(std::vector<std::thread> &_threads):m_threads(_threads)
	{

	}

	~Join_Threads()
	{
		for (size_t i =0; i< m_threads.size(); i++)
		{
			if(m_threads[i].joinable())
			{
				m_threads[i].join();
			}
		}
	}
};