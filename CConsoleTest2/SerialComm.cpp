#include "SerialComm.h"
#include "windows.h"
//#include "stdio.h"
#include "assert.h"
#include "thread"

using namespace std;

//---------------------------------------Constructors --------------------------------------

SerialComm::SerialComm()
{
	_packetCounter = 0;

	_receivedTotal = 0;
	_writeSync.hEvent = INVALID_HANDLE_VALUE;
	HandlersNum = 0;
	_lastTimeCount = 0;
	_isReadingContinued = false;
	_isWriteRunning = false;
	ReadBufSize = READ_BUF_SIZE + 1;
	ReadBuf = new char[ReadBufSize];
	hPort = 0;
}

//---------------------------------------Destructors --------------------------------------

SerialComm::~SerialComm()
{
	Disconnect();
	delete ReadBuf;
}

//---------------------------------------Access funcs --------------------------------------

HANDLE SerialComm::GetPort()
{
	return hPort;
}

OVERLAPPED SerialComm::GetSync()
{
	return _writeSync;
}

char* SerialComm::GetReadBuf()
{
	return ReadBuf;
}

unsigned long SerialComm::GetLastTime()
{
	return _lastTimeCount;
}

unsigned long SerialComm::GetBaudrate()
{
	return _actualBaudrate;
}

//-----------------------------------------Functions ----------------------------------------

bool SerialComm::AttachHandlerHost(SerialCommSubscribable* newHost)
{
	if(HandlersNum > MAX_HANDLERS_NUMBER)
	{
		return false;
	}
	HandlersHosts[HandlersNum++] = newHost;
	return true;
}

bool SerialComm::Connect(const wchar_t* port, int baudrate, unsigned int& errMessage)
{
	if(port == L"")
	{
		errMessage = COMM_EM_NOSERSEL;
		return false;
	}
	hPort = CreateFile(port, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if(hPort == INVALID_HANDLE_VALUE)
	{
		errMessage = COMM_EM_CNTOPNSER;
		return false;
	}
	DCB dcb;
	ZeroMemory(&dcb, sizeof(DCB));
	BOOL bSuccess;
	bSuccess = GetCommState(hPort, &dcb);
	if(!bSuccess)
	{
		errMessage = COMM_EM_GTSTFL;
		return false;
	}
	dcb.BaudRate = (DWORD)baudrate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	bSuccess = SetCommState(hPort, &dcb);
	if(!bSuccess)
	{
		errMessage = COMM_EM_STSTFL;
		return false;
	}
	GetCommState(hPort, &dcb);	//int timeoutConstant = (1.0/dwSelectedBaudrate*9*BUFFER_SIZE)/0.001;
	COMMTIMEOUTS Timeouts = {0, 0, 100/*timeoutConstant*/, 0, 0};
	bSuccess = SetCommTimeouts(hPort, &Timeouts);
	if(!bSuccess)
	{
		errMessage = COMM_EM_STTMFL;
		return false;
	}

	_isReadingContinued = true;

	_lastTimeCount = GetTickCount();
	ReadThread = std::thread(&SerialComm::_readThread, std::ref(*this));
	TimeThread = std::thread(&SerialComm::_timingThread, std::ref(*this));//launches the thread
	//when the function returns - the thread terminates

	return true;
}

void SerialComm::Disconnect()
{//when receiving and sending will be asynchronous it will be necessary
	//to check port statuses (or overlapped, or events) to ensure it's not
	//in the middle of an operation
	if(hPort != 0)
	{
		_isReadingContinued = false;
		while(_isWriteRunning);
		WriteThread.join();
		ReadThread.join();
		TimeThread.join();
		CloseHandle(hPort);
		hPort = 0;
	}
}

bool SerialComm::Write(char* line, unsigned long lineSize, bool* isLineUsed)
{
	assert(isLineUsed != NULL);
	assert(*isLineUsed = true);
	_isLineUsed = isLineUsed;

	if(_packetCounter%23 == 0 && _packetCounter != 0)
	{
		int i = 1;
	}

	while(_isWriteRunning)
	{
		Sleep(0);
	}

	_isWriteRunning = true;

	if(_writeSync.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_writeSync.hEvent);
	}
	ZeroMemory(&_writeSync, sizeof(OVERLAPPED));
	_writeSync.hEvent = CreateEvent(NULL, false, false, NULL);
		
	if(!WriteFile(hPort, line, lineSize, NULL, &_writeSync))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			//MessageBox(NULL, L"WriteFile error", L"Error", MB_OK);
			_isWriteRunning = false;
			return false;
		}
	}

	_packetCounter++;

	WriteThread = std::thread(&SerialComm::_writeThread, std::ref(*this));
	WriteThread.detach();

	return true;
}

//---------------------------------------Threads --------------------------------------

void SerialComm::_writeThread()
{
	unsigned long charsWritten;
	if(!GetOverlappedResult(hPort, &_writeSync, &charsWritten, true))
	{
		wchar_t* message = new wchar_t[100];
		swprintf(message, 100, L"GetOverlappedResult error on write. Packet #%d", _packetCounter);
		MessageBox(NULL, message, L"Error", MB_OK);
		delete message;
	}

	_isWriteRunning = false;
	*_isLineUsed = false;

	return;
}

void SerialComm::_timingThread()
{
	//unsigned long currentCount;

	while(_isReadingContinued)
	{
		Sleep(1000);
	//	currentCount = GetTickCount();
	//	if(currentCount != LastTimeCount)
	//	{
	//		_baudrate = float(BytesPerSecond) * 9.0 / (((float)currentCount - (float)LastTimeCount)/1000.0);
	//		LastTimeCount = currentCount;
	//		BytesPerSecond = 0;
	//		for (unsigned int i = 0; i < HandlersNum; i++)
	//		{
	//			HandlersHosts[i]->SerialBaudrateChangedHandler(ReadBuf, bytesRead);
	//		}
	//	}
	}
}

void SerialComm::_readThread()
{
	unsigned long bytesRead;
	BOOL isSuccess;
	unsigned long error;
	HANDLE hReadCompEvent = CreateEvent(NULL, false, false, NULL);
	OVERLAPPED sync = {0};
	sync.hEvent = hReadCompEvent;
	while(_isReadingContinued)
	{
		isSuccess = ReadFile(hPort, ReadBuf, (ReadBufSize - 1), NULL, &sync);
		if(!isSuccess)
		{
			error = GetLastError();
			if(error != ERROR_IO_PENDING)
			{
				//return;
			}
		}
		isSuccess = GetOverlappedResult(hPort, &sync, &bytesRead, true);
		if(!isSuccess)
		{
			return;
		}
		if(bytesRead > 0)
		{
			ReadBuf[bytesRead] = '\0';
			_receivedTotal += bytesRead;
			for (unsigned int i = 0; i < HandlersNum; i++)
			{
				HandlersHosts[i]->SerialBytesReceivedHandler(ReadBuf, bytesRead);
			}
		}
		else
		{
			for (unsigned int i = 0; i < HandlersNum; i++)
			{
				HandlersHosts[i]->SerialNothingReceivedHandler();
			}
		}
	}
	return;
}