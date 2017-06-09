#ifndef __QED_LIB_H__
#define __QED_LIB_H__

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <sstream>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

namespace QED
{
	//================================================================
	//String utils
	//================================================================

	//Lexical cast - convert between types based on their
	//text representation. Uses std::stringstream as a
	//conversion buffer.
	//Credit for boost::lexical_cast for the idea.
	template <typename Target, typename Source>
	inline Target lexical_cast(Source src)
	{
		std::stringstream buffer;	//buffer
		buffer.unsetf(std::ios::skipws);	//don`t skip white spaces
		buffer << src;	//write source argument to buffer

		Target retVal;	//return value

		buffer.unsetf(std::ios::skipws);	//don`t skip white spaces
		buffer >> retVal;	//read returned value

		return retVal;	//return converted value
	}

	//lexical cast template specialization for astring
	template<>
	inline std::string lexical_cast(std::string src)
	{
		return src;
	}

	//================================================================
	//Synchronization tools for std::ostream's in multithreaded applications
	//================================================================
	class ostream_lock
	{
	public:
		pthread_mutex_t* myMutex;
		ostream_lock(pthread_mutex_t& mutex)
		{
			myMutex = &mutex;
		}
	};

	inline std::ostream& operator<<(std::ostream& stream, const ostream_lock& locker)
	{
		assert(locker.myMutex && "NULL mutex in QED::ostream_lock");
		pthread_mutex_lock(locker.myMutex);
		return stream;
	}

	class ostream_unlock
	{
	public:
		pthread_mutex_t* myMutex;
		ostream_unlock(pthread_mutex_t& mutex)
		{
			myMutex = &mutex;
		}
	};

	inline std::ostream& operator<<(std::ostream& stream, const ostream_unlock& unlocker)
	{
		assert(unlocker.myMutex && "NULL mutex in QED::ostream_unlock");
		stream << std::flush;
		pthread_mutex_unlock(unlocker.myMutex);
		return stream;
	}

	//================================================================
	//Time functions
	//================================================================
	inline unsigned long getTimeInMSEC()
	{
		timeval tv;
		    
		gettimeofday(&tv,NULL);
		return (static_cast<unsigned long>(tv.tv_sec) * 1000UL) +
				(static_cast<unsigned long>(tv.tv_usec) / 1000UL);
	}

	//================================================================
	//Various file descriptor operations.
	//================================================================
	inline bool pipe_has_data(int fd)
	{
		//special case
		if(fd == -1)
		{
			return false;
		}

		int rc;
		timeval tv;
		tv.tv_usec = 0;	//prep time limit
		tv.tv_sec = 0;
		fd_set fds;
		
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		
		rc = select(fd+1, &fds, NULL, NULL, &tv);
		
		//we have input
		if(rc > 0)
		{
			return true ;
		}

		//we have timeout
		else if(rc == 0)
		{
			return false;
			//return true;
		}

		//we have an error
		else
		{
			assert(0 && "select error");
		}
		return false;
	}

	inline void write(int fd, const std::string& str)
	{
		size_t buffLen = str.size() + 1;
		char* buffer = new char[buffLen];

		std::strcpy(buffer, str.c_str());

		::write(fd, buffer, std::strlen(buffer));

		delete[] buffer;
	}

	std::string read(int fd, unsigned int bytes)
	{
		char* buffer = new char[bytes+1];
		std::memset(buffer, 0, (bytes+1)*sizeof(char));
		::read(fd, buffer, bytes+1);

		std::string retStr(buffer);

		delete[] buffer;
		return retStr;
	}

	//================================================================
	//Various file operations.
	//================================================================
	inline bool fileRunnable(const std::string& name)
	{	
		return (access(name.c_str(), F_OK | X_OK) == 0);
	}
	
	inline bool fileReadable(const std::string& name)
	{
		return (access(name.c_str(), F_OK | R_OK) == 0);;
	}


	//================================================================
	//Hic sunt leones.
	//================================================================
#define QED_KEY_FROM_CURRENT_FILE(Letter) ftok(__FILE__, Letter)

//YES, it DOES export "i" into the "what" expression. It is documented,
//so it can be used.
#define QED_DOTIMES(N,what) for(int i = 0 ; i < N ; ++i) { what; }

#define QED_ACTIVE_SLEEP_WHILE(Something) while(Something) {}
#define QED_ACTIVE_SLEEP_UNTIL(Something) while(!(Something)) {}

#define QED_PREPARE_CTRL_C_HANDLER(name) volatile bool name = true; void qed_sigint_handler(int) { name = false; }
#define QED_INSTALL_CTRL_C_HANDLER signal(SIGINT, qed_sigint_handler);

	//pointer to process function
	typedef int (*processFunctionPtr_t)(void*);

	inline int spawn_process(processFunctionPtr_t ptrFun, void* args = NULL)
	{
		pid_t pid = fork();
		if(pid == 0)
		{
			//child process
			std::exit(ptrFun(args));
			return -1;
		}
		else
		{
			return pid;
		}
		assert(0 && "QED::spawn_process - should never get here.");
		return -1;
	}

	inline int spawn_process_with_rebinded_io(processFunctionPtr_t ptrFun, void* args, int input, int output)
	{
		pid_t pid = fork();
		if(pid == 0)
		{
			//std::cout << "This is child, arming stdin with: " << input << " and stdout with: " << output << std::endl;
			//drop this input ;)
			close(STDIN_FILENO);
			close(STDOUT_FILENO);

			//redirect stdout and stdin
			dup2(input, STDIN_FILENO);
			dup2(output, STDOUT_FILENO);

			//child process
			std::exit(ptrFun(args));

			return -1;
		}
		else
		{
			close(input);
			close(output);
			return pid;
		}
		assert(0 && "QED::spawn_process - should never get here.");
		return -1;
	}

	struct pipe
	{
		union
		{
			struct
			{
				int output;	//from here you read
				int input;	//here you write
			};
			int descriptors[2];
		};

		//write data to pipe
		void write(const std::string& str)
		{
			QED::write(input, str);
		}

		//read data from pipe
		std::string read(unsigned int bytes = 1024)
		{
			return QED::read(output, bytes);
		}
	};

	class message_queue
	{
	protected:
		key_t key;
		int id;

	public:
		inline void set_key(key_t newKey)
		{
			key = newKey;
		}

		void create()
		{
			id = msgget(key, 0664 | IPC_CREAT);
			assert( (key != -1) && "Failed to create message queue.");

			//std::cout << "post-create-queue " << id << " / e: " << errno << " " << std::strerror(errno) << std::endl;
		}

		void destroy()
		{
			int res = msgctl(id, IPC_RMID, NULL);
			assert( (res != -1) && "Failed to destroy message queue");
		}

		void attach()
		{
			id = msgget(key, 0664);
			assert( (key != -1) && "Failed to get a message queue.");

			//std::cout << "post-attach-queue " << id << " / e: " << errno << " " << std::strerror(errno) << std::endl;
		}

		template<typename T>
		void send_message(const T& mesg, long messageType = 1)
		{
			struct msg
			{
				long msgType;
				T realMsg;
			} message;

			message.msgType = messageType;

			std::memcpy(&message.realMsg, &mesg, sizeof(T));
			msgsnd(id, &message, sizeof(T), 0);

		}

		template<typename T>
		bool receive_message(T& mesg, long* messageType = NULL, long listenTo = 0)
		{
			struct msg
			{
				long msgType;
				T realMsg;
			} message;

			int res = msgrcv(id, &message, sizeof(T), listenTo, 0);
			std::memcpy(&mesg, &message.realMsg, sizeof(T));

			if(messageType)
			{
				*messageType = message.msgType;
			}

			if(res == -1)
			{
				return false;
			}
			return true;
		}
	};

	class semaphore
	{
	protected:
		key_t key;
		int id;

	public:

		inline void set_key(key_t newKey)
		{
			key = newKey;
		}

		inline void create(int val = 1)
		{
			id = semget(key, 1, 0644 | IPC_CREAT);
			assert( (id != -1) && "Failed to create semaphore.");

			semctl(id,0,SETVAL,1);
		}

		inline void attach()
		{
			id = semget(key, 1, 0644);
			assert( (id != -1) && "Failed to acquire semaphore.");
		}

		inline void destroy()
		{
			int res = semctl(id, 0, IPC_RMID);
			assert( (res != -1) && "Failed to destroy semaphore.");
		}

		inline void lock(int howMuch = 1)
		{
			sembuf operation;
			operation.sem_op = -howMuch;
			operation.sem_num = 0;
			operation.sem_flg = 0;

			semop(id, &operation, 1);
		}

		inline void unlock(int howMuch = 1)
		{
			sembuf operation;
			operation.sem_op = +howMuch;
			operation.sem_num = 0;
			operation.sem_flg = 0;

			semop(id, &operation, 1);
		}

	};
	
	class shared_memory
	{
	protected:
		size_t memorySize;
		key_t key;
		int id;

		void* memPtr;

		semaphore myLocker;

	public:

		shared_memory()
		{
			memorySize = 0;
			memPtr = NULL;
		}

		inline void set_key(key_t newKey)
		{
			key = newKey;
			myLocker.set_key(newKey + 12);	//UGLY HACK-O-RAMA FIXME HACK
		}

		inline void create(size_t size)
		{
			id = shmget(key, size, 0644 | IPC_CREAT);
			assert( (id != -1) && "Failed to create shared memory segment.");

			memPtr = shmat(id, 0, 0);
			assert((memPtr != reinterpret_cast<void*>(-1)) && "Failed to get access to shared memory segment.");
			memorySize = size;

			myLocker.create();
		}

		inline void attach(size_t size)
		{
			id = shmget(key, size, 0644);
			assert( (id != -1) && "Failed to get shared memory segment.");

			memPtr = shmat(id, 0, 0);
			assert((memPtr != reinterpret_cast<void*>(-1)) && "Failed to get access to shared memory segment.");
			memorySize = size;

			myLocker.attach();
		}

		inline void detach()
		{
			int res = shmdt(memPtr);
			assert( (res != -1) && "Failed to detach from shared memory segment.");
		}

		inline void destroy()
		{
			int res = shmctl(id, IPC_RMID, NULL);
			assert( (res != -1) && "Failed to destroy shared memory segment.");

			myLocker.destroy();
		}

		inline size_t memory_size()
		{
			return memorySize;
		}

		inline void* get_memory()
		{
			assert( (memPtr != NULL) && "NULL shared memory pointer.");
			return memPtr;
		}

		//lock access to shared memory segment
		inline void lock()
		{
			//semaphores are needed to do this.
			//assert(0 && "Locking will be implemented next tuesday.");
			myLocker.lock();
		}

		inline void unlock()
		{
			//semaphores are needed to do this.
			//assert(0 && "Unlocking will be implemented next tuesday.");
			myLocker.unlock();
		}
	};

	//function that was needed somewhere, but is too ugly to include
	//in problem solution code ;)
	const char* compare_three_strings(const char* alpha, const char* beta, const char* gamma)
	{
		if( strcmp(alpha, beta) < 0 )
		{
			if(strcmp(alpha, gamma) < 0)
			{
				return alpha;
			}
			else
			{
				return gamma;
			}
		}
		else
		{
			if(strcmp(beta, gamma) < 0)
			{
				return beta;
			}
			else
			{
				return gamma;
			}
		}
	}
}

#endif //__QED_LIB_H__
