#include "qed_lib.h"
#include "kolos_alpha_3_shared.h"

int main()
{
	std::srand(std::time(NULL));
	int howManyOfUs = (rand()%3 ) + 1;	//1-3 clients
	pizza_message_t pmt;

	//1) create our recv-back queue
	QED::message_queue clientQueue;
	key_t myKey = QED_KEY_FROM_CURRENT_FILE( (getpid() % 128) + (getpid()/128)%128 ); 	//low chance of collision


	clientQueue.set_key(myKey);
	clientQueue.create();

	QED::message_queue pizzaQueue;
	pizzaQueue.set_key(42);	//42 = pizza key ;)
	pizzaQueue.attach();

	std::cout << "Client " << getpid() << ": come for pizza house, looking for place." << std::endl;

	//2) ask pizza house for place
	pmt.clientCount = howManyOfUs;
	pmt.clientPID = getpid();
	pmt.clientMessageQueue = myKey;
	pmt.msgType = pizza_message_t::MT_RequestPlace;

	pizzaQueue.send_message(pmt);

	//3) wait for response
	clientQueue.receive_message(pmt);

	//4a) if OK -> wait for random period and leave
	if(pmt.msgType == pizza_message_t::MT_Allowed)
	{
		std::cout << "Client " << getpid() << ": There's a place for me, so I bought the pizza and am currently eating." << std::endl;
		sleep(std::rand() % 15 + 1);
		std::cout << "Client " << getpid() << ": Finished my meal, I'm leaving the pizza house. " << std::endl;

		pmt.clientCount = howManyOfUs;
		pmt.clientPID = getpid();
		pmt.clientMessageQueue = myKey;
		pmt.msgType = pizza_message_t::MT_Leaving;
			
		pizzaQueue.send_message(pmt);
	}
	//4b) if not OK -> die
	else
	{
		std::cout << "Client " << getpid() << ": no place for me, I'm leaving." << std::endl;
	}

	clientQueue.destroy();

	return 0;
}
