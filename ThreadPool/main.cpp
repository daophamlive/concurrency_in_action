// ThreadPool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <functional>
#include "simplest_threadpool.h"
#include <conio.h>

extern int text_count = 0;

// struct print_function
// {
// 	void operator ()()
// 	{
// 		for (int i = 0; i < 10; i++)
// 		{
// 			text_count++;
// 
// 			std::cout << text_count << " ";
// 		}
// 	}
// };

int print_function()
{
	for (int i = 0; i < 10; i++)
	{
		text_count++;

		//std::cout << text_count << " ";
	}
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	simplest_threadpool pool;
	std::function<int()> f(print_function);
	std::vector<std::future<int>> v_res;
	int k = 0;
	for (int i = 0; i < 100000; i++)
	{
		std::future<int>  res = pool.submit(f);
		v_res.push_back(std::move(res) );
	}

	for (size_t i = 0; i < v_res.size(); i++)
	{
		k+= v_res[i].get();
	}
	std::cout << k << " ";
	_getch();
	return 0;
}

