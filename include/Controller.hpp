#include "opencv2/opencv.hpp"
#include <queue>
#include <iostream>
#include <stdlib.h>
#include <string>
#include "CommUnit.hpp"


using namespace std;
using namespace cv;
class Controller{
	private:
		StartTransport * cu; // communication unit
		int worker; // number of workers
		int groupSize; //number of frames 
		queue<vector<Mat> > *clips;
		//pthread_mutex_t lock;
		Mutex lock;
		string filename;
		int index;		

		unsigned int messageID;
		
	public:
		Controller(int worker, queue<vector<Mat> > *clips, int groupSize, StartTransport * cu);
		void send_group();
		void read_video(string filename);
		void push_test();
		void print_queue(queue<vector<Mat> > *clips);
		void receive(queue<string> msgs);
		void start();

		int thread0Finish;
		static void * send_group_thread_callback(void *controllerPtr);
		static void * read_video_thread_callback(void * controllerPtr);
//		~Controller();

};





