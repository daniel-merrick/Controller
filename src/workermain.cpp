#include <iostream>
#include <stdlib.h>
#include <string>
#include "opencv2/opencv.hpp"
#include "CommUnit.hpp"
#include "Controller.hpp"
#include "matSerial.hpp"
#include <pthread.h>
#include <thread>
#include "processor.hpp"

using namespace std;
using namespace cv;

#define NUMBER_OF_FILES_TO_TEST_WRITE 2
#define MAX_CASH 10
/******************************************
* This is used to print (in this case) just
* one item from the inQueue at a time. This function
* is called inside a new thread from main
* in order to run asynchronously with the
* ServerUnit and CommUnit
*******************************************/
Mutex l;
vector<Mat>* buffer;
int DETECTION = 1;
int STANDALONE = 0;

void printResults(StartTransport * s){;

	for(int i = 0;;i++){
		if(!(s->inQueue).empty()){
			if (buffer -> size() > MAX_CASH) continue;

			MessageInfo *temp = (s->inQueue).pop();
//			std::cout << "Message Received: " << (temp -> size_) << std::endl;

			std::vector<unsigned char> tst(temp -> size_ * sizeof(unsigned char));
                        std::memcpy(&tst[0], temp -> msg_, temp -> size_ * sizeof(unsigned char));
			Mat *frame = matRead(tst);
			//l.lock();
			buffer->push_back(*frame);
//			trackBuffer -> push_back(*frame);
			//l.unlock();
			delete frame;
			free(temp -> msg_);
			free(temp);
		}
	}
}

int read_t(vector<Mat>* buffer) {
    VideoCapture cap("dataset/input.mp4");
    cap.set(CV_CAP_PROP_POS_FRAMES, 0);
    if(!cap.isOpened()){  // check if we succeeded
         cout << "It's not opening the file" << endl;
         return 0;
    }

    int numFrames = cap.get(CV_CAP_PROP_FRAME_COUNT);
    setFrameCount(numFrames);
    for(int i = 0;; i++)
    {
	if (buffer -> size() > 100)
        {
            usleep(100000);
	    continue;
        }
	Mat frame;
        cap >> frame;
	buffer -> push_back(frame);
	usleep(1000);
    }
}

int main(int, char**){
	if (STANDALONE) {
	    buffer = new vector<Mat>();

	    if (DETECTION == 1)
            {
		std::thread read_thread(read_t, std::ref(buffer));
                std::thread detect_thread(detect_HOG, std::ref(buffer));
                detect_thread.join();
		read_thread.join();
		return 0;
            }
            else
            {
		std::thread read_thread(read_t, std::ref(buffer));
                std::thread detect_thread(detect, std::ref(buffer));
                std::thread track_thread(track);

                detect_thread.join();
                track_thread.join();
		read_thread.join();
		return 0;
            }
	}

	buffer = new vector<Mat>();

	//local port for this workers ServerUnit to host on
        static char localPort_[5] = {'5','0','5','6','\0'};

        //host port for this workers ClientUnit to connect to
        static char hostPort_[5] = {'5','0','5','5','\0'};

        //host ip for this workers ClientUnit to connect to
        static char hostIP_[13] ={'1','9','2','.','1','6','8','.','1','.','1','3','\0'};

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

	if (DETECTION == 1)
	{
	    std::thread detect_thread(detect_HOG, std::ref(buffer));

            detect_thread.join();

            t1.join();
	    t2.join();
	}
	else
	{
	    std::thread detect_thread(detect, std::ref(buffer));
	    std::thread track_thread(track);

            detect_thread.join();
            track_thread.join();

            t1.join();
            t2.join();
	}

	delete[] s;
	free(temp);
}
