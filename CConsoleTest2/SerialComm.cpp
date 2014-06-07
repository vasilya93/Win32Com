#include "SerialComm.h"
#include "windows.h"
#include "assert.h"
#include "thread"

using namespace std;

//---------------------------------------Constructors --------------------------------------

SerialComm::SerialComm()
{
	_packetCounter = 0;

	_writeSync.hEvent = INVALID_HANDLE_VALUE;
	HandlersNum = 0;

	_firstTick = 0;
	_lastTick = 0;

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

bool SerialComm::ResetTick(unsigned long* firstTick, unsigned long* lastTick)
{
	if (firstTick==NULL || lastTick==NULL)
	{
		return false;
	}
	*firstTick = _firstTick;
	*lastTick = _lastTick;
	_firstTick = 0;
	_lastTick = 0;
	return true;
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
	dcb.fOutxCtsFlow = TRUE;
	dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	bSuccess = SetCommState(hPort, &dcb);
	if(!bSuccess)
	{
		errMessage = COMM_EM_STSTFL;
		return false;
	}
	GetCommState(hPort, &dcb);
	COMMTIMEOUTS Timeouts = {1, 0, 100/*timeoutConstant*/, 0, 0};
	bSuccess = SetCommTimeouts(hPort, &Timeouts);
	if(!bSuccess)
	{
		errMessage = COMM_EM_STTMFL;
		return false;
	}

	_isReadingContinued = true;

	ReadThread = std::thread(&SerialComm::_readThread, std::ref(*this));
	//TimeThread = std::thread(&SerialComm::_timingThread, std::ref(*this));//launches the thread
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

	while(_isWriteRunning);

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
			unsigned long error = GetLastError();
			wchar_t* errorMessage = new wchar_t[200];
			//wcscpy_s(errorMessage, 200, L"WriteFile:");
			//unsigned long length = wcslen(errorMessage);
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, errorMessage, 200, NULL);
			MessageBox(NULL, errorMessage, L"Error", MB_OK);
			delete errorMessage;
			_isWriteRunning = false;
			return false;
		}
	}

	_packetCounter++;
	_internalHigh = _writeSync.InternalHigh;

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
		unsigned long error = GetLastError();
		wchar_t* errorMessage = new wchar_t[200];
		wcscpy_s(errorMessage, 200, L"GetOverlapped:");
		unsigned long length = wcslen(errorMessage);
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, &(errorMessage[length + 1]), 200-length, NULL);
		MessageBox(NULL, errorMessage, L"Error", MB_OK);
		delete errorMessage;
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
		isSuccess = ReadFile(hPort, ReadBuf, (ReadBufSize - 1), NULL, &sync); //зачем делать еще GetOverlapped result
																			  //логгировать фэйлы в файл
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
			return; //катастрофически же
		}
		if(bytesRead > 0)
		{
			_lastTick = GetTickCount();
			if(_firstTick == 0)
			{
				_firstTick = _lastTick;
			}
			ReadBuf[bytesRead] = '\0';
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