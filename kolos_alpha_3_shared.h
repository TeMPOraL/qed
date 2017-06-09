#ifndef __KOLOS_ALPHA_3_SHARED_H__
#define __KOLOS_ALPHA_3_SHARED_H__

#include "qed_lib.h"

struct pizza_message_t
{
	enum E_MessageType
	{
		MT_RequestPlace,	//client asks for place
		MT_Allowed,			//client is allowed to sit
		MT_Disallowed,		//client is not allowed to sit
		MT_Leaving			//client is leaving the pizza house
	};

	E_MessageType msgType;

	int clientCount;
	pid_t clientPID;
	key_t clientMessageQueue;
};

#endif //__KOLOS_ALPHA_3_SHARED_H__
