namespace messaging
{
	struct message_base
	{
		virtual ~message_base()
		{}
	};


	template<typename Msg>
	struct wrapped_message:
		message_base
	{
		Msg m_contents;

		explicit wrapped_message(Msg const &_contents): m_contents(_contents)
		{

		}
	};
}