#include <vector>
#include "qed_lib.h"

typedef std::vector<pid_t> pidVec_t;
pidVec_t pids;

enum E_MessageType
{
	MT_AttachProcess = 1,
	MT_BroadcastStuff = 2
};

QED_PREPARE_CTRL_C_HANDLER(bShouldRun);

QED::message_queue inputMessages;	//server-perspective
QED::message_queue outputMessages;	//server-perspective

int writer_process(void* data)
{
	char buffer[1024];
	while(bShouldRun)
	{
		std::memset(buffer, 0, 1024*sizeof(char));
		std::cin.getline(buffer, 1024);

		inputMessages.send_message(buffer, MT_BroadcastStuff);
	}
	return 0;
}

int main()
{
	QED_INSTALL_CTRL_C_HANDLER;

	inputMessages.set_key(45);
	inputMessages.attach();

	outputMessages.set_key(46);
	outputMessages.attach();

	char buffer[1024];
	std::memset(buffer, 0, 1024*sizeof(char));
	std::sprintf(buffer, "%d", getpid());

	std::cout << "Client " << getpid() << ": Attaching to Server..." << std::endl;
	//1) send greeting message to server
	inputMessages.send_message(buffer, MT_AttachProcess);

	//2) spawn writer
	std::cout << "Client " << getpid() << ": Spawning writer..." << std::endl;
	QED::spawn_process(writer_process, NULL);

	//3) recv process-specific stuff
	while(bShouldRun)
	{
		if(outputMessages.receive_message(buffer, NULL, getpid()))
		{
			std::cout << "Client " << getpid() << ": Received following message: " << buffer << std::endl;
		}
		else
		{
			std::cout << "Client " << getpid() << ": Aborting due to spatial anomaly." << std::endl;
			bShouldRun = false;
		}
	}

	return 0;
}
