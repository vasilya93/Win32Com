#ifndef FILEIO_H
#define FILEIO_H

#define MAX_HANDLERS_NUM 5
#define READ_BUF_SIZE 512

#include "windows.h"

class FileIO
{
	wchar_t* _readFileName;
	unsigned long _readOffset; //is it necessary to store file name - yep, for reading
	HANDLE hReadFile;
	OVERLAPPED _readSync;

	unsigned long _bytesRead;
	unsigned long _bytesRequested;

	bool _isReadRunning;
	unsigned long _readBufSize;
	char* _readBuf;

	wchar_t* _writeFileName;
	HANDLE hWriteFile;

	//bool (Communication::*BytesReadHandlers[MAX_HANDLERS_NUM])(void*, char*, unsigned long);
	//unsigned short BRHandlersNum;

public:
	FileIO();
	FileIO(unsigned long readBufSize);
	~FileIO();

	void SetReadFile(wchar_t* readFile);
	void SetWriteFile(wchar_t* _writeFile);

	//bool AttachBRHandler(bool(Communication::*newHandler)(void*,char*,unsigned long));

	bool ReadFileBytes(unsigned int bytesNum); //getting bytes with offset
	bool WriteBytes(unsigned char* bytes, unsigned int bytesNum); //it is the task of the method to write all bytes that is why
																	//no number of written bytes is referenced

	void ReadThread();
};

#endif