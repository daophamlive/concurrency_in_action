#include "message_queue.h"
#include <memory>
namespace messaging
{
	class close_queue
	{};

	class dispatcher
	{
		std::shared_ptr<message_queue>  m_sp_queue;
		bool m_chained;
	private:
		/*dispatcher(dispatcher const&)=delete;
		dispatcher& operator=(dispatcher const&)=delete;*/
		dispatcher(dispatcher const&);
		dispatcher& operator=(dispatcher const&);
		
		template<typename Dispatcher, typename Msg, typename Func>
		friend class TemplateDispatcher;

		void wait_and_dispatch()
		{
			for(;;)
			{
				auto msg = m_sp_queue->wait_and_pop();
				dispatch(msg);
			}
		}
		bool dispatch(std::shared_ptr<message_base> const& msg)
		{
			if(dynamic_cast<wrapped_message<close_queue>*>(msg.get()))
			{
				throw close_queue();
			}
			return false;
		}

	public:
		dispatcher(dispatcher&& other):
			m_sp_queue(other.m_sp_queue),m_chained(other.m_chained)
		{
			other.m_chained = true;
		}
		
		explicit dispatcher(std::shared_ptr<message_queue> _sp_q):
			m_sp_queue(_sp_q), m_chained(false)
		{}
		
		template<typename Message, typename Func>
		TemplateDispatcher<dispatcher, Message,Func>			handle(Func&& f)
		{
			return TemplateDispatcher<dispatcher, Message, Func>(m_sp_queue,this,std::forward<Func>(f));
		}

		~dispatcher()
		{
			if(!m_chained)
			{
				wait_and_dispatch();
			}
		}
	};
}
