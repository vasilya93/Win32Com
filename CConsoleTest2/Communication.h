#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define COMM_EM_OK 0
#define COMM_EM_NOSERSEL 1
#define COMM_EM_CNTOPNSER 2
#define COMM_EM_GTSTFL 3
#define COMM_EM_STSTFL 4
#define COMM_EM_STTMFL 5
#define COMM_EM_THRCRFL 6
#define COMM_EM_TTHRCRFL 7

#define READ_BUF_SIZE 512
#define MAX_HANDLERS_NUMBER 5

#include "windows.h"
#include "thread"
#include "mutex"

class Communication
{
	bool _isReadingContinued;
	bool _isWriteThreadRunning;
	bool _isWriteThreadSet;

	bool justFlag;

	HANDLE hPort;

	OVERLAPPED _writeSync;
	std::mutex _writeSyncLock;

	unsigned short ReadBufSize;
	char* ReadBuf;

	unsigned long BytesPerSecond;
	unsigned long _baudrate;

	unsigned long LastTimeCount;

	void WriteWait();
	
public:
	std::thread ReadThread;
	std::thread TimeThread;
	std::thread WriteThread;

	void (*BytesReceivedHandlers[MAX_HANDLERS_NUMBER])(char* bytes, unsigned int bytesNumber);
	unsigned int BRHandlersNumber;

	void (*BaudrateChangedHandlers[MAX_HANDLERS_NUMBER])(unsigned long);
	unsigned int BCHandlersNumber;

	Communication();
 	~Communication();
 
	HANDLE GetPort();
	OVERLAPPED GetSync();
	char* GetReadBuf();
	unsigned long GetLastTime();
	unsigned long GetBaudrate();
 
	bool AttachBRHandler(void(*)(char*, unsigned int));
	bool AttachBCHandler(void(*)(unsigned long));
	bool Connect(const wchar_t* port, int baudrate, unsigned int& errMessage);
 	void Disconnect();

	Communication& operator = (Communication& comm);
 
	bool Write(char* line, unsigned int lineSize);

 	void Read();
	void CountTime();
};

#endif