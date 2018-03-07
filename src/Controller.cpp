#include "../include/Controller.hpp"
#include "../include/matSerial.hpp"
#include <iostream>
#include <stdlib.h>
#include <string>
#include <queue>
#include "../include/queue.hpp"
#include <vector>
#include <pthread.h>
#include <bitset>
#include "../include/CommUnit.hpp"
using namespace cv;
using namespace std;
 
//#include "opencv2/core/mat.hpp"
//#include "opencv2/core.hpp"'
#include "opencv2/opencv.hpp"

Controller::Controller(int worker, queue<vector<Mat> > *clips, int groupSize, StartTransport * cu) // constructor
{
	this->worker = worker;
	this->groupSize = groupSize;
	this->clips = clips;
	this->cu = cu;
	messageID = 0;
} 

void Controller::send_group(){
	while(1) { 
		lock.lock();
		// LOCK START ************************
		if(clips->empty()) { // exits function when empty
			// LOCK END**********************
			lock.unlock();
			break;
		} else {

			vector<Mat> frames;
			frames = clips->front();
			clips->pop();
//			printf("#%d REMOVED : \n", index/30);
			// LOCK END**********************
			lock.unlock();
			for(int i = 0; i < (groupSize); i++) { // sending 30 frames one by one
				vector<unsigned char> buf = matWrite(frames[i]);

				//get size of buffer
				long actual_size = static_cast<long>(buf.size());

				/***************************************************************
				* The following lines allocates memory for a message to 
				* store in 'Messageinfo' which will be pushed to the 'outQueue.'
				* 'MessageInfo' stores information such as the message in an
				* unsigned char, size of message in a long, and an ID number
				* as an int.
				***************************************************************/

				//allocate memory for temp which holds the message stored in vector 'buf
				unsigned char *temp = (unsigned char *)malloc(sizeof(unsigned char) * (actual_size));;
				if(temp == NULL){
					std::cout << "Malloc failed in Controller::send_group() ~ temp" << std::endl;
					return;
				}
				//copy stream return from matframe[i] to unsigned char *
				memcpy(&temp[0], &buf[0], actual_size);

				//MessageInfo struct stores information about the message such as size,msg, and ID
				MessageInfo * push_this = (MessageInfo*)malloc(sizeof(MessageInfo));
				if(push_this == NULL){
					std::cout << "Malloc failed in Controller::send_group ~ push_this" << std::endl;
					return;
				}

				//push to the queue
				push_this -> msg_ = temp;
				push_this -> size_ = actual_size;
				cu->outQueue.push(push_this); // send to comUnit
				
				// ADDED FOR VERIFICATION // REMOVE LATER
			
				std::vector<unsigned char> tst(push_this -> size_ * sizeof(unsigned char));
				std::memcpy(&tst[0], &((push_this -> msg_)[0]), push_this -> size_ * sizeof(unsigned char));

				cv::Mat *a = matRead(tst);
				std::vector<int> params;
				params.push_back(cv::IMWRITE_JPEG_QUALITY);
				params.push_back(90);
				char buff [strlen("serverOutput/final.jpg") + 1];
				sprintf(buff,"serverOutput/final%d.jpg", index);
				index++;
				cv::imwrite(buff, *a, params);
			}
			
			
		}		
		
	}
}


void Controller::read_video(string filename){

    VideoCapture cap(filename); // open the default camera

    if(!cap.isOpened()){  // check if we succeeded
         cout << "It's not opening the file" << endl;
	 return;
    }
   vector<Mat> group;

   Mat frame;
   //namedWindow("w",1);
   for(;;)

    {
	int groupNum = 0;
	//for(int i = 0; i < this->groupSize; i++){
        for(int i = 0; i < 30; i++){
	  int frameNum = 1;
	  int frameId = cap.get(frameNum++);
          cap >> frame; // get a new frame from camera
	  group.push_back(frame);
        }

    	lock.lock();
	//read video
    	clips->push(group);

	lock.unlock();
	groupNum++;
	
	if(waitKey(30) >= 0)
		break;
	}
	thread0Finish = 1; // added to notify when read_video thread ends
}

void Controller::print_queue(queue<vector<Mat> > *clips){
	vector<Mat> a;
	int num = 0;
	for(int i = 0; i < clips->size(); i++) {
		a = clips->front();
		clips->pop();
		cout << i << endl;
	}
}

void Controller::receive(queue<string> msgs){
	if(msgs.size() == 0){

		cout << "There is no such message yet";
		return;
	}

	while(msgs.size()!=0){

		messageID++;
		bitset<32> num(messageID);
		cout << num  << " | " <<msgs.front() << endl;
		msgs.pop();
	}
}

void Controller::start(){
		index = 0;
		thread0Finish = 0;
		pthread_t sendThread;
		pthread_t readThread;
	
		int a = pthread_create(&sendThread, NULL, Controller::send_group_thread_callback, this);
		int b = pthread_create(&readThread, NULL, Controller::read_video_thread_callback, this);
		if (a == -1 || b == -1) {printf("Issue Creating Thread %d %d\n",a,b); exit(1);}
		
		while(!thread0Finish) {
			pthread_create(&sendThread, NULL, Controller::send_group_thread_callback, this);
		} 
		std::cout << "readThread finished" << std::endl;
		pthread_create(&sendThread, NULL, Controller::send_group_thread_callback, this);
		pthread_join(readThread, NULL);
		std::cout << "readThread finished" << std::endl; 
		pthread_join(sendThread, NULL);
		std::cout << "sendThread finished" << std::endl;
		//pthread_exit(NULL);
}

void * Controller::send_group_thread_callback(void *controllerPtr) {
	Controller * controller = (Controller*) controllerPtr;
	controller->send_group();
	return controllerPtr;
}

void * Controller::read_video_thread_callback(void * controllerPtr) {
	Controller * controller = (Controller*) controllerPtr;
	controller->read_video("1.mp4"); //uncomment this later!
	//controller->push_test(); // place holder for now
	return controllerPtr;
}

void Controller::push_test() {
	cv::Mat pic = cv::imread("/Users/ruhana/CAM2/Prototype-Controller/Version 2.jpg", CV_LOAD_IMAGE_COLOR);
	for(int i = 0; i < 10; i++) {
		std::vector<cv::Mat> frames;
		// LOCK START********************************
		lock.lock();
		for(int i = 0 ; i < groupSize; i++) {
			frames.push_back(pic.clone());
		}
		clips->push(frames); // add array to queue
		//pthread_mutex_unlock(&lock); // LOCK END*************************
		lock.unlock();
		
	} 
	thread0Finish = 1;
}
