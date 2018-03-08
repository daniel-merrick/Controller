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


/******************************************
* This is used to print (in this case) just
* one image frame to a file. This function
* is called inside a new thread from main 
* in order to run asynchronously with the 
* ServerUnit and CommUnit
*******************************************/
void printResults(StartTransport * s){
// ADDED FOR VERIFICATION // REMOVE LATER	
	int index = 0;
	while(index != 1){	
	MessageInfo *msgInfo_ = s -> inQueue.pop();
	
	std::vector<unsigned char> buf(msgInfo_ -> size_ * sizeof(unsigned char));
	std::cout << "printing results, msgInfo_ size: " << msgInfo_ -> size_ << std::endl;	
	std::memcpy(&buf[0], &((msgInfo_ -> msg_)[0]), msgInfo_ -> size_ * sizeof(unsigned char));
	cv::Mat *a = matRead(buf);
	std::vector<int> params;
	params.push_back(cv::IMWRITE_JPEG_QUALITY);
	params.push_back(90);
	//std::cout << "Writting to file" << std::endl;
	//cv::imshow("window", a);
	char buff[strlen("clientOutput/o.jpg") + 1];
	sprintf(buff,"clientOutput/o%d.jpg", index++);
	cv::imwrite(buff, *a, params);
	std::cout << "file written" << std::endl;
	}
}
int main(int, char**){
	//local ports for each ServerUnit to use
        static char local1_[5] = {'5','0','2','5','\0'};

        //host ports for each ClientUnit to connect to
        static char worker1_[5] = {'5','0','2','6','\0'};

        //host ip address
        static char workerIP1_[10] ={'1','2','7','.','0','.','0','.','1','\0'};

	//fill a vector with structs of 'ConnectionInfo' which
	//is given as a parameter to StartTransport() function
	//The length of the vector(number of structs pushed)
	//determines how many CommUnits are created.. One CommUnit 
	//is made up of one ServerUnit and one ClientUnit
        std::vector<ConnectionInfo*> v;
        ConnectionInfo *temp = (ConnectionInfo *)malloc(sizeof(ConnectionInfo));
        temp -> hostIP_ = workerIP1_;
        temp -> hostPort_ = worker1_;
        temp -> localPort_ = local1_;
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
	std::thread t1 = std::thread(&StartTransport::start, s);
	std::thread t2 = std::thread(printResults, s);
	
	t2.join();
	t1.join();

	delete[] s;
}
