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
#include "noncausal.h"
using namespace std;

int NonCausalityCheck(string tmpstr,int vclock[10], int total_p)
{
	int tmpArray[10];					// Array for temporarily storing received vector clock
	int index;						// Integer to store ID of sender process
		
	stringstream ss1(tmpstr);

	for(int i=0; i < total_p; i++)				// Getting the sender's vector clock from buffer into a temporary vector clock array
	{
		ss1 >> tmpArray[i];
	}
			
	ss1 >> index;

	cout << "\nReceived Sender's Index value: " << index << endl;


	for(int i=0; i < total_p; i++)
	{
		if(tmpArray[i] > vclock[i])			// Updating Vector Clock
		{
			vclock[i] = tmpArray[i];
		}
	}
	return index;
}
