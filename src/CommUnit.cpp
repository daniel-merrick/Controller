
#include <ctime>
#include <iostream>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <ctime>
#include "asio.hpp"
#include "queue.hpp"
#include "CommUnit.hpp"

#define HEADER_LENGTH 7
//start transport funcs

StartTransport::StartTransport(std::vector<ConnectionInfo*> v):ios_(){
	this -> v = v;
}

void StartTransport::start(){
	int i = 0;
	std::vector<std::shared_ptr<std::thread>> threads;
	for ( i = 0; i < v.size(); i++){
		std::shared_ptr<std::thread> t(new std::thread(&StartTransport::establishCommunicationThread, this, std::ref(ios_), v[i]->hostPort_, v[i]->localPort_, v[i]->hostIP_));
		threads.push_back(t);
	}
	for (i = 0; i < threads.size(); i++){
		(threads[i]) -> join();
	}
}

void StartTransport::establishCommunicationThread(asio::io_service& ios_, char *workerPort_, char *localPort_, char *IP_){
	CommUnit cu(ios_, workerPort_, localPort_, IP_, std::ref(inQueue), std::ref(outQueue));
}


//comm unit funcs
CommUnit::CommUnit(asio::io_service& io_service_, char *hostport_, char *localport_, char *host_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){
	initializeCommUnit(io_service_, hostport_, localport_, host_, inQueue, outQueue);
}

void CommUnit::establishServer(short port_, asio::io_service& ios_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){
	std::cout << "creating server class" << std::endl;
	ServerUnit server(ios_, port_, inQueue, outQueue);
}

void CommUnit::establishClient(asio::io_service& ios_, char *host_, char *port_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){
	ClientUnit client(ios_, host_, port_, inQueue, outQueue);

}

void CommUnit::initializeCommUnit(asio::io_service& io_service_, char *hostport_, char *localport_, char *host_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue){

	std::cout << "establish server threa" << std::endl;	
	std::thread t1 = std::thread (&CommUnit::establishServer, atoi(localport_), std::ref(io_service_), std::ref(inQueue), std::ref(outQueue));
	std::cout << "establish client thread" << std::endl;
	std::thread t2 = std::thread (&CommUnit::establishClient, std::ref(io_service_), host_, hostport_, std::ref(inQueue), std::ref(outQueue));
		
	t1.join();
	t2.join();
}

//client unit funcs
ClientUnit::ClientUnit(asio::io_service& io_service, char *host, char *port, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue)
:socket_(io_service), resolver_(io_service), query_(host,port), ec(), inQueue(inQueue), outQueue(outQueue){
	endpoints_ = resolver_.resolve(query_);
	asio::connect(socket_, endpoints_, ec);
	while(ec){
		asio::connect(socket_, endpoints_, ec);
	}
	std::cout << "ClientUnit succesfully connected to a ServerUnit" << std::endl;
	for(;;){
		send();
	}
		
}

void ClientUnit::send(){
	MessageInfo *msgInfo_ = (MessageInfo *) (outQueue.pop());
	unsigned char *msg_ = msgInfo_ -> msg_;
	//build header
	unsigned char header_[HEADER_LENGTH];
	buildHeader(msg_, header_, msgInfo_);

	//build full packet ~ header + message	
	unsigned char send_this_[HEADER_LENGTH + msgInfo_ -> size_];
	buildPacketToSend(msg_, send_this_, header_, msgInfo_);

	//send to socket
	//std::cout << "Sending Message: " << send_this_ << std::endl;
	//std::vector<unsigned char> b(HEADER_LENGTH+msgInfo_ -> size_);
	asio::write(socket_, asio::buffer(send_this_, HEADER_LENGTH + msgInfo_ -> size_), ec);

	//asio::transfer_all(), ec);
	free(msgInfo_ -> msg_);
	free(msgInfo_);
}
	
//build packet to send to socket
void ClientUnit::buildPacketToSend(unsigned char *msg_, unsigned char *send_this_, unsigned char *header_, MessageInfo* msgInfo_){
	
	//build body	
	unsigned char body_[msgInfo_ -> size_];
	std::memcpy(body_, msg_, msgInfo_ -> size_ * sizeof(unsigned char));
	
	//concatenate body with header
	std::memcpy(send_this_, header_, HEADER_LENGTH);
	std::memcpy(send_this_ + HEADER_LENGTH, body_, msgInfo_ -> size_);
}

//build header for sending
void ClientUnit::buildHeader(unsigned char *message_, unsigned char *header_, MessageInfo * msgInfo_){
	sprintf((char *) header_, "%7d", static_cast<int>(msgInfo_ -> size_));
	std::cout << "header_: " << header_ << std::endl;
}
	
//server unit funcs
ServerUnit::ServerUnit(asio::io_service& io_service_, short port_, Queue<MessageInfo *> &inQueue, Queue<MessageInfo *> &outQueue):port_(port_), ec(), socket_(io_service_), acceptor_(io_service_, tcp::endpoint(tcp::v4(), port_)),
inQueue(inQueue), outQueue(outQueue){
		//if the client hangs up, the server cannot
		//reconnect ~ if we want the server to always
		//look for a connection put this function
		//call in a for loop
		std::cout << "accepting" << std::endl;
		accept(); 
}

//accept connection with client trying to merge on port
void ServerUnit::accept(){
	std::cout << "Making connection on port " << port_ << std::endl;
	acceptor_.accept(socket_);
	std::cout << "ServerUnit successfully connected to a ClientUnit" << std::endl;
	for(;;){
		//connection closed by connected client
		if(read() == EXIT_FAILURE){
			break;
		}
		
	}
	std::cout << "Connection Closed... disconnecting from port "<< port_ << std::endl;
	//close socket so we can search for new peer
	socket_.close();
}
	
//continuously read from socket_
int ServerUnit::read(){
	MessageInfo * msgInfo_ = (MessageInfo *)malloc(sizeof(MessageInfo));
	if(msgInfo_ == NULL){
		std::cout << "Malloc failed in ServerUnit::read ~ msgInfo_" << std::endl;
		return EXIT_FAILURE;
	}
	unsigned char *header_ = (unsigned char *)malloc(sizeof(unsigned char)*(HEADER_LENGTH + 1));
	if(header_ == NULL){
		std::cout << "Malloc failed in ServerUnit::read ~ header_" << std::endl;
		return EXIT_FAILURE;
	}
	getHeader(header_);
	//connection closed
	if(ec == asio::error::eof){//connection closed by peer
		std::cout << "ERROR: EOF REACHED: from header" << std::endl;
		return EXIT_FAILURE;
	}
	//insert message size
	msgInfo_ -> size_ = atoi((char *)header_);
	
	//get body of message aka main part
	unsigned char *body_ = (unsigned char*)malloc(sizeof(unsigned char)*(msgInfo_ -> size_));
	if(body_  == NULL){
		std::cout << "Malloc failed in ServerUnit::read ~ body_" << std::endl;
		return EXIT_FAILURE;
	}
	getBody(msgInfo_, body_);

	//connection closed
	if(ec == asio::error::eof){
		std::cout << "ERROR: EOF REACHED: for body" << std::endl;
		return EXIT_FAILURE;
	}
	
	//push recieved message to queue
	//std::cout << "Message Recieved: " << header_ << body_ << std::endl;
	msgInfo_ -> msg_ = body_;
	inQueue.push(msgInfo_);
	return EXIT_SUCCESS;
}
	
//retrieve header of packet from socket
void ServerUnit::getHeader(unsigned char *header_){
	//read header of length 7 bytes from socket
	int bytes_read_ = asio::read(socket_, asio::buffer(header_, HEADER_LENGTH), ec);
	header_[HEADER_LENGTH] = '\0';
	std::cout << "header_ " << header_ << "header_ atoi'd: " << atoi((char *)header_) << std::endl;
	//check if we read wrong amount of bytes from socket
	if(bytes_read_ != 7 && bytes_read_ != 0){
		std::cout << "Incorrect number of bytes read when reading header" << std::endl;
	}
}
	
//retreve body (message) of packet from socket
void ServerUnit::getBody(MessageInfo *msgInfo_, unsigned char *body_){
	
	//read the message from socket
	int bytes_read_ = asio::read(socket_, asio::buffer(body_, msgInfo_ -> size_));
	//body_[msgInfo_ -> size_] = '\0';
	//check if we read wrong about of bytes from the socket
	if(bytes_read_ != msgInfo_ -> size_ && bytes_read_ != 0){
		std::cout << "ERROR: incorrect number of bytes read while reading body" <<std::endl;
	}
}

