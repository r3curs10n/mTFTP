#include <iostream>
#include <cstdio>
#include <csignal>
#include "udpAgent.h"
#include "packets.h"
#include <errno.h>
using namespace std;

#define MAX_ATTEMPTS 10
#define TIMEOUT 2
#define MAX_SEQ_NUM 0x10000

udpAgent* agent;

extern int errno;

void alarmHandler (int sig){
	//do nothing, system call interrupted
	//cout<<"debug"<<endl;
}

bool checkTID(){

	//check if TID is same as agreed upon
	if (agent->isSenderPortLocked()){
		if (!agent->isSenderPortValid()){
			//send error
			ERROR e(5, "");	//unknown transfer ID
			agent->send(e.toString(), e.size);
			cout<<"ERROR packet sent to unauthorized TID"<<endl;
			agent->restoreSenderPort();
			alarm(TIMEOUT);
			return false;
		}
	} else {
		//first packet from server, lock server's TID
		agent->lockSenderPort();
	}
	
	return true;
}

void readfile (char* filename){
	
	//open file
	FILE* f = fopen(filename, "wb");
	if (f == NULL){
		printf("ERROR: Can't open file (client)\n");
		return;
	}
	
	//send RRQ
	RRQ req(filename, "octet");
	agent->send(req.toString(), req.size);
	alarm(TIMEOUT);
	
	printf("RRQ sent\n");
	
	int datalen;
	int blocknum=0;
	int p, n;
	char* chunk = malloc(1024);
	int attempts=0;
	
	do {
		
		//this while loops terminates when a valid chunk is recieved
		
		bool gotChunk= false;
		while (!gotChunk){
		
			//wait for DATA
			n = agent->recv(chunk, 1024);
			
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
			
			//check if sender's TID is valid and send error packet if not
			if (!checkTID()) continue;
			
			p = packetType(chunk);
			
			if (p==pERROR){
				ERROR e(chunk);
				printf("ERROR %d: %s\n", e.errorcode, e.errorcode==0 ? e.errorstr : errormsgs[e.errorcode]);
				alarm(0);
				return;
			}
			else if (p==pDATA){
				DATA d(chunk, n);
				if (d.blocknum == (blocknum+1) % MAX_SEQ_NUM){
					//got required block
					gotChunk = true;
					printf("Recieved block %d\n", d.blocknum);
					blocknum++;
					blocknum %= MAX_SEQ_NUM;
				}
			} else {
				//WTF, ignore packet
				alarm(TIMEOUT);
			}
		
		}
		
		alarm(0);	//chunk recieved, alarm can now be turned off
		
		//read data
		DATA recieved(chunk, n);
		datalen = recieved.datalen;
		
		//write to file
		int n = fwrite(recieved.data, 1, datalen, f);
		if (n != datalen){
			//can't write to file, out of disk space
			ERROR e(3, "");	//out of disk space error code
			agent->send(e.toString(), e.size);
			return;
		}
		
		//Acknowledge
		ACK gotit((unsigned short)recieved.blocknum);
		agent->send(gotit.toString(), gotit.size);
		alarm(TIMEOUT);
		printf("SENT ACK for block %d\n", gotit.blocknum);
		
		attempts = 0;	//clear attempts for next chunk
		
	} while (datalen==512);
	
	fclose(f);
	printf("Done!\n");
	free (chunk);
	
}

void writefile (char* filename){
	
	//open file
	FILE* f = fopen(filename, "rb");
	
	if (f == NULL){
		printf("ERROR: can't open file (client)\n");
		return;
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
			
			//check if sender's TID is valid and send error packet if not
			if (!checkTID()) continue;
			
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
					blocknum %= MAX_SEQ_NUM;
				}
			} else {
				//WTF?? ignore packet
				alarm(TIMEOUT);
			}
			
		}
		
		alarm(0);	//turn off alarm
		
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
	
	fclose(f);
	printf("Done!\n");
	
	free(chunk);
	free(ackbuf);
	
}

void printusage(){
	printf("Usage: ./tftpclient [-h hostname] [-p port] -r/-w filename\n");
}

int main(int argc, char* argv[]){

	//add alarm handler
	signal (SIGALRM, alarmHandler);
	
	//tell OS not to restart system calls on alarm interrupt
	siginterrupt(SIGALRM, 1);
	
	char* hostname = "127.0.0.1";
	int port = 69;
	char* filename = NULL;
	
	int todo = -1;	//0: read, 1: write
	
	//parse arguments
	if (argc==1){
		printusage();
		return 0;
	}
	argv++;
	argc--;
	if (strcmp(argv[0], "-h")==0){
		if (argc==1) {printusage(); return 0;}
		hostname = argv[1];
		
		argv+=2;
		argc-=2;
	}
	if (strcmp(argv[0], "-p")==0){
		if (argc==1) {printusage(); return 0;}
		port = atoi(argv[1]);
		
		argv+=2;
		argc-=2;
	}
	if (strcmp(argv[0], "-r")==0){
		if (argc==1) {printusage(); return 0;}
		filename = argv[1];
		todo = 0;
		
		argv+=2;
		argc-=2;
	} else if (strcmp(argv[0], "-w")==0){
		if (argc==1) {printusage(); return 0;}
		filename = argv[1];
		todo = 1;
		
		argv+=2;
		argc-=2;
	}
	
	//do stuff
	
	try {
		agent = new udpAgent(hostname, port);
	} catch (string ex){
		cout<<ex<<endl;
		return 0;
	}
	
	if (todo==0){
		readfile(filename);
	} else if (todo==1){
		writefile(filename);
	}
	
	return 0;
}
