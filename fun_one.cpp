#include "qed_lib.h"

int Cc_cnt = 0;
int SIGPIPE_cnt = 0;
volatile bool bShouldRun = true;	//volatile prevents compiler from optimizing-out the active-sleep loop in main program

void handle_sigint(int)
{
	++Cc_cnt;
	if(Cc_cnt == 5)
	{
		char c;
		do
		{
			std::cout << "Quit [y/n]: " << std::endl;
			std::cin >> c;
		}
		while( (c != 'y') && (c != 'n'));
		//std::cout << "out of while" << std::endl;
		if(c == 'y')
		{
			bShouldRun = false;
		}
	}
}

void handle_sigpipe(int)
{
	++SIGPIPE_cnt;
}

void* sigpipe_thread(void* )
{
	QED::pipe pipeStuff;

	pipe(pipeStuff.descriptors);

	close(pipeStuff.output);	//close the pipe other end

	while(bShouldRun)
	{
		sleep( std::rand()%5 + 2 );
		pipeStuff.write("hello!");	//generate sigpipe
	}

	return NULL;
}

void handle_totrzecie(int)
{
	std::cout << "SIGINT: " << Cc_cnt << std::endl << "SIGPIPE: " << SIGPIPE_cnt << std::endl;
}

int main()
{
	signal(SIGINT, handle_sigint);
	signal(SIGPIPE, handle_sigpipe);
	signal(SIGTSTP, handle_totrzecie);

	std::srand(std::time(NULL));

	pthread_t thread;
	pthread_create(&thread, NULL, sigpipe_thread, NULL);

	QED_ACTIVE_SLEEP_WHILE(bShouldRun);

	pthread_join(thread, NULL);

	std::cout << "End of app, thread joined." << std::endl;
	return 0;
}
