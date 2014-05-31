#ifndef COMMMANAGER_H
#define COMMMANAGER_H

#define MANAG_SIZE_RECBUF (1024*1024)

#include "FileIO.h"
#include "SerialComm.h"
#include "PacketMaker.h"

class CommManager : public SerialCommSubscribable, public FileIOSubscribable 
{
	FileIO _fileIO;
	SerialComm _serialComm;
	PacketMaker _packetMaker;


	char* _sentFileBuf; //send buffer details
	unsigned long _sentBufCounter;
	unsigned long _bytesLeftToSend;

	char* _recFileBuf;//receive buffer details
	bool _isRecBufEmpty;
	unsigned long _recBufCounter;
	unsigned long _recBufLeft;

	//thread funcs
	void _writeThread();

public:
	CommManager();
	~CommManager();

	bool Connect(const wchar_t* port, int baudrate, unsigned int& errMessage){return _serialComm.Connect(port,baudrate,errMessage);}
	void TransmitFile(wchar_t*);
	void RewriteFile(wchar_t*);

	void SerialBytesReceivedHandler(char*, unsigned int);
	void SerialBaudrateChangedHandler(unsigned long);
	void SerialNothingReceivedHandler();

	void FileBytesReadHandler(char*, unsigned long, bool);
	void FileBytesWrittenHandler();
};

#endif