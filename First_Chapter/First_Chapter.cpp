// First_Chapter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <Commdlg.h>
#include <winbase.h>
#include <wingdi.h>
#include <dwmapi.h>
#include <algorithm>
#include <numeric>
#include <string>
#include <functional>
#include "Second_Chapter.h"
#include <future>
#include <thread>
#include <memory>
#include <direct.h>
#include <stdlib.h>
#include <list>
#include <atomic>
#include <utility>
#include <type_traits>

#define NUM 1000
#define TWOPI (2 * 3.14159)

#define  OUTPUT_FILE_PATH "D:\\LEARN\\Project\\files\\first_chap.txt"

void write_file(const std::string &str)
{
	std::ofstream myfile(OUTPUT_FILE_PATH, std::ios::out | std::ios::app );
	if(myfile.is_open())
	{
		myfile<<"Output: "<< str.c_str() <<std::endl;
		myfile.close();
	}
}

typedef struct func
{
	int& i;

	func(int& i_):i(i_){}

	void func_work(int i_var)
	{
		std::string s = std::to_string(i_var);
		write_file(s);
	}

	void operator()()
	{
		for(unsigned j=0;j<1000;++j)
		{
			func_work(i);
		}
	}

} func;

class thread_guard
{
private:
	std::thread& t;

	//c++ in VS2012 cannot use = delete
	//so I move here
	thread_guard(thread_guard const&);
	thread_guard& operator=(thread_guard const&);
public:
	explicit thread_guard(std::thread &_in_t): t(_in_t)
	{}

	~thread_guard()
	{
		if(t.joinable())
		{
			t.join();
		}
	}

	//c++ in VS2012 cannot use = delete
	/*thread_guard(thread_guard const&)=delete;
	thread_guard& operator=(thread_guard const&)=delete;*/

};


class Scoped_Thread
{
private:
	std::thread &m_thread;
	Scoped_Thread(const Scoped_Thread &t);
	Scoped_Thread & operator=(const Scoped_Thread &t);
public:
	explicit Scoped_Thread( std::thread &t_thread): m_thread(std::move(t_thread))
	{
		if(!m_thread.joinable())
			throw std::logic_error("Invalid thread");
	}

	~Scoped_Thread()
	{
		if(m_thread.joinable())
			m_thread.join();
	}
};

struct do_something
{

	do_something(int i)
	{}

	void operator()() 
	{

	}
};

template<typename Iterator,typename T>
struct accumulate_block
{
	void operator()(Iterator first,Iterator last,T& result)
	{
		result = std::accumulate(first,last,result);
	}
};
template<typename Iterator,typename T>
T parallel_accumulate(Iterator first,Iterator last,T init)
{
	unsigned long const length=std::distance(first,last);
	if(!length) //1
		return init;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =	(length + min_per_thread-1)/ min_per_thread; //2
	unsigned long const hardware_threads =	std::thread::hardware_concurrency();
	unsigned long const num_threads =	std::min( hardware_threads != 0 ? hardware_threads : 2, max_threads); //3
	unsigned long const block_size = length/num_threads; //4
	std::vector<T> results(num_threads);
	std::vector<std::thread> threads( num_threads-1 );//5
	Iterator block_start = first;
	for(unsigned long i=0;i < ( num_threads-1 );++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end,block_size); //6
		threads[i] = std::thread( //7
			accumulate_block<Iterator,T>(),
			block_start, block_end, std::ref(results[i]));
		block_start = block_end; //8
	}
	accumulate_block<Iterator,T>()(
		block_start, last, results[num_threads-1]); //9

	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join)); //10
	return std::accumulate(results.begin(), results.end(), init); //11
}


void show_number(thread_safe_condition_queue<int> &safe_queue)
{
	int t(1);
	while(t != 0)
	{
		safe_queue.wait_and_pop(t);

		std::cout<<" number = "<< t << std::endl;
	}
}

void input_number(thread_safe_condition_queue<int> &safe_queue)
{
	int t (1);
	while (t!= 0)
	{
		//fflush(stdin);
		std::cout<<" input: ";

		std::cin>>t;

		safe_queue.push(t);
	}
}

void output_string()
{
	std::cout<<"\n Output syncall"<<std::endl;
}

//const std::string & getstr()
//{
//	std::string s("abc xyz");
//	return s;
//}

int twice(int m)
{
	return m*2;
}

void accumulate(std::vector<int>::iterator first,
				std::vector<int>::iterator last,
				std::promise<int> accumulate_promise)
{
	int sum = std::accumulate(first, last, 0);
	accumulate_promise.set_value(sum);  // Notify future
}

extern std::promise<int> accumulate_promise;
#define	TEMP_DUMP_FOLDER_C				"C:\\I3_Temp"

void thread1()
{
	static int i = 0;
	const char * folder = TEMP_DUMP_FOLDER_C;
	int error = errno;
	if (mkdir(folder) == -1)
	{
		std::string s ("\n");
		s.append(std::to_string(i++)).append(" Create = ");
		error = errno;
		switch (errno)
		{
		default:

		case ENOENT: // No path found
			OutputDebugString("\n Create = 0");
			break;
		case EEXIST: // Path already exists
			break;

		}
		s.append(std::to_string(error));

		//OutputDebugString(s.c_str());
		std::cout<< s.data();
	}


}

std::ostream& operator<<(std::ostream& ostr, const std::list<int>& list)
{
	for (auto &i : list) {
		ostr << " " << i;
	}
	return ostr;
}

template<typename T>
struct sorter
{
	struct chunk_to_sort
	{
		std::list<T> data;
		std::promise<std::list<T> > promise;
		//virtual chunk_to_sort() = default;
		chunk_to_sort(chunk_to_sort&& other)
			: data(std::move(other.data))
			, promise(std::move(other.promise))
		{}
	};

	thread_safe_condition_queue<chunk_to_sort> chunks;
	std::vector<std::thread> threads;
	unsigned const max_thread_count;
	std::atomic<bool> end_of_data;

	sorter():
		max_thread_count(std::thread::hardware_concurrency()-1),
		end_of_data(false)
	{}

	~sorter()
	{
		end_of_data=true;
		for(unsigned i=0;i<threads.size();++i)
		{
			threads[i].join();
		}
	}

	void try_sort_chunk()
	{
		std::shared_ptr<chunk_to_sort > chunk=chunks.pop();
		if(chunk)
		{
			sort_chunk(chunk);
		}
	}

	std::list<T> do_sort(std::list<T>& chunk_data)
	{
		if(chunk_data.empty())
		{
			return chunk_data;
		}

		std::list<T> result;
		result.splice(result.begin(), chunk_data, chunk_data.begin());
		T const& partition_val = *result.begin();

		typename std::list<T>::iterator divide_point =
			std::partition(chunk_data.begin(),chunk_data.end(),
			[&](T const& val){return val<partition_val;});

		chunk_to_sort new_lower_chunk;
		new_lower_chunk.data.splice(new_lower_chunk.data.end(),
			chunk_data,chunk_data.begin(),
			divide_point);

		std::future<std::list<T> > new_lower=
			new_lower_chunk.promise.get_future();
		chunks.push(std::move(new_lower_chunk));
		if(threads.size()<max_thread_count)
		{
			threads.push_back(std::thread(&sorter<T>::sort_thread,this));
		}

		std::list<T> new_higher(do_sort(chunk_data));

		result.splice(result.end(),new_higher);
		while(new_lower.wait_for(std::chrono::seconds(0)) !=
			std::future_status::ready)
		{
			try_sort_chunk();
		}

		result.splice(result.begin(),new_lower.get());
		return result;
	}

	void sort_chunk(std::shared_ptr<chunk_to_sort > const& chunk)
	{
		chunk->promise.set_value(do_sort(chunk->data));
	}

	void sort_thread()
	{
		while(!end_of_data)
		{
			try_sort_chunk();
			std::this_thread::yield();
		}
	}
};

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if(input.empty())
	{
		return input;
	}
	sorter<T> s;
	return s.do_sort(input);
}

#include "Future_Promise.h"

int _tmain(int argc, _TCHAR* argv[])
{
// 	std::list<int> list1 ;
// 	for (int i = 10000; i > 0; i--)
// 	{
// 		list1.push_back(i);
// 	}
// 	std::list<int> list2 = parallel_quick_sort<int> (list1);
// 
// 	std::for_each(list2.begin(), list2.end(),[](int const &i){std::cout<<" "<<i << "";});

	father_thread();

	getchar();

}
//int _tmain(int argc, _TCHAR* argv[])
//{
//	//std::vector<int> numbers /*= { 1, 2, 3, 4, 5, 6 }*/;
//	//for (size_t i =0; i < 1000; i++)
//	//{
//	//	numbers.push_back(i);
//	//}
//	//
//	//std::future<int> accumulate_future = accumulate_promise.get_future();
//	//std::thread work_thread(accumulate, numbers.begin(), numbers.end(),
//	//	std::move(accumulate_promise));
//	//accumulate_future.wait();  // wait for result
//	//std::cout << "result=" << accumulate_future.get() << '\n';
//	//work_thread.join();  // wait for thread completion
//	int num = 1000;
//	std::vector<std::thread> s;
//	for (int i = 0; i<num;i++)
//	{
//		s.push_back(std::move(std::thread(thread1)));
//	}
//
//	for (int i =0; i < num;i++)
//	{
//		if(s[i].joinable())
//			s[i].join();
//	}
//
//	getchar();
//	return 0;
//}

