#include <iostream>
#include <cstdio>
#include <csignal>
#include "udpAgent.h"
#include "packets.h"
#include <errno.h>
using namespace std;

#define MAX_ATTEMPTS 10
#define TIMEOUT 2

udpAgent* agent;

extern int errno;

void alarmHandler (int sig){
	//do nothing, system call interrupted
	//cout<<"debug"<<endl;
}

void readfile (char* filename){
	
	FILE* f = fopen(filename, "wb");
	if (f == NULL){
		printf("ERROR: Can't open file (client)\n");
		return;
	}
	
	//open file and send RRQ
	RRQ req(filename, "octet");
	agent->send(req.toString(), req.size);
	alarm(TIMEOUT);
	
	printf("RRQ sent\n");
	
	int datalen;
	int blocknum=0;
	int p;
	char* chunk = malloc(1024);
	int attempts=0;
	
	do {
		
		//this while loops terminates when a valid chunk is recieved
		
		bool gotChunk= false;
		while (!gotChunk){
		
			//wait for DATA
			int n = agent->recv(chunk, 1024);
			if (n == -1){
				//check for timer interrupt
				if (errno==EINTR){
					//timeout
					
					if (attempts == MAX_ATTEMPTS){
						//throw error and exit
						printf("FAILED: max attempts reached\n");
						return;
					}
					
					agent->sendprev();
					alarm(TIMEOUT);
					printf("TIMEOUT: retrying...\n");
					attempts++;
					continue;
				}
			}
			
			p = packetType(chunk);
			
			if (p==pERROR){
				ERROR e(chunk);
				printf("ERROR %d: %s\n", e.errorcode, e.errorcode==0 ? e.errorstr : errormsgs[e.errorcode]);
				alarm(0);
				return;
			}
			else if (p==pDATA){
				DATA d(chunk);
				if (d.blocknum == blocknum+1){
					//got required block
					gotChunk = true;
					printf("Recieved block %d\n", d.blocknum);
					blocknum++;
				}
			} else {
				//WTF
			}
		
		}
		
		alarm(0);	//chunk recieved, alarm can now be turned off
		
		//read data
		DATA recieved(chunk);
		datalen = recieved.datalen;
		
		//write to file
		fwrite(recieved.data, 1, datalen, f);
		
		//Acknowledge
		ACK gotit(recieved.blocknum);
		agent->send(gotit.toString(), gotit.size);
		alarm(TIMEOUT);
		
		attempts = 0;	//clear attempts for next chunk
		
	} while (datalen==512);
	
	printf("Done!\n");
	free (chunk);
	
}

void writefile (char* filename){
	
	//open file
	FILE* f = fopen(filename, "rb");
	
	if (f == NULL){
		printf("ERROR: can't open file (client)\n");
	}
	
	char* chunk = malloc(1024);
	char* ackbuf = malloc(1024);
	
	//send write request
	WRQ req(filename, "octet");
	agent->send(req.toString(), req.size);
	alarm(TIMEOUT);
	
	printf("WRQ sent\n");
	
	int bytesRead;
	int blocknum = 0;
	
	bool lastblocksent=false;
	bool lastblockackd=false;
	
	int attempts=0;
	
	do {
		
		//while loop terminats when appropriate acknowledgement is recieved
		bool ackrecieved = false;
		while (!ackrecieved){
		
			//wait for ack
			int n = agent->recv(ackbuf, 1024);
			
			if (n == -1){
				//check for timer interrupt
				if (errno==EINTR){
					//timeout
					
					if (attempts == MAX_ATTEMPTS){
						//throw error and exit
						printf("FAILED: max attempts reached\n");
						return;
					}
					
					agent->sendprev();
					alarm(TIMEOUT);
					printf("TIMEOUT: retrying...\n");
					attempts++;
					continue;
				}
			}
			
			int p = packetType(ackbuf);
		
			
			if(p==pERROR){
				ERROR e(ackbuf);
				printf("ERROR %d: %s\n", e.errorcode, e.errorcode==0 ? e.errorstr : errormsgs[e.errorcode]);
				//close file and stop sending
				alarm(0);
				return;
			} else if (p==pACK){
				ACK a(ackbuf);
				if (a.blocknum == blocknum){
					ackrecieved = true;
					if (lastblocksent) lastblockackd = true;
					printf("RECIEVED ACK for BLOCK %d\n", blocknum);
					blocknum++;
				}
			} else {
				//WTF?? ignore and stop sending
			}
			
		}
		
		alarm(0);
		
		if (lastblockackd) break;
			
		//read chunk from file
		bytesRead = fread(chunk, 1, 512, f);
		
		if (bytesRead<512) {
			lastblocksent=true;
		}
		
		//send chunk
		DATA d(blocknum, chunk, bytesRead);
		agent->send(d.toString(), d.size);
		alarm(TIMEOUT);
		printf("BLOCK %d sent\n", blocknum);
		
		attempts=0;	//clear attempts for next acknowledgement
		
	} while (true);
	
	printf("Done!\n");
	
	free(chunk);
	free(ackbuf);
	
}

int main(){

	//add alarm handler
	signal (SIGALRM, alarmHandler);
	
	//tell OS not to restart system calls on alarm interrupt
	siginterrupt(SIGALRM, 1);
	
	agent = new udpAgent("127.0.0.1", 69);
	readfile("hw");
	return 0;
}
