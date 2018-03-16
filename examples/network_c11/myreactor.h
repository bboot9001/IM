#ifndef _H_MYREACTOR_
#define _H_MYREACTOR_
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>

#define  WORKER_THREAD_NUM   5

class myreactor
{
public:
	myreactor()
	{

	};
	~myreactor()
	{

	};

public:
	bool init(const char* ip,int nport);
	bool unint();
	bool close_client(int client);

	static void* main_loop(void* p);

private:
	myreactor(const myreactor& rhs);
	myreactor& operator=(const myreactor& rhs);


	bool create_server_listener(const char* ip,int port);
	static void accept_thread_proc(myreactor* pReatcor);
	static void worker_thread_proc(myreactor* pReatcor);

private:
	int  m_listenfd = 0;
	int  m_epollfd  = 0;
	bool m_bStop    = false;

	std::shared_ptr<std::thread> m_acceptthread;
	std::shared_ptr<std::thread> m_workerthreads[WORKER_THREAD_NUM];

	std::condition_variable  m_acceptcond;
	std::mutex               m_acceptmutex;

    std::condition_variable  m_workercond;
	std::mutex               m_workermutex;

	std::list<int>           m_listClients;

};
#endif