#include "FileIO.h"
#include "windows.h"
#include "thread"

FileIO::FileIO()
{
	_readFileName = L"";
	_readOffset = 0;
	hReadFile = INVALID_HANDLE_VALUE;
	hWriteFile = INVALID_HANDLE_VALUE;
	_isReadRunning = false;
	_isWriteRunning = false;
	_readSync.hEvent = INVALID_HANDLE_VALUE;
	_writeSync.hEvent = INVALID_HANDLE_VALUE;
	_readBufSize = 0;
	_readBuf = new char[0];

	_writeFileName = L"";
	hWriteFile = INVALID_HANDLE_VALUE;

	BRHandlersNum = 0;
}

FileIO::~FileIO()
{
	delete _readBuf;
}

unsigned long FileIO::GetBytesRead()
{
	return _bytesRead;
}

char* FileIO::GetReadBuf()
{
	return _readBuf;
}

void FileIO::SetReadFile(wchar_t* readFile)
{
	//delete _readFileName;
	_readFileName = readFile;
	_readOffset = 0;
}

void FileIO::SetWriteFile(wchar_t* writeFile)
{
	_writeFileName = writeFile;
}

bool FileIO::AttachBRHandler(FileIOSubscribable* newHost)
{
	if(BRHandlersNum > MAX_HANDLERS_NUM)
	{
		return false;
	}
	BRHandlerHosts[BRHandlersNum++] = newHost;
	return true;
}

bool FileIO::ReadFileBytes(int bytesNum)
{
	if(bytesNum > FILE_MAX_BUF_SIZE)
	{
		throw "more than 1 mb is requested to read from file";
		return false;
	}

	while(_isReadRunning)
	{
		Sleep(0);
	}

	delete _readBuf;
	_readBuf = NULL;

	if (_readFileName == L"")
	{
		_readBuf = "";
		_readBufSize = 1;
		_bytesRead = 0;
		return false;
	}

	if(_readSync.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_readSync.hEvent);
	}
	ZeroMemory(&_readSync, sizeof(OVERLAPPED));
	_readSync.hEvent = CreateEvent(NULL, false, false, NULL);

	if(_readOffset == 0)
	{
		hReadFile = CreateFile(_readFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if(hReadFile == INVALID_HANDLE_VALUE)
		{
			throw "invalid handle value on file read";
			return false;
		}
	}
	else
	{
		_readSync.Offset = _readOffset;
	}

	if(bytesNum == FILE_READ_FULL)
	{
		_bytesRequested = GetFileSize(hReadFile, NULL);
		if(_bytesRequested == INVALID_FILE_SIZE)
		{
			throw "invalid file size";
			return false;
		}
		_isFullFile = true;
	}
	else
	{
		_isFullFile = false;
		_bytesRequested = bytesNum;
	}
	_readBuf = new char[_bytesRequested + 1];

	if(!ReadFile(hReadFile, _readBuf, _bytesRequested, NULL, &_readSync))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			throw "error on ReadFile operation";
			return false;
		}
	}

	_isReadRunning = true;
	std::thread readThread(&FileIO::_readThread, std::ref(*this));
	readThread.detach();

	return true;
}

bool FileIO::WriteBytes(char* bytes, unsigned int bytesNum, bool doAppend)
{
	if (_writeFileName == L"" || bytes == NULL)
	{
		throw "no write file or null bytes transmitted";
		return false;
	}

	while(_isWriteRunning)
	{
		Sleep(0);
	}

	if(_writeSync.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_writeSync.hEvent);
	}
	ZeroMemory(&_writeSync, sizeof(OVERLAPPED));
	_writeSync.hEvent = CreateEvent(NULL, false, false, NULL);

	unsigned long creationDisp = CREATE_ALWAYS;
	if(doAppend)
		creationDisp = OPEN_ALWAYS;
	hWriteFile = CreateFile(_writeFileName, GENERIC_WRITE, 0, NULL, creationDisp, FILE_FLAG_OVERLAPPED, NULL);
	if(hWriteFile == INVALID_HANDLE_VALUE)
	{
		throw "Failed to open written file";
		return false;
	}

	unsigned long fileSize = GetFileSize(hWriteFile, NULL);
	if (fileSize == INVALID_FILE_SIZE)
	{
		throw "Invalid file size on writing";
		return false;
	}
	_writeSync.Offset = fileSize;

	if(!WriteFile(hWriteFile, bytes, bytesNum, &_bytesWritten, &_writeSync))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			throw "WriteFile operation failed";
			return false;
		}
	}

	_isWriteRunning = true;
	std::thread writeThread(&FileIO::_writeThread, std::ref(*this));
	writeThread.detach();

	return true;
}

void FileIO::_readThread()
{
	if(!GetOverlappedResult(hReadFile, &_readSync, &_bytesRead, true))
	{
		throw "Getoverlapped result returned false";
	}

	bool isLast = (_bytesRead < _bytesRequested) || _isFullFile;
	if(isLast)
	{
		CloseHandle(hReadFile);
		hReadFile = INVALID_HANDLE_VALUE;
		_readOffset = 0;
	}
	else _readOffset += _bytesRead;

	_readBuf[_bytesRead] = '\0';

	for (int i = 0; i < BRHandlersNum; i++)
	{
		BRHandlerHosts[i]->FileBytesReadHandler(_readBuf, _bytesRead, isLast);
	}

	delete _readBuf;
	_readBuf = NULL;

	_isReadRunning = false;
	return;
}

void FileIO::_writeThread()
{
	if(!GetOverlappedResult(hWriteFile, &_writeSync, &_bytesWritten, true))
	{
		throw "Getoverlapped result returned false on write to file";
	}

	for (int i = 0; i < BRHandlersNum; i++)
	{
		BRHandlerHosts[i]->FileBytesWrittenHandler();
	}

	CloseHandle(hWriteFile);
	_isWriteRunning = false;

	return;
}