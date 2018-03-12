#include <iostream>
#include <stdlib.h>
#include <string>
#include "opencv2/opencv.hpp"
#include "../include/CommUnit.hpp"
#include "../include/Controller.hpp"
#include "../include/matSerial.hpp"
#include <pthread.h>
#include <thread>

using namespace std;
using namespace cv;

#define NUMBER_OF_FILES_TO_TEST_WRITE 2
/******************************************
* This is used to print (in this case) just
* one item from the inQueue at a time. This function
* is called inside a new thread from main 
* in order to run asynchronously with the 
* ServerUnit and CommUnit
*******************************************/
void printResults(StartTransport * s){
	for(;;){
		if(!(s->inQueue).empty()){
			MessageInfo *temp = (s->inQueue).pop();
			std::cout << "Message Received: " << (temp -> msg_) << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			free(temp -> msg_);
			free(temp);
		}
	}

}
int main(int, char**){
	//local port for this workers ServerUnit to host on
        static char localPort_[5] = {'5','0','2','8','\0'};

        //host port for this workers ClientUnit to connect to
        static char hostPort_[5] = {'5','0','2','6','\0'};

        //host ip for this workers ClientUnit to connect to
        static char hostIP_[10] ={'1','2','7','.','0','.','0','.','1','\0'};

	//fill a vector with structs of 'ConnectionInfo' which
	//is given as a parameter to StartTransport() function
	//The length of the vector(number of structs pushed)
	//determines how many CommUnits are created.. One CommUnit 
	//is made up of one ServerUnit and one ClientUnit
        std::vector<ConnectionInfo*> v;
        ConnectionInfo *temp = (ConnectionInfo *)malloc(sizeof(ConnectionInfo));
        temp -> hostIP_ = hostIP_;
        temp -> hostPort_ = hostPort_;
        temp -> localPort_ = localPort_;
        v.push_back(temp);
	
	/*******************************************************
	* If you want to run a function asynchronously 
	* with the CommUnit, then you MUST call your
	* function or functions inside a new thread, similar to
	* how I call printResults.
	********************************************************/
	
	//NOTE:
		//StartTransport holds the 'inQueue' and 'outQueue', so if you want access to them, you must pass
		//the object to your function parameter when calling your thread, similar to how call 'thread t2'
			//'inQueue' is the data that this worker is receiving from the manager
			//'outQueue' is queue that YOU must push to and want to be sent to the manager
	StartTransport * s = new StartTransport(v);      
	std::thread t1 = std::thread(&StartTransport::start, std::ref(s));
	std::thread t2 = std::thread(&printResults, std::ref(s));
	
	t1.join();
	t2.join();

	delete[] s;
	free(temp);
}
