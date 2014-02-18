#include <iostream>
#include <cstdio>
#include "udpAgent.h"
#include "packets.h"
using namespace std;

udpAgent* agent;

void readfile (char* filename){
	
	//send RRQ
	RRQ req(filename, "octet");
	agent->send(req.toString(), req.size);
	
	int datalen;
	
	do {
		
		//wait for DATA
		char* chunk = malloc(1024);
		agent->recv(chunk, 1024);
		
		//read data
		DATA recieved(chunk);
		datalen = recieved.datalen;
		
		printf("%s", recieved.data);
		
		//Acknowledge
		ACK gotit(recieved.blocknum);
		agent->send(gotit.toString(), gotit.size);
	} while (datalen==512);
	
}


int main(){
	agent = new udpAgent("127.0.0.1", 5555);
	readfile("hello");
	return 0;
}
