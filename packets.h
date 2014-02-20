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

#define pRRQ 1
#define pWRQ 2
#define pDATA 3
#define pACK 4
#define pERROR 5

const char* errormsgs[] = {"Not defined", "File not found", "Access violation", "Disk full or allocation exceeded", "Illegal TFTP operation", "Unknown transfer ID", "File already exists", "No such user"};

int packetType(char* packet){
	return (int) ntohs( *( (unsigned short*)packet ) );
}

struct RRQ {
	unsigned short opcode;
	char* filename;
	char* mode;
	int size;
	
	char* packet;
	
	RRQ (){}
	
	RRQ (char* _filename, char* _mode){
		filename = malloc(1024);
		mode = malloc(1024);
		packet = NULL;
		strcpy(filename, _filename);
		strcpy(mode, _mode);
		opcode = (unsigned short) 1;
		size = strlen(filename) + strlen(mode) + 1 + 1 + 2;
	}
	
	RRQ (char* packet){
		this->packet = NULL;	//global
		opcode = 1;
		filename = malloc(1024);
		mode = malloc(1024);
		strcpy(filename, packet + 2);
		
		strcpy(mode, packet + 2 + strlen(filename) + 1);
		size = strlen(filename) + strlen(mode) + 1 + 1 + 2;
	}
	
	char* toString(){
		if (!packet) packet = malloc(size+10);
		*((unsigned short*)&packet[0]) = htons(opcode);
		strcpy(packet+2, filename);
		strcpy(packet+2+strlen(filename)+1, mode);
		return packet; 
	}
	
	~RRQ (){
		//cout<<"des RRQ enter"<<endl;
		free (filename);
		free (mode);
		if (packet) free (packet);
		//cout<<"des RRQ leave"<<endl;
	}
};

struct WRQ {
	unsigned short opcode;
	char* filename;
	char* mode;
	int size;
	
	char* packet;
	
	WRQ (){}
	
	WRQ (char* _filename, char* _mode){
		filename = malloc(1024);
		mode = malloc(1024);
		packet = NULL;
		strcpy(filename, _filename);
		strcpy(mode, _mode);
		opcode = (unsigned short) 2;
		size = 2 + strlen(filename) + strlen(mode) + 1 + 1;
	}
	
	WRQ (char* packet){
		this->packet = NULL;	//global
		opcode = 2;
		filename = malloc(1024);
		mode = malloc(1024);
		strcpy(filename, packet + 2);
		strcpy(mode, packet + 2 + strlen(filename) + 1);
		size = 2 + strlen(filename) + strlen(mode) + 1 + 1;
	}
	
	char* toString(){
		if (!packet) packet = malloc(size+10);
		*((unsigned short*)&packet[0]) = htons(opcode);
		strcpy(packet+2, filename);
		strcpy(packet+2+strlen(filename)+1, mode);
		return packet; 
	}
	
	~WRQ (){
		free (filename);
		free (mode);
		if (packet) free (packet);
	}
};

struct DATA {
	unsigned short opcode;
	unsigned short blocknum;
	char* data;
	int datalen;
	int size;
	
	char* packet;
	
	DATA (){}
	
	DATA (unsigned short _blocknum, char* _data, int _datalen){
		packet = NULL;
		data = malloc(1024);
		memcpy(data, _data, _datalen);
		blocknum = _blocknum;
		datalen = _datalen;
		opcode = (unsigned short) 3;
		size = 2 + 2 + datalen;
	}
	
	DATA (char* _packet, int _size){
		packet = NULL;	//global
		opcode = 3;
		blocknum = ntohs(*((unsigned short*)&_packet[2]));
		data = malloc(1024);
		memcpy(data, _packet+2+2, _size-2-2);
		datalen = _size-2-2;
		size = _size;
	}
	
	char* toString(){
		if (!packet) packet = malloc(size+10);
		*((unsigned short*)&packet[0]) = htons(opcode);
		*((unsigned short*)&packet[2]) = htons(blocknum);
		memcpy(packet+2+2, data, datalen);
		return packet; 
	}
	
	~DATA (){
		//cout<<"des DATA enter"<<endl;
		free (data);
		//cout<<"ffffff"<<endl;
		if (packet) free (packet);
		//cout<<"des DATA leave"<<endl;
	}
	
};

struct ACK {
	unsigned short opcode;
	unsigned short blocknum;
	int size;
	
	char* packet;
	
	ACK (){}
	
	ACK (unsigned short _blocknum){
		packet = NULL;
		blocknum = _blocknum;
		opcode = (unsigned short) 4;
		size = 2 + 2;
	}
	
	ACK (char* packet){
		this->packet = NULL;	//global
		opcode = 4;
		blocknum = ntohs(*((unsigned short*)&packet[2]));
		size = 2 + 2;
	}
	
	char* toString(){
		if (!packet) packet = malloc(size+10);
		*((unsigned short*)&packet[0]) = htons(opcode);
		*((unsigned short*)&packet[2]) = htons(blocknum);
		return packet; 
	}
	
	~ACK (){
		if (packet) free (packet);
	}
};

struct ERROR {
	unsigned short opcode;
	unsigned short errorcode;
	char* errorstr;
	
	char* packet;
	int size;
	
	ERROR (unsigned short _errorcode, char* _errorstr){
		packet = NULL;
		errorstr = malloc(1024);
		errorcode = _errorcode;
		strcpy(errorstr, _errorstr);
		opcode = (unsigned short) 5;
		size = 2 + 2 + strlen(errorstr) + 1;
	}
	
	ERROR (char* packet){
		this->packet = NULL;
		errorstr = malloc(1024);
		opcode = 5;
		errorcode = ntohs(*((unsigned short*)&packet[2]));
		strcpy(errorstr, packet+4);
		size = 2 + 2 + strlen(errorstr) + 1;
	}
	
	char* toString(){
		if (!packet) packet = malloc(size+10);
		*((unsigned short*)&packet[0]) = htons(opcode);
		*((unsigned short*)&packet[2]) = htons(errorcode);
		strcpy(packet+4, errorstr);
		return packet;
	}
	
	~ERROR (){
		if (packet) free(packet);
		free(errorstr);
	}
	
};
