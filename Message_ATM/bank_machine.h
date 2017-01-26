#pragma once
#include "withdraw.h"
#include <iostream>

class bank_machine
{
	messaging::receiver incoming;
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
				incoming.wait()
					.handle<verify_pin>(
						[&](verify_pin const& msg)
				{
					if (msg.pin == "1937")
					{
						msg.atm_queue.send(pin_verified());
					}
					else
					{
						msg.atm_queue.send(pin_incorrect());
					}
				}
						)
					.handle<withdraw>(
						[&](withdraw const& msg)
				{
					if (msg.m_amount <= m_balance)
					{
					msg.m_atm_queue.send(withdraw_ok());
						m_balance -= msg.m_amount;
					}
					else
					{
						msg.m_atm_queue.send(withdraw_denied());
					}
				}
						)
					.handle<get_balance>(
						[&](get_balance const& msg)
				{
					msg.m_atm_queue.send(::balance(m_balance));
				}
						)
					.handle<withdrawal_processed>(
						[&](withdrawal_processed const& msg)
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
		return incoming;
	}
};


class interface_machine
{
	messaging::receiver incoming;
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
				incoming.wait()
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
						std::lock_guard<std::mutex> lk(iom);
						std::cout << "Insufficient funds" << std::endl;
					}
				}
						)
					.handle<display_enter_pin>(
						[&](display_enter_pin const& msg)
				{
					{
						std::lock_guard<std::mutex> lk(iom);
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
						std::lock_guard<std::mutex> lk(iom);
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
						std::lock_guard<std::mutex> lk(iom);
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
						std::lock_guard<std::mutex> lk(iom);
						std::cout << "Withdrawal cancelled"
							<< std::endl;
					}
				}
						)
					.handle<display_pin_incorrect_message>(
						[&](display_pin_incorrect_message const& msg)
				{
					{
						std::lock_guard<std::mutex> lk(iom);
						std::cout << "PIN incorrect" << std::endl;
					}
				}
						)
					.handle<eject_card>(
						[&](eject_card const& msg)
				{
					{
						std::lock_guard<std::mutex> lk(iom);
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
		return incoming;
	}
};

class atm
{
	messaging::receiver incoming;
	messaging::sender bank;
	messaging::sender interface_hardware;
	void (atm::*state)();
	std::string account;
	unsigned withdrawal_amount;
	std::string pin;
	void process_withdrawal()
	{
		incoming.wait()
			.handle<withdraw_ok>(
				[&](withdraw_ok const& msg)
		{
			interface_hardware.send(issue_money(withdrawal_amount));
			bank.send(withdrawal_processed(account, withdrawal_amount));
			state = &atm::done_processing;
		}
				)
			.handle<withdraw_denied>(
				[&](withdraw_denied const& msg)
		{
			interface_hardware.send(display_insufficient_funds());
			state = &atm::done_processing;
		}
				)
			.handle<cancel_pressed>(
				[&](cancel_pressed const& msg)
		{
			bank.send(
				cancel_withdrawal(account, withdrawal_amount));
			interface_hardware.send(
				display_withdrawal_cancelled());
			state = &atm::done_processing;
		}
		);
	}

	void process_balance()
	{
		incoming.wait()
			.handle<balance>(
				[&](balance const& msg)
		{
			interface_hardware.send(display_balance(msg.m_amount));
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
		incoming.wait()
			.handle<withdraw_pressed>(
				[&](withdraw_pressed const& msg)
		{
			withdrawal_amount = msg.amount;
			bank.send(withdraw(account, msg.amount, incoming));
			state = &atm::process_withdrawal;
		}
				)
			.handle<balance_pressed>(
				[&](balance_pressed const& msg)
		{
			bank.send(get_balance(account, incoming));
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
		incoming.wait()
			.handle<pin_verified>(
				[&](pin_verified const& msg)
		{
			state = &atm::wait_for_action;
		}
				)
			.handle<pin_incorrect>(
				[&](pin_incorrect const& msg)
		{
			interface_hardware.send(
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
		incoming.wait()
			.handle<digit_pressed>(
				[&](digit_pressed const& msg)
		{
			unsigned const pin_length = 4;
			pin += msg.digit;
			if (pin.length() == pin_length)
			{
				bank.send(verify_pin(account, pin, incoming));
				state = &atm::verifying_pin;
			}
		}
				)
			.handle<clear_last_pressed>(
				[&](clear_last_pressed const& msg)
		{
			if (!pin.empty())
			{
				pin.pop_back();
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
		interface_hardware.send(display_enter_card());
		incoming.wait()
			.handle<card_inserted>(
				[&](card_inserted const& msg)
		{
			account = msg.account;
			pin = "";
			interface_hardware.send(display_enter_pin());
			state = &atm::getting_pin;
		}
		);
	}

	void done_processing()
	{
		interface_hardware.send(eject_card());
		state = &atm::waiting_for_card;
	}

	// 	atm(atm const&)=delete;
	// 	atm& operator=(atm const&)=delete;

	atm(atm const&) {};
	atm& operator=(atm const&) {};

public:
	atm(messaging::sender bank_,
		messaging::sender interface_hardware_) :
		bank(bank_), interface_hardware(interface_hardware_)
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
		return incoming;
	}
};