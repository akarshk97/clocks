/* Processes send multicast messages to each other and a causal state of those messages is maintained  
   The port number of each process is passed as an argument */
#include "causality.h"
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <list>

using namespace std;

struct servernode{						// A structure for all the required information of individual process is maintained
	int p_id;
	long sock_fd;
	long port;
	struct sockaddr_in servernode_addr;
	struct hostent *nserver;
};

void error(const char *msg) 					// function to print error messages
{
	cout<<msg<<endl;
	exit(1);
}

int counter = 1;
int machines, noc = 1;
struct servernode p[10];
int v[10];
int pid;
pthread_mutex_t l1;
list<string> q;

void *acceptconn(void *sockfd) 				// thread function for accepting new connection request
{
	int i = 0, n;
	char buf [256];
	socklen_t client_len;
	long new_sock_fd[10];
	struct sockaddr_in client_addr;
	client_len = sizeof(client_addr);

	struct servernode p1[10];

	while(counter < machines)					// Running the Accept loop till desired number of processes have connected
	{
		new_sock_fd[i] = accept((long)sockfd,(struct sockaddr *) &client_addr,&client_len);

		bzero(buf ,256);			
		stringstream ss, ss1, ss2;
		ss << p[0].p_id;
		string tmpstr = ss.str();				// Converting Process ID from int to string or char array
		strcpy(buf ,tmpstr.c_str());				// Now converting from string to const char * and copying to buf 

		n = send(new_sock_fd[i],buf ,strlen(buf ), 0);	// Sending This machine's process ID to connected machine
		if (n < 0) error("error");


		bzero(buf ,256);
		recv(new_sock_fd[i],buf ,255, 0);			// Reading the machine ID of connected machine
		
		ss1 << buf ;
		string tmpstr1 = ss1.str();
	
		p1[i].p_id = atoi(tmpstr1.c_str());

		cout << "Connected to Machine ID <" << p1[i].p_id << ">, ";

		n = send(new_sock_fd[i],"ID received",11, 0);
		if (n < 0) error("ERROR writing to socket");

		bzero(buf ,256);
		recv(new_sock_fd[i],buf ,255, 0);			// Reading the port number of connected machine
		
		ss2 << buf ;
		string tmpstr2 = ss2.str();
	
		p1[i].port = atoi(tmpstr2.c_str());			// Saving the port number of connected machine

		cout << "with port number <" << p1[i].port << ">" << endl;

		n = send(new_sock_fd[i],"Port received",13, 0);
		if (n < 0) error("ERROR writing to socket");

		p1[i].sock_fd = new_sock_fd[i];				// Saving the Socket Descriptor of connected machine

		i++;							// counter for accepted connections
		counter++;							// counter for total connections
	}

	for(int k=0; k < i; k++)					// Storing the accepted process details into main datastructure of all processes
	{
		p[noc] = p1[k];
		noc++;
	}
	
}

int causalCheck(string tmpstr)
{
	int arr[10];					// Array for temporarily storing received vector clock
	int id;						// Integer to store ID of sender process
		
	stringstream ss1(tmpstr);

	for(int i=0; i < machines; i++)			// Getting the sender's vector clock from buf  into a temporary vector clock array
	{
		ss1 >> arr[i];
	}
			
	ss1 >> id;

	int flag = 1;					// Flag to check Causal Ordering of events

	for(int i=0; i < machines; i++)
	{
		if(i == (id - 1))
		{
			if(arr[i] != (v[i] + 1))	// Check for Causal Ordering Condition 1 violation
			{
				flag = 0;
				break;
			}
		}
		else
		{
			if(arr[i] > (v[i]))	// Check for Causal Ordering Condition 2 violation
			{
				flag = 0;
				break;
			}
		}
	}
	if(flag == 1)
	{
		return id;
	}
	else
	{
		return -1;
	}
}
int bufCheck()
{
	int flag = 0;
	for(list<string> :: iterator s = q.begin(); s != q.end(); )
	{
		int result = causalCheck(*s);
		if(result > 0)	// If both conditions satisfied, message is delivered to application
		{
			flag = 1;
			cout << "\n*****Message Delivered from buf  for <Machine (" << result << ")>*****" << endl;
		
			v[result - 1]++;


			cout << "\n---------------------------------------------" << endl;
			cout << "\tVector Clock: (";
			for(int i = 0; i < (machines - 1); i++)
			{
				cout << "[" << v[i] << "]" << ",";
			}
			cout << "[" << v[machines - 1] << "]" << ")\n";
		
			cout << "===============================================" << endl;

			s = q.erase(s);
			break;

		}
		else
		{
			s++;
		}
	}
	return flag;
}

void *MulticastRecv(void *sockfd) // thread function for receiving multicast message from particular process
{
	char buf [256];
	int n;
	long sock_fd = (long)sockfd;		// Socket FD for communicating with specific process
	
	while(1)
	{
		
		int arr[10];					// Array for temporarily storing received vector clock
		int id;						// Integer to store ID of sender process

		bzero(buf ,256);		
		
		srand(time(0));
		sleep(rand()%6);
	
		int r_fd = recv(sock_fd,buf ,sizeof(buf ), 0);	// Receive message from sender with their vector clock
		
		if(r_fd < 0)
		{
			cout<<"Error: Unable to read from socket"<<endl;		// Print respective message if there's an error while receving the message
		}
		else
		{
			stringstream ss;
			string tmpstr;
			ss.str("");
			ss << buf ;
			
			tmpstr = ss.str();
		
			int flag = causalCheck(tmpstr);
				
			if(flag > 0)					// If both conditions satisfied, message is delivered to application
			{
				cout << "\nMulticast MSG from <Machine (" << flag << ")>" << endl;
			
				v[flag - 1]++;


				cout << "\n---------------------------------------------" << endl;
				cout << "\tVector Clock: (";
				for(int i = 0; i < (machines - 1); i++)
				{
					cout << "[" << v[i] << "]" << ",";
				}
				cout << "[" << v[machines - 1] << "]" << ")\n";
	
				cout << "===============================================" << endl;

				pthread_mutex_lock(&l1);
				if(!q.empty())				 // If the buf  has some pending messages, check if they satisfy causality now!
				{
					int res;
					do {
						res = bufCheck();
					} while(res == 1);		// If a message from buf  is delivered, check for the causality of remaining messages in buf  
						
				}
				pthread_mutex_unlock(&l1);
				
			} 
			else							
			{
				
				if(!q.empty())				// If a message fails causal conditions, check if there are pending messages in buf . Check if they satisfy causality now!
				{
					pthread_mutex_lock(&l1);
					int res, flag = 0;
					do {
						res = bufCheck();
						if(res == 1)		// A message from buf  is delivered!
						{
							if(flag == 0)
							{
								int recheck = causalCheck(tmpstr);	// If message from buf  is delivered. Re-Check for causality of current message
								if(recheck > 0)				// If both conditions are satisfied, deliver current message to application
								{	
									flag = 1;
									cout << "\nMulticast Message Received from <Machine (" << recheck << ")>" << endl;
									v[recheck - 1]++;
	
									cout << "\n---------------------------------------------" << endl;
									cout << "\tVector Clock: (";
									for(int i = 0; i < (machines - 1); i++)
									{
										cout << "[" << v[i] << "]" << ",";
									}
									cout << "[" << v[machines - 1] << "]" << ")\n";
		
									cout << "===============================================" << endl;
								}
							}
						}
					} while(res == 1);
					pthread_mutex_unlock(&l1);
					if(flag == 0)
					{		
						cout << "\n##### Buffering the message #####" << endl;
						pthread_mutex_lock(&l1);	
						q.push_back(tmpstr);			// message is buffered to not violate the causal ordering
						pthread_mutex_unlock(&l1);
					}	
				}
				else
				{
					cout << "\n##### Buffering the message #####" << endl;

					pthread_mutex_lock(&l1);
					q.push_back(tmpstr);				// When any condition is not satisfied, message is buffered
					pthread_mutex_unlock(&l1);
				}
			}

		}

	}
}


void *MulticastSend(void *arg) 							// thread function for sending multicast message
{
	char buf [256];
	int n;
	while(1)
	{	
		int ans;
		cout << "Press 1 to multicast: ";
		cin >> ans;
		if(ans==1)
		{
			int k = 0;	
			while(k<100)	
			{
				k++;
				v[pid-1]++;					// Send message event corresponds to one vector clock increment


				for(int i=0; i < machines; i++)
				{
					if(i == (pid - 1))
					{
						continue;
					}
					else
					{
		
						bzero(buf ,256);			
						stringstream ss;
						string tmpstr;
	
						ss.str("");
						ss << v[0];
	
						tmpstr = ss.str();				// Converting Process ID from int to string or char array
						strcpy(buf ,tmpstr.c_str());
	
						strcat(buf ," ");
	
						for(int j=1; j < machines; j++)
						{
							ss.str("");
							ss << v[j];
	
							tmpstr = ss.str();			// Converting Process ID from int to string or char array
							strcat(buf ,tmpstr.c_str());		// Now converting from string to const char *
						
							strcat(buf ," ");
	
						}
						ss.str("");
						ss << pid;
	
						tmpstr = ss.str();				// Converting Process ID from int to string or char array
						strcat(buf ,tmpstr.c_str());					
	
						n = send(p[i].sock_fd,buf ,sizeof(buf ), 0);
						if (n < 0) error("ERROR writing to socket");
					}
				}

				srand(time(0));
				sleep(rand()%5);
		
				cout << "\nMulticast Message Sent from <Machine (" << pid << ")>" << endl;
				cout << "\n---------------------------------------------" << endl;
				cout << "\tVector Clock: (";
				for(int i = 0; i < (machines - 1); i++)
				{
					cout << "[" << v[i] << "]" << ",";
				}
				cout << "[" << v[machines - 1] << "]" << ")\n";
	
				cout << "=============================================" << endl;

			}
		}
	}
}

int main(int argc, char *argv[])
{

int ans, n;
long port;
char buf [256];
long sockfd, new_sock_fd[10], portno;
socklen_t client_len;	
struct sockaddr_in serv_addr, client_addr;
pthread_t mcastsend, mcastrecv, newconnections;		//threads for handling client requests
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	if(pthread_mutex_init(&l1, NULL) != 0)		//Initializing mutex object
	{
		cout << "mutex init has failed" << endl;
	}

	ifstream myfile("ProcInfo.txt");			// Taking number of processes from file

	string line;
	
	if(myfile.is_open())
	{
		getline(myfile, line);				// Reading file by line and storing the number of machines
		istringstream ss(line);
		ss >> machines;
		myfile.close();
	}
	else cout << "Unable to open file"; 

	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sockfd < 0) 
		error("ERROR opening socket");

bzero((char *) &serv_addr, sizeof(serv_addr));
portno = atoi(argv[1]);
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
serv_addr.sin_port = htons(portno);	
	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)	//To reuse socket address in case of crashes and failures
		perror("setsockopt(SO_REUSEADDR) failed");

	#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
		perror("setsockopt(SO_REUSEPORT) failed");
	#endif

	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd,10);

	cout << "What is your process ID? ";
	cin >> pid;
	p[0].p_id = pid;
	p[0].port = portno;
	p[0].sock_fd = sockfd;

	int ret = pthread_create(&newconnections, NULL, acceptconn, (void*)sockfd);
	
	cout << "---------------------------------------------" << endl;
	cout << "Establishing Connections" << endl;
	cout << "=============================================" << endl;
	
	cout << "Do you wish to connect to any machine? (yes = 1 / no = 2) ";
	cin >> ans;
	if(ans == 1)
	{
		cout << "Enter the number of machines to connect: ";
		cin >> noc;
		for(int i=1; i <= noc; i++)
		{
			cout << "Enter the port number of Machine to connect: ";
			cin >> p[i].port;

			p[i].sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	
			if(p[i].sock_fd < 0) 
				error("ERROR opening socket");

			p[i].nserver = gethostbyname("localhost");
	
			bzero((char *) &p[i].servernode_addr, sizeof(p[i].servernode_addr));
			p[i].servernode_addr.sin_family = AF_INET;
			bcopy((char *)p[i].nserver->h_addr, (char *)&p[i].servernode_addr.sin_addr.s_addr, p[i].nserver->h_length);
			p[i].servernode_addr.sin_port = htons(p[i].port);


			if(connect(p[i].sock_fd,(struct sockaddr *) &p[i].servernode_addr,sizeof(p[i].servernode_addr)) < 0) 
				error("ERROR connecting");

			bzero(buf ,256);
			recv(p[i].sock_fd,buf ,sizeof(buf ), 0);			//Reading the machine ID of connected machines
			
			stringstream ss, ss1, ss2;
			ss << buf ;
			string tmpstr = ss.str();

			p[i].p_id = atoi(tmpstr.c_str());

			cout << "Connected to machine ID: " << p[i].p_id << endl;
			
			bzero(buf ,256);
			
			ss1 << p[0].p_id;
			string tmpstr1 = ss1.str();				// Converting Process ID from int to string or char array
			strcpy(buf ,tmpstr1.c_str());				// Now converting from string to const char *
	
			n = send(p[i].sock_fd,buf ,strlen(buf ), 0);	// Sending This machine's process ID to connected machine
			if (n < 0) error("ERROR writing to socket");

			bzero(buf ,256);
     			n = recv(p[i].sock_fd,buf ,255, 0);

			bzero(buf ,256);

			ss2 << p[0].port;
			string tmpstr2 = ss2.str();				// Converting Port Number from long to string or char array
			strcpy(buf ,tmpstr2.c_str());				// Now converting from string to const char *
	
			n = send(p[i].sock_fd,buf ,strlen(buf ), 0);	// Sending This machine's Port Number to connected machine
			if (n < 0) error("ERROR writing to socket");

			bzero(buf ,256);
     			n = recv(p[i].sock_fd,buf ,255, 0);

			counter++;
		}
		noc++;								// Incrementing the counter for any processes still accepting new connections
	}

	while(counter < machines)							// Waiting till all the processes are connected
	{
		continue;
	}
	cout << "Connected machines: " << endl;
	cout << "\tID\tPort" << endl;

//--------------------Connections Completed ---------------------------------------

	for(int i=0; i < (machines-1); i++)						// Sorting the processes according to their ID values
	{
		for(int j =0; j < (machines -i-1); j++)
		{
			if(p[j].p_id > p[j+1].p_id)
			{
				swap(p[j], p[j+1]);
			}
		}
	}

	for(int i = 0; i < machines; i++)						// Printing the Process IDs and respective Port Numbers
	{
		cout << "\t" << p[i].p_id << "\t" << p[i].port << "\n";
	}

	cout << "\n===========================================" << endl;
	
	for(int i=0; i < machines; i++)							// Spawning Multicast Receive Threads
	{
		if(i == (pid - 1))
		{
			continue;
		}
		else
		{
			pthread_create(&mcastrecv, NULL, MulticastRecv, (void *)p[i].sock_fd);
		}
	}

	pthread_create(&mcastsend, NULL, MulticastSend, NULL);				// Spawning Multicast Send Thread

	while(1)
	{
		continue;
	}

	pthread_mutex_destroy(&l1);					// Code never reacher here, but still following good practice to destroy mutexes

}

