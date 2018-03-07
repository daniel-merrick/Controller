#include "../include/Controller.hpp"
#include <iostream>
#include <stdlib.h>
#include <string>
#include "opencv2/opencv.hpp"
#include "../include/CommUnit.hpp"
#include <pthread.h>
#include <thread>
//This main fucntion is for testing read_viode() function


using namespace std;
using namespace cv;

static void testQueue(StartTransport * s){
	int i = 0;
//	for ( i = 0; i < 10; i++){
//		char *tst = (char *)malloc(sizeof(char)*1);
//		sprintf(tst, "%d", i);
//		(s -> outQueue).push(tst);
//	}

}
int main(int, char**){


	//local ports for each ServerUnit to use
        static char local1_[5] = {'5','0','2','6','\0'};
        //static char local2_[5] = {'9','9','9','3','\0'};
        //static char local3_[5] = {'9','9','9','4','\0'};

        //host ports for each ClientUnit to connect to
        static char worker1_[5] = {'5','0','2','5','\0'};
        //static char worker2_[5] = {'4','1','1','2','\0'};
        //static char worker3_[5] = {'5','1','1','2','\0'};

        //local ip address

        static char workerIP1_[10] ={'1','2','7','.','0','.','0','.','1','\0'};
        //static char workerIP2_[10] ={'1','2','7','.','0','.','0','.','1','\0'};
        //static char workerIP3_[10] ={'1','2','7','.','0','.','0','.','1','\0'};

        std::vector<ConnectionInfo*> vectorOfConnections;
        ConnectionInfo *temp = (ConnectionInfo *)malloc(sizeof(ConnectionInfo));
	if(temp == NULL){
		std::cout << "Malloc failed in Main" << std::endl;
		return EXIT_FAILURE;
	 }
        temp -> hostIP_ = workerIP1_;
        temp -> hostPort_ = worker1_;
        temp -> localPort_ = local1_;
	
       	vectorOfConnections.push_back(temp);

  	string filename = "1.mp4";
  	int groupSize = 30;
  	queue<vector<Mat>>* clips = new queue<vector<Mat>>();
  	StartTransport * communicate = new StartTransport(vectorOfConnections);
	Controller *controller = new Controller(3,clips,5,communicate);
	std::thread t1 = std::thread (&StartTransport::start, communicate);
	std::thread t2 = std::thread (&Controller::start, controller);
	t1.join();
	t2.join();
		
	delete[] clips;
	delete[] communicate;
	delete[] controller;
	free(temp);
	//controller->start();
	//controller->receive(msgs);
}
