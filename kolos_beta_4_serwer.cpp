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

int main()
{
	QED_INSTALL_CTRL_C_HANDLER;

	QED::message_queue inputMessages;
	inputMessages.set_key(45);
	inputMessages.create();

	QED::message_queue outputMessages;
	outputMessages.set_key(46);
	outputMessages.create();

	long recvdMsgType;

	char buffer[1024];

	while(bShouldRun)
	{
		if(inputMessages.receive_message(buffer, &recvdMsgType) )
		{
			if(recvdMsgType == MT_AttachProcess)
			{
				std::cout << "Server: Attaching process " << buffer << "." << std::endl;
				pids.push_back(QED::lexical_cast<pid_t>(buffer));
			}
			else if(recvdMsgType == MT_BroadcastStuff)
			{
				std::cout << "Server: Broadcasting stuff: " << buffer << "." << std::endl;
				QED_DOTIMES(static_cast<int>(pids.size()), outputMessages.send_message(buffer, pids[i]));
			}
			else
			{
				assert(0 && "Unknown message type.");
			}
		}
		else
		{
			std::cout << "Server: Aborting due tu spatial anomaly." << std::endl;
			bShouldRun = false;
		}

	}

	outputMessages.destroy();
	inputMessages.destroy();
}
