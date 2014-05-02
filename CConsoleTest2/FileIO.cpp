#include "FileIO.h"
#include "windows.h"
#include "thread"

FileIO::FileIO()
{
	_readFileName = L"";
	_readOffset = 0;
	hReadFile = INVALID_HANDLE_VALUE;
	_isReadRunning = false;
	_readSync.hEvent = INVALID_HANDLE_VALUE;
	_readBufSize = READ_BUF_SIZE;
	_readBuf = new char[_readBufSize];

	_writeFileName = L"";
	hWriteFile = INVALID_HANDLE_VALUE;

	//BRHandlersNum = 0;
}

FileIO::FileIO(unsigned long readBufSize)
{
	_readFileName = L"";
	_readOffset = 0;
	hReadFile = INVALID_HANDLE_VALUE;
	_isReadRunning = false;
	_readSync.hEvent = INVALID_HANDLE_VALUE;
	_readBufSize = readBufSize;
	_readBuf = new char[_readBufSize];

	_writeFileName = L"";
	hWriteFile = INVALID_HANDLE_VALUE;

	//BRHandlersNum = 0;
}

FileIO::~FileIO()
{
	delete _readBuf;
}

void FileIO::SetReadFile(wchar_t* readFile)
{
	delete _readFileName;
	_readFileName = readFile;
	_readFileName = 0;
}

void FileIO::SetWriteFile(wchar_t* writeFile)
{
	delete _writeFileName;
	_writeFileName = writeFile;
}

//bool FileIO::AttachBRHandler(bool(Communication::*newHandler)(char*,unsigned long))
//{
//	if(BRHandlersNum > MAX_HANDLERS_NUM)
//	{
//		return false;
//	}
//	BytesReadHandlers[BRHandlersNum++] = newHandler;
//	return true;
//}

bool FileIO::ReadFileBytes(unsigned int bytesNum)
{
	if (_readFileName == L"" || bytesNum > _readBufSize)
		return false;

	while(_isReadRunning)
	{
		Sleep(0);
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
			return false;
	}
	else
	{
		_readSync.Offset = _readOffset;
	}

	char* content = new char(bytesNum);

	if(!ReadFile(hReadFile, content, bytesNum, NULL, &_readSync))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			//MessageBox(NULL, L"WriteFile error", L"Error", MB_OK);
			return false;
		}
	}

	_isReadRunning = true;
	std::thread readThread(&FileIO::ReadThread, std::ref(*this));
	readThread.detach();
	return true;
}

bool FileIO::WriteBytes(unsigned char* bytes, unsigned int bytesNum)
{
	return true;
}

void FileIO::ReadThread()
{
	if(!GetOverlappedResult(hReadFile, &_readSync, &_bytesRead, true))
	{
		MessageBox(NULL, L"GetOverlappedResult error on write", L"Error", MB_OK);
	}

	if(_bytesRead < _bytesRequested)
	{
		CloseHandle(hReadFile);
		_readOffset = 0;
	}
	else _readOffset += _bytesRead;

	/*for (int i = 0; i < BRHandlersNum; i++)
	{
		(*BytesReadHandlers[i])(_readBuf, _bytesRead);
	}*/

	_isReadRunning = false;
	return;
}