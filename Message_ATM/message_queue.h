#include <queue>
#include <condition_variable>
#include <memory>
#include "message.h"
namespace messaging
{
	
	class message_queue
	{
	public:
		message_queue();
		~message_queue();

		template<typename T>
		std::shared_ptr<T> wait_and_pop()
		{
			std::unique_lock<std::mutex> writeLock(m_mutex);
			m_condition.wait(writeLock,[]{ return !m_queue.empty(); });
			auto sp_message = std::move(m_queue.front());
			m_queue.pop();
			return sp_message;
		}

		template<typename T>
		void push( T const &_msg)
		{
			std::lock_guard<std::mutex> writelock(m_mutex);
			m_queue.push(std::make_shared<wrapped_message<T>>(_msg));
			m_condition.notify_all();
		}

		bool empty()
		{
			std::lock_guard<std::mutex> readlock(m_mutex);
			return m_queue.empty();
		}

	private:
		std::queue<std::shared_ptr<message_base>> m_queue;
		std::condition_variable m_condition;
		std::mutex m_mutex;
	};
	
}