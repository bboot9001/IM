#include <myreactor.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <list>
#include <errno.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <unistd.h>

#define min(a, b) ((a <= b) ? (a) : (b))

bool myreactor::create_server_listener(const char* ip,int port)
{
	m_listenfd = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);
	if (m_lisenfd == -1)
	{
		return false;
	}

	int on = 1;
	::setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,(char*)&on,sizeof(on));
	::setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEPORT,(char*)&on,sizeof(on));

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip);
	servaddr.sin_port   = htons(port);

	if (::bind(m_listenfd,(sockaddr*)&servaddr,sizeof(servaddr)) == -1)
	{
		return false;
	}

	if (::listen(m_listenfd,50) == -1)
	{
		return false;
	}	

	m_epollfd = epoll_create(1);
	if (m_epollfd == -1)
	{
		return false;
	}
     
     struct epoll_event e;
     memset(&e,0,sizeof(e));
     e.events = EPOLLIN | EPOLLRDHUP;
     e.data.fd = m_listenfd;
     if (::epoll_ctl(m_epollfd,EPOLL_CTL_ADD,m_listenfd,&e) == -1)
     {
     	return false;
     }

     return ture;
}

bool myreactor::init(const char* ip,int nport)
{
	if (!create_server_listener(ip,nport))
	{
		std::cout<<"Unable to bind:"<<ip<<":"<<nport<<"."<<std::endl;
		return false;
	}

	std::cout<<"main thread id="<<std::this_thread::get_id()<<std::endl;
	m_accpetthread.reset(new std::thread(myreactor::accept_thread_proc,this));

	for(auto& t : m_workthreads)
	{
		t.reset(new std::thread(myreactor::worker_thread_proc,this));
	}

	return true;
}

bool myreactor::unint()
{
	m_bStop = true;
	m_acceptcond.notify_one();
	m_workercond.notify_all();
	m_acceptthread.join();

	for(auto& t:m_workerthreads)
	{
		t->join();
	}

	::epoll_ctl(m_epollfd,EPOLL_CTL_DEL,m_listenfd,NULL);
	::shuntdown(m_listenfd,SHUT_RDWR);
	::close(M_listenfd);
	::close(m_epollfd);
	return true;
}

bool myreactor::close_client(int clientfd)
{
	if (::epoll_ctl(m_epollfd,EPOLL_CTL_DEL,clientfd,NULL) == -1)
	{
		std:cout<<"close client socket failed as call epoll_ctl faild"<<std::endl;
	}
	::close(clientfd);
	return true;
}

void* myreactor::main_loop(void* p)
{
	std::cout<<"main thread id"<<std::this_thread::get_id()<<std::endl;
	myreactor* pThis = static_cast<myreactor*>(p);

	while(!pThis->m_bStop)
	{
		struct epoll_event ev[1024];
		int n = ::epoll_wait(pThis->m_epollfd,ev,1024,10);

		if (n == 0)
		{
			continue;
		}
		else if (n < 0)
		{
			std::cout<<"epoll_wait error"<<std::endl;
			continue;
		}

		int m = min(n,1024);
		for(int i = 0;i < m;++i)
		{
			if (ev[i].data.fd == m_listenfd)
			{
				pThis->m_acceptcond.notifyone();
			}
			else
			{
				{
					std::unique_lock<std::mutex> guard(pThis->m_workermutex);
					pThis->m_listClients.push_back(ev[i].data.fd);
				}

				pThis->m_workercond.notifyone();
			}
		}

	}

	std::cout<<"main loop exit ...."<<std::endl;
	return NULL;
}

 void myreactor::accept_thread_proc(myreactor* pReatcor)
 {
 	std::cout<<"accept thread ,thread id ="<<std::this_thread::get_id()<<std::endl;

 	while(true)
 	{
 		int newfd;
 		struct sockaddr_in clientaddr;
 		socklen_t addrlen;
 		{
 			std::unique_lock<std::mutex> guard(pThis->m_acceptmutex);
 			pthis->m_acceptcond.wait(guard);
 			if (pThis->m_bStop)
 			{
 				/* code */
 				break;
 			}

 			newfd = ::accept(pThis->m_listenfd,(struct sockaddr*)&clientaddr,&addrlen);
 		}

 		if (newfd == -1)
 		{
 			/* code */
 			continue;
 		}

 		std::cout<<"new client connected:"<<::inet_ntoa(clientaddr.sin_addr)<<":"<<::ntohs(clientaddr.sin_port)
 		<<std::endl;

 		//setsockopt(newfd,SOL_SOCK,SOCK_NONBLOCK,)
 		int oldflags = ::fcntl(newfd,F_GETFL,0);
 		int newflag  = oldflags | O_NONBLOCK;
 		if (::fcntl(newfd,F_SETFL,newflag) == -1)
 		{
 			std::cout<<"fcntl error,oldflag ="<<oldflag<<", newflag ="<<newflag<<std::endl;
 			continue;
 			/* code */
 		}

 		struct epoll_event e;
 		memset(&e,0,sizeof(e));
 		e.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
 		e.data.fd = newfd;
 		if (::epoll_ctl(pThis->m_epollfd,EPOLL_CTL_ADD,newfd,&e) == -1)
 		{
 			/* code */
 			std::cout<<"epoll_ctl error, fd ="<<newfd<<std::endl;
 		}
 	}

 	std::cout<<"accept thread exit ..."<<std::endl;
 }

 void myreactor::worker_thread_proc(myreactor* pReatcor)
{
	std::cout<<"new worker thread , thread id ="<<std::this_thread::get_id()<<std::endl;

	while(true)
	{

		int clientfd;
		{
			std::unique_lock<std::mutex>  guard(pThis->m_workermutex);
			while(pThis->m_listClients.empty())
			{
				if (pThis->m_bStop)
				{
					std::cout<<"worker thread exit"<<std::endl;
					/* code */
					return;
				}
				pThis->m_workercond.wait(guard);
			}

			clientfd  = pThis->m_listClients.front();
			pThis->m_listClients.pop_front();
		}
		std::cout<<std::endl;
		std::string  strclientmsg;
		char buff[256];
		bool bError  = false;
		while(true)
		{
			memset(buff,0,sizeof(buff));
			int nRecv = ::recv(clientfd,buff,256,0);
			if (nRecv == -1)
			{
				/* code */
				if (errno == EWOULDBLOCK)
				{
					/* code */
					break;
				}
				else
				{
					std::cout<<"recv error ,client disconnected,fd ="<<clientfd<<std::endl;
					pThis->close_client(clientfd);
					bError = true;
					break;
				}


			}
			else if (nRecv == 0)
			{
			    std::cout << "peer closed, client disconnected, fd = " << clientfd << std::endl;
				pReatcor->close_client(clientfd);
				bError = true;
				break;
			}
			strclientmsg += buff;

			if (bError)
			{
				/* code */
				continue;
			}

        std::cout << "client msg: " << strclientmsg;

		//将消息加上时间标签后发回
		time_t now = time(NULL);
		struct tm* nowstr = localtime(&now);
		std::ostringstream ostimestr;
		ostimestr << "[" << nowstr->tm_year + 1900 << "-"
			<< std::setw(2) << std::setfill('0') << nowstr->tm_mon + 1 << "-"
			<< std::setw(2) << std::setfill('0') << nowstr->tm_mday << " "
			<< std::setw(2) << std::setfill('0') << nowstr->tm_hour << ":"
			<< std::setw(2) << std::setfill('0') << nowstr->tm_min << ":"
			<< std::setw(2) << std::setfill('0') << nowstr->tm_sec << "]server reply: ";

		strclientmsg.insert(0, ostimestr.str());

		while (true)
		{
			int nSent = ::send(clientfd, strclientmsg.c_str(), strclientmsg.length(), 0);
			if (nSent == -1)
			{
				if (errno == EWOULDBLOCK)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}
				else
				{
					std::cout << "send error, fd = " << clientfd << std::endl;
					pReatcor->close_client(clientfd);
					break;
				}

			}

			std::cout << "send: " << strclientmsg;
			strclientmsg.erase(0, nSent);

			if (strclientmsg.empty())
				break;
		}
		
		}
	}
}