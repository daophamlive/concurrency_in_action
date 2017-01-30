#include "message_queue.h"
namespace messaging
{
	class dispatcher
	{
		message_queue * m_p_queue;
		bool m_chained;
	private:
		/*dispatcher instances cannot be copied*/
		dispatcher(dispatcher const&) = delete;
		dispatcher& operator=(dispatcher const&) = delete;
		/*dispatcher(dispatcher const&) {};
		dispatcher& operator=(dispatcher const&) {};*/

		/*Allow TemplateDispatcher instances to access the internals*/
		template<typename Dispatcher, typename Msg, typename Func>
		friend class template_dispatcher;

		void wait_and_dispatch()
		{
			/*Loop, waiting for and dispatching messages*/
			for (;;)
			{
				if (m_p_queue)
				{
					auto msg = m_p_queue->wait_and_pop();
					dispatch(msg);
				}
			}
		}

		/*dispatch() checks for a close_queue message, and throws*/
		bool dispatch(std::shared_ptr<message_base> const& msg)
		{
			if (dynamic_cast<wrapped_message<close_queue>*>(msg.get()))
			{
				throw close_queue();
			}
			return false;
		}

	public:

		/*dispatcher instances can be moved*/
		dispatcher(dispatcher&& other) :
			m_p_queue(other.m_p_queue), m_chained(other.m_chained)
		{
			/*The source mustn't wait for messages*/
			other.m_chained = true;
		}

		explicit dispatcher(message_queue *_sp_q) :
			m_p_queue(_sp_q), m_chained(false)
		{}

		template<typename Message, typename Func>
		template_dispatcher<dispatcher, Message, Func> handle(Func&& f)
		{
			/*Handle a specific type of message with a TemplateDispatcher*/
			return template_dispatcher<dispatcher, Message, Func>(m_p_queue, this, std::forward<Func>(f));
		}

		~dispatcher() noexcept(false) /*The destructor might throw exceptions*/
		{
			if (!m_chained)
			{
				wait_and_dispatch();
			}
		}
	};

	template<typename PreviousDispatcher, typename Msg, typename Func>
	class template_dispatcher
	{
		message_queue  *m_p_queue;
		PreviousDispatcher *m_p_prev;
		Func m_func;
		bool m_chained;
		template_dispatcher(template_dispatcher const&) = delete;
		template_dispatcher& operator=(template_dispatcher const&) = delete;
		/*template_dispatcher(template_dispatcher const&) {};
		template_dispatcher& operator=(template_dispatcher const&) {};*/

		/*TemplateDispatcher instantiations are friends of each other*/
		template<typename Dispatcher, typename OtherMsg, typename OtherFunc>
		friend class template_dispatcher;

		void wait_and_dispatch()
		{
			for (;;)
			{
				if (m_p_queue)
				{
					auto msg = m_p_queue->wait_and_pop();
					if (dispatch(msg)) /*If we handle the message, break out of the loop*/
						break;
				}
			}
		}

		bool dispatch(std::shared_ptr<message_base> const& _msg)
		{

			//Check the message type,	and call the function
			if (wrapped_message<Msg>* wrapper =
				dynamic_cast<wrapped_message<Msg>*>(_msg.get()))
			{
				m_func(wrapper->m_contents);
				return true;
			}
			else
			{
				return m_p_prev->dispatch(_msg); /*Chain to the previous dispatcher*/
			}
		}

	public:
		template_dispatcher(template_dispatcher&& other) :
			m_p_queue(other.m_p_queue), m_p_prev(other.m_p_prev), m_func(std::move(other.m_func)),
			m_chained(other.m_chained)
		{
			other.m_chained = true;
		}

		template_dispatcher(message_queue *q_, PreviousDispatcher *prev_, Func&& f_) :
			m_p_queue(q_), m_p_prev(prev_), m_func(std::forward<Func>(f_)), m_chained(false)
		{
			prev_->m_chained = true;
		}

		template<typename OtherMsg, typename OtherFunc>
		template_dispatcher<template_dispatcher, OtherMsg, OtherFunc>
			handle(OtherFunc&& of) //Additional handlers can be chained
		{
			return template_dispatcher<template_dispatcher, OtherMsg, OtherFunc>(
				m_p_queue, this, std::forward<OtherFunc>(of));
		}

		~template_dispatcher() noexcept(false)
		{
			if (!m_chained)
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
	private:
		message_queue* m_p_msg_queue;//sender is wrapper around queue pointer

	public:
		sender() :m_p_msg_queue(nullptr) /*Default-constructed sender has no queue*/
		{}

		explicit sender(message_queue *sp_queue) :
			m_p_msg_queue(sp_queue) /*Allow construction from pointer to queue*/
		{

		}
		template<typename Message>
		void send(Message const &_msg)
		{
			if (m_p_msg_queue)
				m_p_msg_queue->push(_msg);//Sending pushes	message on the queue
		}

	};


	/************************************************************************/
	/* receiver                                                                    */
	/************************************************************************/
	class receiver
	{
		message_queue m_msg_queue; //A receiver owns the queue

	public:
		/*Allow implicit conversion to a sender that references the queue*/
		operator sender()
		{
			return sender(&m_msg_queue); 
		}

		dispatcher wait() //Waiting for a queue creates a dispatcher
		{
			return dispatcher(&m_msg_queue);
		}
	};

	
}
