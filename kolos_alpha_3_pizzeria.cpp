#include "qed_lib.h"
#include "kolos_alpha_3_shared.h"

void handle_client(const pizza_message_t& msg)
{
	std::cout << "PizzaHouse: Handling client " << msg.clientPID << "." << std::endl;
	if(msg.msgType == pizza_message_t::MT_Leaving)
	{
		std::cout << "PizzaHouse: Client " << msg.clientPID << " has left the house." << std::endl;

		//TODO mark table as free
	}
	else if(msg.msgType == pizza_message_t::MT_RequestPlace)
	{
		std::cout << "PizzaHouse: Client " << msg.clientPID << " requested a place." << std::endl;

		//TODO check if available

		//send a response to the client
		QED::message_queue clientMsgQueue;
		clientMsgQueue.set_key(msg.clientMessageQueue);
		clientMsgQueue.attach();

		pizza_message_t pmt;
		pmt.msgType = pizza_message_t::MT_Allowed;

		clientMsgQueue.send_message(pmt);

		//TODO mark seats as taken, ect.
	}
	else
	{
		assert(0 && "Should never get this message from client.");
	}
}

bool bShouldRun = true;

void signal_handler(int)
{
	bShouldRun = false;
}

int main()
{
	//set up CTRL+C
	signal(SIGINT, signal_handler);

	QED::message_queue pizzaMsgQueue;
	pizzaMsgQueue.set_key(42);	//magic pizza house key
	pizzaMsgQueue.create();

	pizza_message_t pmt;

	std::cout << "Starting the Pizza House!" << std::endl;

	//main pizza loop
	while(bShouldRun)
	{
		if(pizzaMsgQueue.receive_message(pmt))
		{
			handle_client(pmt);
		}
	}

	std::cout << "Pizza House is closing!" << std::endl;

	pizzaMsgQueue.destroy();
	return 0;
}
