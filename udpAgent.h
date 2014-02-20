#ifndef H_UDPAGENT
#define H_UDPAGENT

#include <iostream>
#include <string>
#include <exception>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include <cstring>
#include <cstdlib>

using namespace std;

class udpAgent {

private:
	int soc;	//socket file descriptor
	struct sockaddr_in serv_addr;
	char* prevPacket; int prevLength;
	int lport;
	
public:

	bool closed;

	udpAgent (string serverIP, int port){
		
		closed = true;
		lport = -1;
		prevPacket=malloc(1024);
		prevPacket[0]=0;
		prevLength=0;
		
		//set up socket
		if ((soc=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1){
			throw string("Can't open socket");
		}
		
		//set up serv_addr
		memset ((char*) &serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons((short)port);
		
		if (inet_aton(serverIP.c_str(), &serv_addr.sin_addr)==0) {
			throw string("ERROR: inet_aton() failed");
		}
		
		//connection opened successfully
		closed = false;
		
	}
	
	size_t send(char* msg, int len){
	
		memcpy(prevPacket, msg, len);
		prevLength = len;
	
		size_t n = sendto(soc, msg, len, 0, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
		//if (n==-1) throw string("Failed to send message");
		return n;
	}
	
	size_t recv(char* buf, int len){
		socklen_t sl=sizeof(serv_addr);
		errno = 0;
		memset(buf, 0, len);
		size_t n = recvfrom(soc, buf, len, 0, (struct sockaddr*) &serv_addr, &sl);
		
		//if (n==0) closed = true;
		//if (n==-1) throw string("Failed to recieve message");
		return n; 
	}
	
	size_t sendprev(){
		return send(prevPacket, prevLength);
	}
	
	void lockSenderPort(){
		lport = ntohs(serv_addr.sin_port);
	}
	
	bool isSenderPortValid(){
		if (!isSenderPortLocked()) return true;
		return lport == ntohs(serv_addr.sin_port);
	}
	
	bool isSenderPortLocked(){
		return lport!=-1;
	}
	
	void restoreSenderPort(){
		if (!isSenderPortLocked()) return;
		serv_addr.sin_port = htons(lport);
	}

};

#endif
