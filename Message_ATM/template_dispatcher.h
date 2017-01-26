#include "message_queue.h"
namespace messaging
{
	template<typename PreviousDispatcher,typename Msg,typename Func>
	class template_dispatcher
	{
		message_queue  *m_p_queue;
		PreviousDispatcher *m_p_prev;
		Func m_func;
		bool m_chained;
		/*template_dispatcher(template_dispatcher const&)=delete;
		template_dispatcher& operator=(template_dispatcher const&)=delete;*/
		template_dispatcher(template_dispatcher const&);
		template_dispatcher& operator=(template_dispatcher const&);

		template<typename Dispatcher,typename OtherMsg,typename OtherFunc>
		friend class template_dispatcher;
		void wait_and_dispatch()
		{
			for(;;)
			{
				auto msg=m_p_queue->wait_and_pop();
				if(dispatch(msg))
					break;
			}
		}
		bool dispatch(std::shared_ptr<message_base> const& _msg)
		{
			
				if(wrapped_message<Msg>* wrapper=
					dynamic_cast<wrapped_message<Msg>*>(_msg.get()))
				{
					m_func(wrapper->contents);
					return true;
				}
				else
				{
					return m_p_prev->dispatch(_msg);
				}
		}
	public:
		template_dispatcher(template_dispatcher&& other):
			m_p_queue(other.m_p_queue),m_p_prev(other.m_p_prev),m_func(std::move(other.m_func)),
			m_chained(other.m_chained)
		{
			other.m_chained=true;
		}

		template_dispatcher(message_queue *q_, PreviousDispatcher *prev_, Func&& f_):
			m_p_queue(q_), m_p_prev(prev_),  m_func(std::forward<Func>(f_)), m_chained(false)
		{
			prev_->m_chained=true;
		}

		template<typename OtherMsg,typename OtherFunc>
		template_dispatcher<template_dispatcher, OtherMsg, OtherFunc>
			handle(OtherFunc&& of)
		{
			return template_dispatcher<template_dispatcher, OtherMsg, OtherFunc>(
				m_p_queue,this, std::forward<OtherFunc>(of));
		}

		~template_dispatcher() /*noexcept(false)*/
		{
			if(!m_chained)
			{
				wait_and_dispatch();
			}
		}
	};
}