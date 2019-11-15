#include "causality.h"
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <list>
#include <unistd.h>

using namespace std;
int counter = 1;
int machines, connections = 1;

int v[10];
int pid;
pthread_mutex_t l1;


//Each seperate machine data

struct servernode{
	int p_id;
	long sock_fd;
	long p1;
	struct sockaddr_in servernode_addr;
	struct hostent *nserver;
};
list<string> queue1;
struct servernode p[10];
// accepting connections
void *acceptconn(void *f1) 				
{
int i = 0, n;
char buf [256];
socklen_t client_length;
long new_sock_fd[10];
struct sockaddr_in client_addr;
client_length = sizeof(client_addr);
struct servernode p1[10];
while(counter < machines)					
{
	new_sock_fd[i] = accept((long)f1,(struct sockaddr *) &client_addr,&client_length);
             //creating buffer to store contents
	bzero(buf ,256);			
	stringstream ss, ss1, ss2;
	ss << p[0].p_id;
	string tmpstr = ss.str();			
	strcpy(buf ,tmpstr.c_str());			 
               // Sending This machine's process ID to connected machine
	n = send(new_sock_fd[i],buf ,strlen(buf ), 0);	
		if (n < 0) cout<<"error"<<endl;
		bzero(buf ,256);
		//reading the socket
		recv(new_sock_fd[i],buf ,255, 0);		
		
		ss1 << buf ;
		string tmpstr1 = ss1.str();
	
		p1[i].p_id = atoi(tmpstr1.c_str());

		cout << "Connected to Machine ID <" << p1[i].p_id << ">, ";

		n = send(new_sock_fd[i],"ID received",11, 0);
		if (n < 0) cout<<"ERROR writing to socket"<<endl;

		bzero(buf ,256);
		// Reading the socket
		recv(new_sock_fd[i],buf ,255, 0);			
		
		ss2 << buf ;
		string tmpstr2 = ss2.str();
	
		p1[i].p1 = atoi(tmpstr2.c_str());		

		cout << "with p1 number <" << p1[i].p1 << ">" << endl;

		n = send(new_sock_fd[i],"Port received",13, 0);
		if (n < 0) cout<<"unable to write to socket"<<endl;

		p1[i].sock_fd = new_sock_fd[i];			

		i++;							
		counter++;							
	}

	for(int k=0; k < i; k++)				
	{
		p[connections] = p1[k];
		connections++;
	}
	
}
//function to verify the causal ordering 
int causalCheck(string tmpstr)
{
int arr[10];					
int id;				
		
stringstream ss1(tmpstr);
	for(int i=0; i < machines; i++)			
	{
		ss1 >> arr[i];
	}
			
ss1 >> id;
        
int flag = 1;					
//verifying the causal conditions
	for(int i=0; i < machines; i++)
	{
		if(i == (id - 1))
		{
			if(arr[i] != (v[i] + 1))
			{
				flag = 0;
				break;
			}
		}
		else
		{
			if(arr[i] > (v[i]))	
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
//Store the messages and send when it satisfies the causal ordering
int bufQ()
{
int j;
int k = 0;
	for(list<string> :: iterator iter = queue1.begin(); iter != queue1.end(); )
	{
//verifying the causal ordering 
		 j = causalCheck(*iter);
		if(j > 0)	
		{
			k = 1;
			cout << "\nBuffered message is sent to machine : " << j << endl;
		
			v[j - 1]++;
			cout << "\tVector timestamp : ";
			cout<<"(";
			for(int i = 0; i < (machines - 1); i++)
			{
				cout << v[i] << ",";
			}
			cout << "[" << v[machines - 1] << ")" << "\n";
		
			

			iter = queue1.erase(iter);
			break;

		}
		else
		{
		iter++;
		}
	}
return k;
}

//implmentation of receiving, the multicast message without any causal voilations
void *receiveMessage(void *f1) 
{
int n;
//creating file descriptor for socket
long sock_fd = (long)f1;		
char buf [256];	
while(1)
{

	int arr[10];					
	int id;						
	//buffer to store the timestamps sent.
	bzero(buf ,256);		
	
	srand(time(0));
	sleep(rand()%5);
        // Receiving vector timestamp
	int r_fd = recv(sock_fd,buf ,sizeof(buf ), 0);	
	
	if(r_fd < 0)
	{
		cout<<"Unable to read from socket"<<endl;	
	}
	else
	{
		
		string tmpstr;                      
		stringstream ss;			
		ss.str("");
		ss << buf ;
		
		tmpstr = ss.str();
	
		int k = causalCheck(tmpstr);
			
		if(k > 0)					
		{
			// If conditions are satisfied deliver the msg
			cout << "\nMulticast MSG from <Machine (" << k << ")>" << endl;
		
			v[k-1]++;
			cout << "\tVector Timestamp : (";
			for(int i = 0; i < (machines - 1); i++)
			{
				cout << "[" << v[i] << "]" << ",";
			}
			cout << "[" << v[machines - 1] << "]" << ")\n";
				pthread_mutex_lock(&l1);
			if(!queue1.empty())				 
			{
				int res;
				do {
					res = bufQ();
				} while(res == 1);	
					
			}
			pthread_mutex_unlock(&l1);
			
		} 
		else							
		{
			//verifying causaulity for the buffered messages after some msgs
			if(!queue1.empty())	
			{//locking the critical section
				pthread_mutex_lock(&l1);
				int res, flag = 0;
				do {
					res = bufQ();
					if(res == 1)		
					{
						if(flag == 0)
						{
							int recheck = causalCheck(tmpstr);	
							//If both conditions satisfy message will be sent
							if(recheck > 0)				
							{	
								flag = 1;
								cout << "\nMulticast Message received from <Machine (" << recheck << ")>" << endl;
								v[recheck - 1]++;
								cout << "\tVector Clock: (";
								for(int i = 0; i < (machines - 1); i++)
								{
									cout << "[" << v[i] << "]" << ",";
								}
								cout << "[" << v[machines - 1] << "]" << ")\n";
							}
						}
					}
				} while(res == 1);
				pthread_mutex_unlock(&l1);
				if(flag == 0)
				{		
					cout << "\nMulticast message is buffered" << endl;
					pthread_mutex_lock(&l1);
					// buffering the message	
					queue1.push_back(tmpstr);			
					pthread_mutex_unlock(&l1);
					}	
			}
			else
			{
				cout << "\nMulticast Message is buffered" << endl;
					pthread_mutex_lock(&l1);
				// Buffer message if it does not satify conditions			
				queue1.push_back(tmpstr);				
				pthread_mutex_unlock(&l1);
			}
		}
		}
	}
}

//implementation of sending multicast message to other machines or processes
void *sendMessage(void *arg) 				
{
int n;
char buf [256];
	while(1)
	{	
		string action;
		cout << "Type 'send' and enter to multicast message ";
		cin >> action;
		if(action=="send")
		{
			int k = 0;	
			while(k<100)	
			{
				k++;

				v[pid-1]++;					


				for(int i=0; i < machines; i++)
				{
					if(i == (pid - 1))
					{
						continue;
					}
					else
					{
						//creating buffer to store the timestamp to be sent
						bzero(buf ,256);			
						stringstream ss;
						string tmpstr;
	
						ss.str("");
						ss << v[0];
	
						tmpstr = ss.str();				
						strcpy(buf ,tmpstr.c_str());
	
						strcat(buf ," ");
						//storing the vector clock into the buffer
						for(int j=1; j < machines; j++)
						{
							ss.str("");
							ss << v[j];
						
							tmpstr = ss.str();
										
							strcat(buf ,tmpstr.c_str());		
						
							strcat(buf ," ");
	
						}
						ss.str("");
						ss << pid;
	
						tmpstr = ss.str();
						//storing the process id into the buffer to send				
						strcat(buf ,tmpstr.c_str());					
						n = send(p[i].sock_fd,buf ,sizeof(buf ), 0);
						if (n < 0) cout<<"unable to write to socket"<<endl;
					}
				}

		
				cout << "\nMulticast Message Sent from <Machine (" << pid << ")>" << endl;
		
				srand(time(0));
				sleep(rand()%5);		
				cout << "\tVector Timestamp: [";
				for(int i = 0; i < (machines - 1); i++)
				{
					cout  << v[i] << ",";
				}
				cout << "[" << v[machines - 1] << "]" << "]\n";
	
				

			}
		}
	}
}

int main(int argc, char *argv[])
{

long p1, new_sock_fd[10], p2;
int action, ret_val;
char buf [256];
long f1;
socklen_t client_length;	
struct sockaddr_in serv_addr, client_addr;
//thread1 for send, thread2 for receive and for other connections thread3
pthread_t thread1,thread2, thread3;		
	if (argc < 2) {
		fprintf(stderr,"please provide the port as well during execution\n");
		exit(1);
	}

	if(pthread_mutex_init(&l1, NULL) != 0)		
	{
		cout << "mutex init has failed" << endl;
	}
	// Reading data related to machines from the file(number of machines)
	ifstream myfile("num_machines.txt");			

	string line;
	
	if(myfile.is_open())
	{
		getline(myfile, line);				
		istringstream ss(line);
		//storing number of machines information
		ss >> machines;
		myfile.close();
	}
	else
	{
	 cout << "Unable to open file"; 
	}
	
	f1 = socket(AF_INET, SOCK_STREAM, 0);
	
	if(f1 < 0)
	{ 
		cout<<"unable to open socket"<<endl;
	}
//creating buffer to store the server address
bzero((char *) &serv_addr, sizeof(serv_addr));
//converting string to integer
p2 = atoi(argv[1]);
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
//converting host to network byte order
serv_addr.sin_port = htons(p2);	
	int reuse = 1;
	//enable to reuse socket addres port 
	if (setsockopt(f1, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)	
		perror("setsockopt(SO_REUSEADDR) failed");

	#ifdef SO_REUSEPORT
	if (setsockopt(f1, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
		perror("setsockopt(SO_REUSEPORT) failed");
	#endif

	if(bind(f1, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		cout<<"Unable to bind"<<endl;

	listen(f1,10);

	cout << "Enter the machine number? ";
	cin >> pid;
	p[0].p_id = pid;
	p[0].p1 = p2;
	p[0].sock_fd = f1;

	int ret = pthread_create(&thread3, NULL, acceptconn, (void*)f1);
	cout << "press 1 to connect to other machines else 2 and enter ";
	cin >> action;
	if(action == 1)
	{
		cout << "give no. of machines to connect:  ";
		cin >> connections;
		for(int i=1; i <= connections; i++)
		{
			cout << "give port of Machine to connect: ";
			cin >> p[i].p1;

			p[i].sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	
			if(p[i].sock_fd < 0) 
				cout<<"Unable to open socket"<<endl;

			p[i].nserver = gethostbyname("localhost");
	
			bzero((char *) &p[i].servernode_addr, sizeof(p[i].servernode_addr));
			p[i].servernode_addr.sin_family = AF_INET;
			bcopy((char *)p[i].nserver->h_addr, (char *)&p[i].servernode_addr.sin_addr.s_addr, p[i].nserver->h_length);
			p[i].servernode_addr.sin_port = htons(p[i].p1);


			if(connect(p[i].sock_fd,(struct sockaddr *) &p[i].servernode_addr,sizeof(p[i].servernode_addr)) < 0) 
				cout<<"unable connecting"<<endl;

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
	
			ret_val = send(p[i].sock_fd,buf ,strlen(buf ), 0);	// Sending This machine's process ID to connected machine
			if (ret_val < 0) cout<<"unable to write to socket"<<endl;

			bzero(buf ,256);
     			ret_val = recv(p[i].sock_fd,buf ,255, 0);

			bzero(buf ,256);

			ss2 << p[0].p1;
			string tmpstr2 = ss2.str();				// Converting Port Number from long to string or char array
			strcpy(buf ,tmpstr2.c_str());				// Now converting from string to const char *
	
			ret_val = send(p[i].sock_fd,buf ,strlen(buf ), 0);	// Sending This machine's Port Number to connected machine
			if (ret_val < 0) cout<<"Unable to write to socket"<<endl;

			bzero(buf ,256);
     			ret_val = recv(p[i].sock_fd,buf ,255, 0);

			counter++;
		}
		connections++;								// Incrementing the counter for any processes still accepting new connections
	}

	while(counter < machines)							// Waiting till all the processes are connected
	{
		continue;
	}
	cout << "Connected machines: " << endl;
	cout << "\tID\tPort" << endl;

//--------------------All the machines are connected ---------------------------------------

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
		cout << "\tconnected to " << p[i].p_id << " through port : \t" << p[i].p1 << "\n";
	}

	
	for(int i=0; i < machines; i++)							// Spawning Multicast Receive Threads
	{
		if(i == (pid - 1))
		{
			continue;
		}
		else
		{
			pthread_create(&thread2, NULL, receiveMessage, (void *)p[i].sock_fd);
		}
	}

	pthread_create(&thread1, NULL, sendMessage, NULL);				// Spawning Multicast Send Thread

	while(1)
	{
		continue;
	}
//destroying the locks
	pthread_mutex_destroy(&l1);					

}

