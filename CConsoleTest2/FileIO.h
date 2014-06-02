#ifndef FILEIO_H
#define FILEIO_H

#define MAX_HANDLERS_NUM 5
#define FILE_MAX_BUF_SIZE (1024 * 1024 * 10)
#define FILE_READ_FULL -1

#include "windows.h"

class FileIOSubscribable
{
public:
	FileIOSubscribable(){}
	virtual ~FileIOSubscribable(){};
	virtual void FileBytesReadHandler(char*, unsigned long, bool) = 0;
	virtual void FileBytesWrittenHandler() = 0;
};

class FileIO
{
	//-------------------------------Read elements -------------------------------
	wchar_t* _readFileName; //file data
	HANDLE hReadFile;

	OVERLAPPED _readSync; // thread data
	bool _isReadRunning;
	
	unsigned long _readOffset; // current operation data
	unsigned long _bytesRequested;
	unsigned long _bytesRead;
	bool _isFullFile;

	long _readBufSize; //buffer data
	char* _readBuf; //it is necessary to ensure that the buffer is read before running the next read operation;
					//it would be enough if the handlers wouldn't run threads without copying the buffer;

	FileIOSubscribable* BRHandlerHosts[MAX_HANDLERS_NUM]; //handlers data
	unsigned short BRHandlersNum;

	void _readThread();

	//-------------------------------Write elements -------------------------------

	wchar_t* _writeFileName; //file data
	HANDLE hWriteFile;

	OVERLAPPED _writeSync; // thread data
	bool _isWriteRunning;

	unsigned long _bytesWritten; // current operation data

	void _writeThread();

public:
	//Constructors and destructors
	FileIO();
	~FileIO();

	unsigned long GetBytesRead();
	char* GetReadBuf();

	void SetReadFile(wchar_t* readFile);
	void SetWriteFile(wchar_t* _writeFile);

	bool AttachBRHandler(FileIOSubscribable* handlerHost);

	bool ReadFileBytes(int bytesNum); //getting bytes with offset
	bool WriteBytes(char* bytes, unsigned int bytesNum, bool doAppend); //it is the task of the method to write all bytes that is why
																	//no number of written bytes is referenced
};

#endif