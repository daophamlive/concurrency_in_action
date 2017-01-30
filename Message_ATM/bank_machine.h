#pragma once
#include "withdraw.h"
#include <iostream>

class bank_machine
{
	messaging::receiver m_incoming;
	unsigned int m_balance;
public:
	bank_machine() :
		m_balance(199)
	{}

	void done()
	{
		get_sender().send(messaging::close_queue());
	}

	void run()
	{
		try
		{
			for (;;)
			{
				m_incoming.wait()
					.handle<verify_pin>(
						[&](verify_pin const& _msg)
				{
					if (_msg.pin == "1937")
					{
						_msg.atm_queue.send(pin_verified());
					}
					else
					{
						_msg.atm_queue.send(pin_incorrect());
					}
				}
						)
					.handle<withdraw>(
						[&](withdraw const& _msg)
				{
					if (_msg.m_amount <= m_balance)
					{
						_msg.m_atm_queue.send(withdraw_ok());
						m_balance -= _msg.m_amount;
					}
					else
					{
						_msg.m_atm_queue.send(withdraw_denied());
					}
				}
						)
					.handle<get_balance>(
						[&](get_balance const& _msg)
				{
					_msg.m_atm_queue.send(::balance(m_balance));
				}
						)
					.handle<withdrawal_processed>(
						[&](withdrawal_processed const& _msg)
				{
				}
						)
					.handle<cancel_withdrawal>(
						[&](cancel_withdrawal const& msg)
				{
				}
				);
			}
		}
		catch (messaging::close_queue const&)
		{
		}
	}

	messaging::sender get_sender()
	{
		return m_incoming;
	}
};


class interface_machine
{
	messaging::receiver m_incoming;
	std::mutex iom;
public:
	void done()
	{
		get_sender().send(messaging::close_queue());
	}
	void run()
	{
		try
		{
			for (;;)
			{
				m_incoming.wait()
					.handle<issue_money>(
						[&](issue_money const& msg)
				{
					{
						std::lock_guard<std::mutex> lk(iom);
						std::cout << "Issuing "
							<< msg.amount << std::endl;
					}
				}
						)
					.handle<display_insufficient_funds>(
						[&](display_insufficient_funds const& msg)
				{
					{
						//std::lock_guard<std::mutex> lk(iom);
						std::cout << "Insufficient funds" << std::endl;
					}
				}
						)
					.handle<display_enter_pin>(
						[&](display_enter_pin const& msg)
				{
					{
						//std::lock_guard<std::mutex> lk(iom);
						std::cout
							<< "Please enter your PIN (0-9)"
							<< std::endl;
					}
				}
						)
					.handle<display_enter_card>(
						[&](display_enter_card const& msg)
				{
					{
						//std::lock_guard<std::mutex> lk(iom);
						std::cout << "Please enter your card (I)"
							<< std::endl;
					}
				}
						)
					.handle<display_balance>(
						[&](display_balance const& msg)
				{
					{
						std::lock_guard<std::mutex> lk(iom);
						std::cout
							<< "The balance of your account is "
							<< msg.m_amount << std::endl;
					}
				}
						)
					.handle<display_withdrawal_options>(
						[&](display_withdrawal_options const& msg)
				{
					{
						//std::lock_guard<std::mutex> lk(iom);
						std::cout << "Withdraw 50? (w)" << std::endl;
						std::cout << "Display Balance? (b)"
							<< std::endl;
						std::cout << "Cancel? (c)" << std::endl;
					}
				}
						)
					.handle<display_withdrawal_cancelled>(
						[&](display_withdrawal_cancelled const& msg)
				{
					{
						//std::lock_guard<std::mutex> lk(iom);
						std::cout << "Withdrawal canceled"
							<< std::endl;
					}
				}
						)
					.handle<display_pin_incorrect_message>(
						[&](display_pin_incorrect_message const& msg)
				{
					{
						//std::lock_guard<std::mutex> lk(iom);
						std::cout << "PIN incorrect" << std::endl;
					}
				}
						)
					.handle<eject_card>(
						[&](eject_card const& msg)
				{
					{
						//std::lock_guard<std::mutex> lk(iom);
						std::cout << "Ejecting card" << std::endl;
					}
				}
				);
			}
		}
		catch (messaging::close_queue&)
		{
		}
	}

	messaging::sender get_sender()
	{
		return m_incoming;
	}
};

class atm
{
	messaging::receiver m_incoming;
	messaging::sender m_bank;
	messaging::sender m_interface_hardware;
	void (atm::*state)();
	std::string m_account;
	unsigned m_withdrawal_amount;
	std::string m_pin;
	void process_withdrawal()
	{
		m_incoming.wait()
			.handle<withdraw_ok>(
				[&](withdraw_ok const& msg)
		{
			m_interface_hardware.send(issue_money(m_withdrawal_amount));
			m_bank.send(withdrawal_processed(m_account, m_withdrawal_amount));
			state = &atm::done_processing;
		}
				)
			.handle<withdraw_denied>(
				[&](withdraw_denied const& msg)
		{
			m_interface_hardware.send(display_insufficient_funds());
			state = &atm::done_processing;
		}
				)
			.handle<cancel_pressed>(
				[&](cancel_pressed const& msg)
		{
			m_bank.send(
				cancel_withdrawal(m_account, m_withdrawal_amount));
			m_interface_hardware.send(
				display_withdrawal_cancelled());
			state = &atm::done_processing;
		}
		);
	}

	void process_balance()
	{
		m_incoming.wait()
			.handle<balance>(
				[&](balance const& msg)
		{
			m_interface_hardware.send(display_balance(msg.m_amount));
			state = &atm::wait_for_action;
		}
				)
			.handle<cancel_pressed>(
				[&](cancel_pressed const& msg)
		{
			state = &atm::done_processing;
		}
		);
	}

	void wait_for_action()
	{
		m_incoming.wait()
			.handle<withdraw_pressed>(
				[&](withdraw_pressed const& msg)
		{
			m_withdrawal_amount = msg.amount;
			m_bank.send(withdraw(m_account, msg.amount, m_incoming));
			state = &atm::process_withdrawal;
		}
				)
			.handle<balance_pressed>(
				[&](balance_pressed const& msg)
		{
			m_bank.send(get_balance(m_account, m_incoming));
			state = &atm::process_balance;
		}
				)
			.handle<cancel_pressed>(
				[&](cancel_pressed const& msg)
		{
			state = &atm::done_processing;
		}
		);
	}

	void verifying_pin()
	{
		m_incoming.wait()
			.handle<pin_verified>(
				[&](pin_verified const& msg)
		{
			state = &atm::wait_for_action;
		}
				)
			.handle<pin_incorrect>(
				[&](pin_incorrect const& msg)
		{
			m_interface_hardware.send(
				display_pin_incorrect_message());
			state = &atm::done_processing;
		}
				)
			.handle<cancel_pressed>(
				[&](cancel_pressed const& msg)
		{
			state = &atm::done_processing;
		}
		);
	}

	void getting_pin()
	{
		m_incoming.wait()
			.handle<digit_pressed>(
				[&](digit_pressed const& msg)
		{
			unsigned const pin_length = 4;
			m_pin += msg.digit;
			if (m_pin.length() == pin_length)
			{
				m_bank.send(verify_pin(m_account, m_pin, m_incoming));
				state = &atm::verifying_pin;
			}
		}
				)
			.handle<clear_last_pressed>(
				[&](clear_last_pressed const& msg)
		{
			if (!m_pin.empty())
			{
				m_pin.pop_back();
			}
		}
				)
			.handle<cancel_pressed>(
				[&](cancel_pressed const& msg)
		{
			state = &atm::done_processing;
		}
		);
	}

	void waiting_for_card()
	{
		m_interface_hardware.send(display_enter_card());
		m_incoming.wait()
			.handle<card_inserted>(
				[&](card_inserted const& msg)
		{
			m_account = msg.account;
			m_pin = "";
			m_interface_hardware.send(display_enter_pin());
			state = &atm::getting_pin;
		}
		);
	}

	void done_processing()
	{
		m_interface_hardware.send(eject_card());
		state = &atm::waiting_for_card;
	}

	 	atm(atm const&)=delete;
	 	atm& operator=(atm const&)=delete;

	/*atm(atm const&) {};
	atm& operator=(atm const&) {};*/

public:
	atm(messaging::sender bank_,
		messaging::sender interface_hardware_) :
		m_bank(bank_), m_interface_hardware(interface_hardware_)
	{}

	void done()
	{
		get_sender().send(messaging::close_queue());
	}

	void run()
	{
		state = &atm::waiting_for_card;
		try
		{
			for (;;)
			{
				(this->*state)();
			}
		}
		catch (messaging::close_queue const&)
		{
		}
	}

	messaging::sender get_sender()
	{
		return m_incoming;
	}
};