#include <iostream>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
using namespace std;

//structs to do gruntwork (serialization and deserialization)

struct RRQ {
	unsigned short opcode;
	char* filename;
	char* mode;
	int size;
	
	RRQ (){}
	
	RRQ (char* _filename, char* _mode){
		filename = malloc(1024);
		mode = malloc(1024);
		strcpy(filename, _filename);
		strcpy(mode, _mode);
		opcode = (unsigned short) 1;
		size = strlen(filename) + strlen(mode) + 1 + 1 + 2;
		//cout<<"done\n";
	}
	
	RRQ (char* packet){
		opcode = 1;
		filename = malloc(1024);
		mode = malloc(1024);
		//cout<<"dhkhkjhkhkkj"<<endl;
		strcpy(filename, packet + 2);
		
		strcpy(mode, packet + 2 + strlen(filename) + 1);
		size = strlen(filename) + strlen(mode) + 1 + 1 + 2;
	}
	
	char* toString(){
		char* packet = malloc(size);
		*((unsigned short*)&packet[0]) = htons(opcode);
		strcpy(packet+2, filename);
		strcpy(packet+2+strlen(filename)+1, mode);
		return packet; 
	}
};

struct WRQ {
	unsigned short opcode;
	char* filename;
	char* mode;
	int size;
	
	WRQ (){}
	
	WRQ (char* _filename, char* _mode){
		filename = malloc(1024);
		mode = malloc(1024);
		strcpy(filename, _filename);
		strcpy(mode, _mode);
		opcode = (unsigned short) 2;
		size = 2 + strlen(filename) + strlen(mode) + 1 + 1;
	}
	
	WRQ (char* packet){
		opcode = 2;
		filename = malloc(1024);
		mode = malloc(1024);
		strcpy(filename, packet + 2);
		strcpy(mode, packet + 2 + strlen(filename) + 1);
		size = 2 + strlen(filename) + strlen(mode) + 1 + 1;
	}
	
	char* toString(){
		char* packet = malloc(size);
		*((unsigned short*)&packet[0]) = htons(opcode);
		strcpy(packet+2, filename);
		strcpy(packet+2+strlen(filename)+1, mode);
		return packet; 
	}
};

struct DATA {
	unsigned short opcode;
	unsigned short blocknum;
	char* data;
	int datalen;
	int size;
	
	DATA (){}
	
	DATA (unsigned short _blocknum, char* _data, int _datalen){
		blocknum = _blocknum;
		data = _data;
		datalen = _datalen;
		opcode = (unsigned short) 3;
		size = 2 + 2 + datalen + 1;
	}
	
	DATA (char* packet){
		opcode = 3;
		blocknum = ntohs(*((unsigned short*)&packet[2]));
		data = malloc(1024);
		strcpy(data, packet+2+2);
		datalen = strlen(data);
		size = 2 + 2 + datalen + 1;
	}
	
	char* toString(){
		char* packet = malloc(size);
		*((unsigned short*)&packet[0]) = htons(opcode);
		*((unsigned short*)&packet[2]) = htons(blocknum);
		strncpy(packet+2+2, data, datalen);
		packet[2+2+datalen]=0;
		return packet; 
	}
	
};

struct ACK {
	unsigned short opcode;
	unsigned short blocknum;
	int size;
	
	ACK (){}
	
	ACK (unsigned short _blocknum){
		blocknum = _blocknum;
		opcode = (unsigned short) 4;
		size = 2 + 2;
	}
	
	ACK (char* packet){
		opcode = 4;
		blocknum = ntohs(*((unsigned short*)&packet[2]));
		size = 2 + 2;
	}
	
	char* toString(){
		char* packet = malloc(size);
		*((unsigned short*)&packet[0]) = htons(opcode);
		*((unsigned short*)&packet[2]) = htons(blocknum);
		return packet; 
	}
};

struct ERROR {
	unsigned short opcode;
	unsigned short errorcode;
	char* errorstr;
	
	ERROR (unsigned short _errorcode, char* _errorstr){
		errorcode = _errorcode;
		errorstr = _errorstr;
		opcode = (unsigned short) 5;
	}
	
};
