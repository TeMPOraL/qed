#include <algorithm>
#include "qed_lib.h"

//================================================================
//Shared section
//================================================================
QED::pipe alphaPipe;
QED::pipe alphaResultsPipe;

QED::message_queue betaQueue;
QED::message_queue alphaSharedQueue;

QED::shared_memory gammaSharedMem;

//================================================================
//Alpha section
//================================================================
int process_alpha(void* data)
{
	char buffer[1024];
	alphaSharedQueue.attach();

	while(true)
	{
		std::cin.getline(buffer, 1024);

		std::string napis(buffer);
		std::reverse(napis.begin(), napis.end());

		//grab other two
		alphaSharedQueue.receive_message(buffer);
		std::string napis2(buffer);

		alphaSharedQueue.receive_message(buffer);
		std::string napis3(buffer);

		//compare and select best:

		//send winner to main program
		//comparison function was too ugly to include in this elegant program,
		//so it went to QED lib ;)
		std::cout << QED::compare_three_strings(napis.c_str(), napis2.c_str(), napis3.c_str()) << std::flush;
	}

	return 0;
}

//================================================================
//Beta section
//================================================================
int process_beta(void* data)
{
//	std::cout << "Beta reporting!" << std::endl;

	betaQueue.attach();
	alphaSharedQueue.attach();

	char buffer[1024];

	while(true)
	{
		std::memset(buffer, 0, 1024*sizeof(char));
		
		//get data from msg queue
		betaQueue.receive_message(buffer);

		//cut out last two letters
		buffer[std::max(static_cast<int>(std::strlen(buffer)) - 2, 0)] = '\0';

		//send data via message queue to P1
		alphaSharedQueue.send_message(buffer);
	}

	return 0;
}

//================================================================
//Gamma section
//================================================================

void gamma_signal(int code)
{
	//do nothing
	return;
}

int process_gamma(void* data)
{
//	std::cout << "Gamma reporting!" << std::endl;

	gammaSharedMem.attach(1024);
	alphaSharedQueue.attach();

	char buffer[1024];

	//install signal
	signal(SIGUSR1, gamma_signal);

	while(true)
	{
		//sleep() for signal
		pause();

		//read from shared memory
		char* napis = reinterpret_cast<char*>(gammaSharedMem.get_memory());

		//cut out first two letters
		if(std::strlen(napis) < 2)
		{
			std::memset(buffer, 0, 1024);
		}
		else
		{
			std::memmove(buffer, napis+2, 1022);
		}

		//send to P1
		alphaSharedQueue.send_message(buffer);
	}
	gammaSharedMem.detach();

	return 0;
}

//================================================================
//Parent section
//================================================================
int main()
{
	std::cout << "Type \".\" to quit." << std::endl;
	pipe(alphaPipe.descriptors);
	pipe(alphaResultsPipe.descriptors);

	betaQueue.set_key(QED_KEY_FROM_CURRENT_FILE('B'));
	betaQueue.create();

	alphaSharedQueue.set_key(QED_KEY_FROM_CURRENT_FILE('A'));
	alphaSharedQueue.create();

	gammaSharedMem.set_key(QED_KEY_FROM_CURRENT_FILE('M'));
	gammaSharedMem.create(1024);

	int alphaPID =	QED::spawn_process_with_rebinded_io(process_alpha, NULL, alphaPipe.output, alphaResultsPipe.input);
	int betaPID =	QED::spawn_process(process_beta, NULL);
	int gammaPID =	QED::spawn_process(process_gamma, NULL);

	char buffer[1024];

	//TODO break on signal + kill all processes
	while(true)
	{
		std::cout << "> " << std::flush;
		std::cin.getline(buffer, 1024);

		//break on "."
		if(std::strcmp(buffer, ".") == 0)
		{
			std::cout << "] breaking up" << std::endl;
			kill(alphaPID, SIGTERM);
			kill(betaPID, SIGTERM);
			kill(gammaPID, SIGTERM);
			break;
		}

		//send to process alpha
		alphaPipe.write(std::string(buffer) + std::string("\n"));

		//send to process beta
		betaQueue.send_message(buffer);

		// send to process gamma
		memcpy(gammaSharedMem.get_memory(), buffer, 1024);
		kill(gammaPID, SIGUSR1);	//wake up gamma

		std::cout << "] waiting for result..." << std::endl;

		//hang on process alpha pipe to get result, then print it
		std::cout << alphaResultsPipe.read(1024) << std::endl;
	}

	gammaSharedMem.detach();
	gammaSharedMem.destroy();

	//wait for those 3 processes
	int dummy;
	QED_DOTIMES(3, wait(&dummy));

	alphaSharedQueue.destroy();
	betaQueue.destroy();

	return 0;
}
