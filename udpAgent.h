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

#include <cstring>

using namespace std;

class udpAgent {

private:
	int soc;	//socket file descriptor
	struct sockaddr_in serv_addr;
	
public:

	bool closed;

	udpAgent (string serverIP, int port){
		
		closed = true;
		
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
		size_t n = sendto(soc, msg, len, 0, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
		if (n==-1) throw string("Failed to send message");
		return n;
	}
	
	size_t recv(char* buf, int len){
		socklen_t sl=sizeof(serv_addr);
		size_t n = recvfrom(soc, buf, len, 0, (struct sockaddr*) &serv_addr, &sl);
		cout<<">>"<<ntohs(serv_addr.sin_port)<<endl;
		if (n==0) closed = true;
		if (n==-1) throw string("Failed to recieve message");
		return n; 
	}

};

#endif
