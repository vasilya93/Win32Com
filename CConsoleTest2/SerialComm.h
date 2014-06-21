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

#define READ_BUF_SIZE 1000
#define MAX_HANDLERS_NUMBER 5

#include "windows.h"
#include "thread"
#include "mutex"
#include "FileIO.h"
#include "Log.h"

class SerialCommSubscribable
{
public:
	SerialCommSubscribable(){}
	~SerialCommSubscribable(){}

	virtual void SerialBytesReceivedHandler(char*, unsigned int) = 0;
	virtual void SerialBaudrateChangedHandler(unsigned long) = 0;
	virtual void SerialNothingReceivedHandler() = 0;
};

class SerialComm
{
	//-----------------------------Can delete after -----------------------------

	unsigned int _packetCounter;
	unsigned long _internalHigh;


	//-----------------------------Read/write common -----------------------------

	HANDLE hPort;

	//-------------------------------Read elements -------------------------------
	unsigned long _firstTick;  //data for actual baudrate calculation, single transaction
	unsigned long _lastTick;
	
	bool _isReadingContinued; //thread data	
	
	unsigned short ReadBufSize; //buffer data
	char* ReadBuf;

	//-------------------------------Write elements -------------------------------

	OVERLAPPED _writeSync; //thread data
	bool _isWriteRunning;
	volatile bool* _isLineUsed;//means line transferred to be transmitted
	
	void _writeThread();
	void _timingThread();
	void _readThread();

public:
//-----------------------------Linked objects -----------------------------

	Log* CommLog;

	std::thread ReadThread;
	std::thread TimeThread;
	std::thread WriteThread;

	SerialCommSubscribable* HandlersHosts[MAX_HANDLERS_NUMBER];	
	unsigned short HandlersNum;

	SerialComm();
	~SerialComm();

	HANDLE GetPort();
	OVERLAPPED GetSync();
	char* GetReadBuf();
	bool ResetTick(unsigned long*, unsigned long*);

	bool AttachHandlerHost(SerialCommSubscribable* newHost);
	bool Connect(const wchar_t* port, int baudrate, unsigned int& errMessage);
	void Disconnect();

	SerialComm& operator = (SerialComm& comm);

	bool Write(char* line, unsigned long lineSize, volatile bool* isLineUsed);
};

#endif