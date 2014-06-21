#ifndef COMMMANAGER_H
#define COMMMANAGER_H

#define MANAG_SIZE_RECBUF (1024*1024*10)

#include "FileIO.h"
#include "SerialComm.h"
#include "PacketMaker.h"
#include "Log.h"

class CommManager : public SerialCommSubscribable, public FileIOSubscribable 
{
	// Linked objects
	FileIO* _fileIO;
	SerialComm _serialComm;
	PacketMaker _packetMaker;
	Log* _log;


	char* _sentFileBuf; //send buffer details
	unsigned long _sentBufCounter;
	unsigned long _bytesLeftToSend;

	char* _recFileBuf;//receive buffer details
	bool _isRecBufEmpty;
	unsigned long _recBufCounter;
	unsigned long _recBufHandledCounter; 
	unsigned long _recBufLeft;

	unsigned int _corruptedPacketsNum;

	char* _writeFileBuf;//write to file buffer details
	unsigned long _writeBufCounter;
	unsigned long _writeBufLeft;


	//thread funcs
	void _mainThreadFunc();
	//possible situations main thread should handle:
	//1. request for writing sent buffer to serial
	bool _doWriteToSerial;
	//2. request for writing rec buffer to file
	bool _doWriteToFile;

	//thread auxiliary funcs
	void _writeSentBufToSerial();
	void _writeRecBufToFile();

	//thread details
	bool _isMainThreadRunning;
	std::thread _mainThread;

public:
	CommManager();
	~CommManager();

	bool Connect(const wchar_t* port, int baudrate, unsigned int& errMessage){return _serialComm.Connect(port,baudrate,errMessage);}
	void TransmitFile(wchar_t*);
	void WriteFile(wchar_t* fileName);
	void GetTransactParams(unsigned long&, unsigned long&, unsigned long&);

	//Log wrappers
	void SaveLogToFile(){_log->SaveToFile();}
	void ClearLog(){_log->Clear();}

	void SerialBytesReceivedHandler(char*, unsigned int);
	void SerialBaudrateChangedHandler(unsigned long);
	void SerialNothingReceivedHandler();

	void FileBytesReadHandler(char*, unsigned long, bool);
	void FileBytesWrittenHandler();
};

#endif