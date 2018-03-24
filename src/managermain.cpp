#include "../include/Controller.hpp"
#include <iostream>
#include <stdlib.h>
#include <string>
#include "opencv2/opencv.hpp"
#include "../include/CommUnit.hpp"
#include <pthread.h>
#include <thread>


using namespace std;
using namespace cv;
/*************************************
* This is a function for testing 
* purposes... it fills the 'outQueue'
* with unsigned char* strings such as
* '1', '2', ... '9999.'
*************************************/
static void testQueue(StartTransport * s){
		int i = 0;
		for ( i = 0; i < 10000; i++){
			MessageInfo *msgInfo_ = (MessageInfo *)malloc(sizeof(MessageInfo));
			char *tst = (char *)malloc(sizeof(char)*5);
			sprintf(tst, "%4d", i);
			tst[4] = '\0';
			msgInfo_ -> msg_ = (unsigned char*)tst;
			msgInfo_ -> size_ = 5;
			(s -> outQueue).push(msgInfo_);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

}

int main(int, char**){


	//local ports for this managers ServerUnits to use
	static char localPort1_[5] = {'5','0','2','6','\0'};
	//static char localPort2_[5] = {'5','0','2','7','\0'};
	//static char localPort3_[5] = {'9','9','9','4','\0'};

	//host ports for this managers ClientUnits to connect to
	static char workerPort1_[5] = {'5','0','2','8','\0'};
	//static char workerPort2_[5] = {'5','0','2','9','\0'};
	//static char workerPort3_[5] = {'5','1','1','2','\0'};

	//host ip address for this managers ClientUnits to connect to
	static char workerIP1_[10] ={'1','2','7','.','0','.','0','.','1','\0'};
	//static char workerIP2_[10] ={'1','2','7','.','0','.','0','.','1','\0'};
	//static char workerIP3_[10] ={'1','2','7','.','0','.','0','.','1','\0'};

	std::vector<ConnectionInfo*> vectorOfConnections;
	ConnectionInfo *temp1 = (ConnectionInfo *)malloc(sizeof(ConnectionInfo));
	if(temp1 == NULL){
		std::cout << "Malloc failed in Main" << std::endl;
		return EXIT_FAILURE;
	}
	temp1 -> hostIP_ = workerIP1_;
	temp1 -> hostPort_ = workerPort1_;
	temp1 -> localPort_ = localPort1_;
	
	vectorOfConnections.push_back(temp1);

	//string filename = "1.mp4";
	//int groupSize = 30;
	//queue<vector<Mat>>* clips = new queue<vector<Mat>>();
	
	//Initial call to build StartTransport class, holds inQueue and outQueue
	StartTransport * communicate = new StartTransport(vectorOfConnections);
	
	//thread for filling StartTransport::outQueue, for testing purposes
	std::thread t2 = std::thread (&testQueue, std::ref(communicate));
	
	//Controller *controller = new Controller(3,clips,5,std::ref(communicate));
	
	//calls function StartTransport::start which builds all CommUnits 
	std::thread t1 = std::thread (&StartTransport::start, std::ref(communicate));
	
	//std::thread t2 = std::thread (&Controller::start, std::ref(controller));
	t1.join();
	t2.join();

	//delete[] clips;
	delete[] communicate;
	//delete[] controller;
	free(temp1);
	
	//???
	//controller->receive(msgs);
}
