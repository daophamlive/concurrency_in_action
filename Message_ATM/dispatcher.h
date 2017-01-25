#include "message_queue.h"
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
		friend class template_dispatcher;

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
		template_dispatcher<dispatcher, Message,Func> handle(Func&& f)
		{
			return template_dispatcher<dispatcher, Message, Func>(m_sp_queue,this,std::forward<Func>(f));
		}

		~dispatcher()
		{
			if(!m_chained)
			{
				wait_and_dispatch();
			}
		}
	};


	/************************************************************************/
	/* sender                                                                     */
	/************************************************************************/
	class sender
	{
	public:
		sender();
		explicit sender(std::shared_ptr<message_queue> sp_queue):
		m_sp_msg_queue(sp_queue)
		{

		}
		template<typename Message>
		void send(Message const &_msg)
		{
			if(m_sp_msg_queue.get())
				m_sp_msg_queue->push(_msg);
		}

	private:
		std::shared_ptr<message_queue> m_sp_msg_queue;
	};


	/************************************************************************/
	/* receiver                                                                    */
	/************************************************************************/
	class receiver
	{
		std::shared_ptr<message_queue> m_queue;

	public:
		operator sender()
		{
			return sender(m_queue);
		}

		dispatcher wait()
		{
			return dispatcher(m_queue);
		}
	};
}
