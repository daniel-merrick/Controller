
#include <ctime>
#include <iostream>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <ctime>
#include "asio.hpp"
#include "../include/queue.hpp"
#include "../include/CommUnit.hpp"

int HEADER_LENGTH = 7;
//start transport funcs

StartTransport::StartTransport(std::vector<ConnectionInfo*> v){
	this -> v = v;
}

void StartTransport::start(){
	int i = 0;
	std::vector<std::shared_ptr<std::thread>> threads;
	for ( i = 0; i < v.size(); i++){
		/********************************************
		* Create a thread pool equal to the 
		* size of the vector passed to constructor
		* Each thread creates an instance of CommUnit
		********************************************/
		std::shared_ptr<std::thread> t(new std::thread(&StartTransport::establishCommunicationThread, this, std::ref(ios_), v[i]->hostPort_, v[i]->localPort_, v[i]->hostIP_));
		threads.push_back(t);
	}
	for (i = 0; i < threads.size(); i++){
		(threads[i]) -> join();
	}
}

void StartTransport::establishCommunicationThread(asio::io_service& ios_, char *workerPort_, char *localPort_, char *IP_){
	/*********************
	* Construct a CommUnit
	*********************/
	CommUnit cu(ios_, workerPort_, localPort_, IP_, std::ref(inQueue), std::ref(outQueue));
}

CommUnit::CommUnit(asio::io_service& io_service_, char *hostport_, char *localport_, char *host_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){
	/**************************
	* Initialize a CommUnit..
	* A CommUnit contains a 
	* ServerUnit and ClientUnit
	**************************/
	initializeCommUnit(io_service_, hostport_, localport_, host_, inQueue, outQueue);
}

void CommUnit::initializeCommUnit(asio::io_service& io_service_, char *hostport_, char *localport_, char *host_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){
	/**********************************
	* Thread to initialize a ServerUnit
	**********************************/
	std::thread t1 = std::thread (&CommUnit::establishServer, atoi(localport_), std::ref(io_service_), std::ref(inQueue), std::ref(outQueue));
	/**********************************
	* Thread to initialize a ClientUnit
	**********************************/	
	std::thread t2 = std::thread (&CommUnit::establishClient, std::ref(io_service_), host_, hostport_, std::ref(inQueue), std::ref(outQueue));
		
	t1.join();
	t2.join();
}

void CommUnit::establishServer(short port_, asio::io_service& ios, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){
	/**************************************
	* Instantiate an instance of ServerUnit
	**************************************/	
	ServerUnit server(ios, port_, inQueue, outQueue);
}

void CommUnit::establishClient(asio::io_service& ios, char *host_, char *port_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){
	/**************************************
	* Instantiate an instance of ClientUnit
	**************************************/	
	ClientUnit client(ios, host_, port_, inQueue, outQueue);

}

//client unit funcs
ClientUnit::ClientUnit(asio::io_service& io_service, char *host, char *port, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue)
:io_service_(io_service),socket_(io_service), resolver_(io_service), query_(host,port), ec(), inQueue(inQueue), outQueue(outQueue){
	endpoints_ = resolver_.resolve(query_);
	/***************************
	* Wait until this ClientUnit
	* connects with a ServerUnit
	***************************/
	asio::connect(socket_, endpoints_, ec);
	while(ec){
		asio::connect(socket_, endpoints_, ec);
	}
	std::cout << "This CientUnit successfully connected to a ServerUnit" << std::endl;
	/**********************************
	* Continuously send data from
	* outQueue to the shared Socket_
	* between ServerUnit and ClientUnit
	**********************************/
	for(;;){
		send();
	}
}

void ClientUnit::send(){
	//pop next message struct from outQueue to send to worker
	MessageInfo *msgInfo_ = (MessageInfo *) (outQueue.pop());
	unsigned char *msg_ = msgInfo_ -> msg_;
	
	//build header
	unsigned char header_[HEADER_LENGTH];
	buildHeader(msg_, header_, msgInfo_);

	//build full packet ~ header + message	
	unsigned char send_this_[HEADER_LENGTH + msgInfo_ -> size_*sizeof(unsigned char)];
	buildPacketToSend(msg_, send_this_, header_, msgInfo_);

	//send to socket
	asio::write(socket_, asio::buffer(send_this_, HEADER_LENGTH + msgInfo_ -> size_), ec);

	free(msgInfo_ -> msg_);
	free(msgInfo_);
}
	
//build packet to send to socket
void ClientUnit::buildPacketToSend(unsigned char *msg_, unsigned char *send_this_, unsigned char *header_, MessageInfo* msgInfo_){
	
	//build body	
	unsigned char *body_ = (unsigned char*)malloc(sizeof(unsigned char) * (msgInfo_ -> size_));
	if (body_ == NULL){
		std::cout << "Malloc failed in ClientUnit::buildPacketToSend ~ body_" << std::endl;
		return;
	}
	//copy msg into body
	std::memcpy(body_, msg_, (msgInfo_ -> size_) * sizeof(unsigned char));
	//copy header into send_this_
	std::memcpy(send_this_, header_, (HEADER_LENGTH) * sizeof(unsigned char));
	//concatenate body_ with header_ in send_this_
	std::memcpy((send_this_) + (HEADER_LENGTH), body_, msgInfo_ -> size_ * sizeof(unsigned char));
	free(body_);
}

//build header for sending
void ClientUnit::buildHeader(unsigned char *message_, unsigned char *header_, MessageInfo * msgInfo_){
	sprintf((char *) header_, "%7d", static_cast<int>(msgInfo_ -> size_));
}
	
//server unit funcs
ServerUnit::ServerUnit(asio::io_service& io_service, short port_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue):port_(port_), ec(), socket_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port_)),
inQueue(inQueue), outQueue(outQueue){
		//if the client hangs up, the server cannot
		//reconnect ~ if we want the server to always
		//look for a connection put this function
		//call in a for loop
		accept(); 
}

//accept connection with client trying to merge on port
void ServerUnit::accept(){
	std::cout << "This ServerUnit waiting for a ClientUnit to connect on port: " << port_ << std::endl;
	acceptor_.accept(socket_);
	std::cout << "This ServerUnit is connected to a ClientUnit" << std::endl;
	for(;;){
		//connection closed by connected client
		if(read() == EXIT_FAILURE){
			break;
		}
		
	}
	std::cout << "A ClientUnit disconnected from this ServerUnit, connection disconnected on port: "<< port_ << std::endl;
	//close socket 
	socket_.close();
}
	
//continuously read from socket_
int ServerUnit::read(){
	//allocate memory for a new struct to hold msg info and push to inQueue
	MessageInfo * msgInfo_ = (MessageInfo *)malloc(sizeof(MessageInfo));
	if(msgInfo_ == NULL){
		std::cout << "Malloc failed in ServerUnit::read ~ msgInfo_" << std::endl;
		return EXIT_FAILURE;
	}
	//allocate memory for a header_
	unsigned char *header_ = (unsigned char *)malloc(sizeof(unsigned char)*(HEADER_LENGTH + 1));
	if(header_ == NULL){
		std::cout << "Malloc failed in ServerUnit::read ~ header_" << std::endl;
		return EXIT_FAILURE;
	}
	//read the header from socket_
	getHeader(header_);
	//check if the connection was closed by connected ClientUnit
	if(ec == asio::error::eof){//connection closed by peer
		std::cout << "ERROR: EOF REACHED: from header" << std::endl;
		return EXIT_FAILURE;
	}
	//insert message size into message struct
	msgInfo_ -> size_ = atoi((char *)header_);
	
	//allocate memory for body of message
	unsigned char *body_ = (unsigned char*)malloc(sizeof(unsigned char)*(msgInfo_ -> size_));
	if(body_  == NULL){
		std::cout << "Malloc failed in ServerUnit::read ~ body_" << std::endl;
		return EXIT_FAILURE;
	}
	//read body of message from socket
	getBody(msgInfo_, body_);

	//check if connection was closed by connected ClientUnit
	if(ec == asio::error::eof){
		std::cout << "ERROR: EOF REACHED: for body" << std::endl;
		return EXIT_FAILURE;
	}
	
	//std::cout << "Message Recieved: " << header_ << body_ << std::endl;
	//insert message into msg struct
	msgInfo_ -> msg_ = body_;
	//push to queue
	inQueue.push(msgInfo_);
	return EXIT_SUCCESS;
}
	
//retrieve header of packet from socket
void ServerUnit::getHeader(unsigned char *header_){
	
	//read header from socket
	long bytes_read_ = asio::read(socket_, asio::buffer(header_, HEADER_LENGTH), ec);
	//set delimeter
	header_[HEADER_LENGTH] = '\0';
	
	//check if we read wrong amount of bytes from socket
	if(bytes_read_ != 7 && bytes_read_ != 0){
		std::cout << "Incorrect number of bytes read when reading header" << std::endl;
	}
}
	
//retreve body (message) of packet from socket
void ServerUnit::getBody(MessageInfo *msgInfo_, unsigned char *body_){
	
	//read the message from socket
	long bytes_read_ = asio::read(socket_, asio::buffer(body_, msgInfo_ -> size_));
	
	//check if we read wrong about of bytes from the socket
	if(bytes_read_ != msgInfo_ -> size_ && bytes_read_ != 0){
		std::cout << "ERROR: incorrect number of bytes read while reading body" <<std::endl;
	}
}

