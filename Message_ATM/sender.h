#include "message_queue.h"
namespace messaging
{
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
}