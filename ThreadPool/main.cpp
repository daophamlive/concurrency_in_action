// ThreadPool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <functional>
#include "simplest_threadpool.h"
#include <conio.h>

extern int text_count = 0;

void print_text()
{
	for (int i = 0; i < 10; i ++)
	{
		text_count ++;

		std::cout << text_count << " ";
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	//std::function<void()> function (print_text);
	simplest_threadpool pool;
	function_wrapper f(print_text);
	 pool.submit(f);
	

	_getch();
	return 0;
}

