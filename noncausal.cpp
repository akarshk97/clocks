#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h> 
#include <pthread.h>
#include <ctime>
#include <list>

using namespace std;

struct servernode{						 
	long p1,sock_fd;
	int p_id;
	struct sockaddr_in servernode_addr;
	struct hostent *nserver;
};

void error(const char *msg) 					// function to print error messages
{
	cout<<msg<<endl;
	exit(1);
}

int counter = 1, machines, connections = 1;
struct servernode p[10];
int v[10], pid;

void *acceptconn(void *f1) 				// thread function for accepting new connection request
{
	int i = 0, n;
	char buf[256];
	socklen_t client_length;
	long new_sock_fd[10];
	struct sockaddr_in client_addr;
	client_length = sizeof(client_addr);

	struct servernode p1[10];

	while(counter < machines)					// Running the Accept loop till desired number of processes have connected
	{
		new_sock_fd[i] = accept((long)f1,(struct sockaddr *) &client_addr,&client_length);

		bzero(buf,256);			
		stringstream ss, ss1, ss2;
		ss << p[0].p_id;
		string tmpstr = ss.str();				// Converting Process ID from int to string or char array
		strcpy(buf,tmpstr.c_str());				// Now converting from string to const char * and copying to buf

		n = send(new_sock_fd[i],buf,strlen(buf), 0);	// Sending This machine's process ID to connected machine
		if (n < 0)
		 cout<<"Unable to write to the socket"<<endl;


		bzero(buf,256);
		recv(new_sock_fd[i],buf,255, 0);			// Reading the machine ID of connected machine
		
		ss1 << buf;
		string tmpstr1 = ss1.str();
	
		p1[i].p_id = atoi(tmpstr1.c_str());

		cout << "Connected to Machine ID '" << p1[i].p_id << "', ";

		n = send(new_sock_fd[i],"ID received",11, 0);
		if (n < 0) cout<<"Unable to write to the socket"<<endl;

		bzero(buf,256);
		recv(new_sock_fd[i],buf,255, 0);			// Reading the port number of connected machine
		
		ss2 << buf;
		string tmpstr2 = ss2.str();
	
		p1[i].p1 = atoi(tmpstr2.c_str());			// Saving the port number of connected machine

		cout << "with port number '" << p1[i].p1 << "'." << endl;

		n = send(new_sock_fd[i],"Port received",13, 0);
		if (n < 0) cout<<"Unable to write to the socket"<<endl;

		p1[i].sock_fd = new_sock_fd[i];				// Saving the Socket Descriptor of connected machine

		i++;							// counter for accepted connections
		counter++;							// counter for total connections
	}

	for(int j=0; j < i; j++)					// Storing the accepted process details into main datastructure of all processes
	{
		p[connections] = p1[j];
		connections++;
	}
	
}

int NonCausalityCheck(string tmpstr)
{
	int arr[10];					// Array for temporarily storing received vector clock
	int id;						// Integer to store ID of sender process
		
	stringstream ss1(tmpstr);

	for(int i=0; i < machines; i++)				// Getting the sender's vector clock from buf into a temporary vector clock array
	{
		ss1 >> arr[i];
	}
			
	ss1 >> id;

	cout << "\nReceived Sender's Index value: " << id << endl;


	for(int i=0; i < machines; i++)
	{
		if(arr[i] > v[i])			// Updating Vector Clock
		{
			v[i] = arr[i];
		}
	}
	return id;
}

void *receiveMessage(void *f1) 					// thread function for receiving multicast message from particular process
{
	char buf[256];
	int n;
	long sock_fd = (long)f1;					// Socket FD for communicating with specific process
	
	while(1)
	{
		bzero(buf,256);
		
		srand(time(0));
		sleep(rand()%6);		

		int rc = recv(sock_fd,buf,sizeof(buf), 0);	// Receive message from sender with their vector clock
		
		if(rc < 0)
		{
			cout<<"Unable to read to the socket"<<endl;		// Print respective message if there's an error while receving the message
		}
		else
		{
			stringstream ss;
			string tmpstr;
			ss.str("");
			ss << buf;
			
			tmpstr = ss.str();
		
			int flag = NonCausalityCheck(tmpstr);

			cout << "\nMulticast Message Received from <Machine (" << flag << ")>" << endl;
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

void *sendMessage(void *arg) 							// thread function for sending multicast message
{
	char buf[256];
	int n;
	while(1)
	{	
		int action;
		cout << "Press 1 to multicast: ";
		cin >> action;
		if(action==1)
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
		
						bzero(buf,256);			
						stringstream ss;
						string tmpstr;
	
						ss.str("");
						ss << v[0];
	
						tmpstr = ss.str();				// Converting Process ID from int to string or char array
						strcpy(buf,tmpstr.c_str());
	
						strcat(buf," ");
	
						for(int j=1; j < machines; j++)
						{
							ss.str("");
							ss << v[j];
	
							tmpstr = ss.str();			// Converting Process ID from int to string or char array
							strcat(buf,tmpstr.c_str());		// Now converting from string to const char *
						
							strcat(buf," ");
	
						}
						ss.str("");
						ss << pid;
	
						tmpstr = ss.str();				// Converting Process ID from int to string or char array
						strcat(buf,tmpstr.c_str());					
	
						n = send(p[i].sock_fd,buf,sizeof(buf), 0);
						if (n < 0) cout<<"Unable to write to the socket"<<endl;
					}
				}

				srand(time(0));
				sleep(rand()%5);
		
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

	int action, n;
	//creating ports and filecdescriptors
	long p1,p2, new_sock_fd[10];

	char buf[256];
	long f1;
	socklen_t client_length;
	
	struct sockaddr_in serv_addr, client_addr;

	pthread_t thread1, thread2, thread3;		//threads for handling client requests

	if (argc < 2) {
		fprintf(stderr,"please provide the port as well during execution\n");
		exit(1);
	}

	ifstream myfile("num_machines.txt");			// Taking number of processes from file

	string line;
	
	if(myfile.is_open())
	{
		getline(myfile, line);				// Reading file by line and storing the number of machines
		istringstream ss(line);
		ss >> machines;
		myfile.close();
	}
	else cout << "Unable to open file"; 

	
	f1 = socket(AF_INET, SOCK_STREAM, 0);
	
	if(f1 < 0) 
	{
		cout<<"Unable to open socket"<<endl;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	p2 = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(p2);
	
	int reuse = 1;
	if (setsockopt(f1, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)	//To reuse socket address in case of crashes and failures
		perror("setsockopt(SO_REUSEADDR) failed");

	#ifdef SO_REUSEPORT
	if (setsockopt(f1, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
		perror("setsockopt(SO_REUSEPORT) failed");
	#endif

	if(bind(f1, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		cout<<"Unable to bind"<<endl;
	}
	listen(f1,10);

	cout << "Enter the machine number(id) ";
	cin >> pid;
	p[0].p_id = pid;
	p[0].p1 = p2;
	p[0].sock_fd = f1;

	int rc = pthread_create(&thread3, NULL, acceptconn, (void*)f1);
	
	cout << "press 1 to connect to other machines else 2 and enter ";
	cin >> action;
	if(action == 1)
	{
		cout << "give no. of machines to connect: ";
		cin >> connections;
		for(int i=1; i <= connections; i++)
		{
			cout << "give port of Machine to connect: ";
			cin >> p[i].p1;

			p[i].sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	
			if(p[i].sock_fd < 0) 
				cout<<"Unable to open socket"<<endl;
			//getting host by name
			p[i].nserver = gethostbyname("localhost");
	
			bzero((char *) &p[i].servernode_addr, sizeof(p[i].servernode_addr));
			p[i].servernode_addr.sin_family = AF_INET;
			bcopy((char *)p[i].nserver->h_addr, (char *)&p[i].servernode_addr.sin_addr.s_addr, p[i].nserver->h_length);
			p[i].servernode_addr.sin_port = htons(p[i].p1);


			if(connect(p[i].sock_fd,(struct sockaddr *) &p[i].servernode_addr,sizeof(p[i].servernode_addr)) < 0) 
			cout<<"Unable to connect"<<endl;
			//creating buffer to store the received results
			bzero(buf,256);
			//reading data from the socket
			recv(p[i].sock_fd,buf,sizeof(buf), 0);			
			
			stringstream ss, ss1, ss2;
			ss << buf;
			string tmpstr = ss.str();

			p[i].p_id = atoi(tmpstr.c_str());

			cout << "Connected to machine ID: " << p[i].p_id << endl;
			//creating the  buffer to send the data
			bzero(buf,256);
			
			ss1 << p[0].p_id;
			string tmpstr1 = ss1.str();	
			//storing the data into buffer to send 
			strcpy(buf,tmpstr1.c_str());				
			// send the current process index to the connected machine
			n = send(p[i].sock_fd,buf,strlen(buf), 0);	
			if (n < 0) cout<<"Unable to write to the socket"<<endl;

			bzero(buf,256);
     			n = recv(p[i].sock_fd,buf,255, 0);

			bzero(buf,256);
			//reading port number
			ss2 << p[0].p1;
			string tmpstr2 = ss2.str();
			// storing the data into the buffer to send	
			strcpy(buf,tmpstr2.c_str());				
			// acknowledging the connected machien with the current machine port number
			n = send(p[i].sock_fd,buf,strlen(buf), 0);	
			if (n < 0)
			{
			//printing error message on to the console
			 cout<<"Unable to read to the socket"<<endl;
			}
			bzero(buf,256);
     			n = recv(p[i].sock_fd,buf,255, 0);

			counter++;
		}
		connections++;								
	}

	while(counter < machines)
	{
		continue;
	}
	cout << "Data of Connected machines:" << endl;
	cout << "\tID\tPort" << endl;



//Connections Completed 

	//sorting the machines in order

	for(int i=0; i < (machines-1); i++)						
	{
		for(int j =0; j < (machines -i-1); j++)
		{
			if(p[j].p_id > p[j+1].p_id)
			{
				swap(p[j], p[j+1]);
			}
		}
	}

	
	for(int i=0; i < machines; i++)
	{
		if(i == (pid - 1))
		{
			continue;
		}
		else
		{
			//thread to run the receive routine
			pthread_create(&thread2, NULL, receiveMessage, (void *)p[i].sock_fd);
		}
	}
	//thread to handle the sending message
	pthread_create(&thread1, NULL, sendMessage, NULL);

	while(1)
	{
		continue;
	}
}

