#include "Communication.h"
#include "windows.h"
#include "thread"

using namespace std;

Communication::Communication()
{
	_writeSync.hEvent = INVALID_HANDLE_VALUE;
	BRHandlersNumber = 0;
	BCHandlersNumber = 0;
	LastTimeCount = 0;
	_isReadingContinued = false;
	_isWriteThreadRunning = false;
	_isWriteThreadSet = false;
	justFlag = false;
	ReadBufSize = READ_BUF_SIZE + 1;
	ReadBuf = new char[ReadBufSize];
	BytesPerSecond = 0;
	hPort = 0;
}

Communication::~Communication()
{
	Disconnect();
	delete ReadBuf;
}

HANDLE Communication::GetPort()
{
	return hPort;
}

OVERLAPPED Communication::GetSync()
{
	return _writeSync;
}

char* Communication::GetReadBuf()
{
	return ReadBuf;
}

unsigned long Communication::GetLastTime()
{
	return LastTimeCount;
}

unsigned long Communication::GetBaudrate()
{
	return _baudrate;
}

bool Communication::AttachBRHandler(void(*newHandler)(char*, unsigned int))
{
	if(BRHandlersNumber > MAX_HANDLERS_NUMBER)
	{
		return false;
	}
	BytesReceivedHandlers[BRHandlersNumber++] = newHandler;
	return true;

}

bool Communication::AttachBCHandler(void(*newHandler)(unsigned long))
{
	if(BCHandlersNumber > MAX_HANDLERS_NUMBER)
	{
		return false;
	}
	BaudrateChangedHandlers[BCHandlersNumber++] = newHandler;
	return true;
}

bool Communication::Connect(const wchar_t* port, int baudrate, unsigned int& errMessage)
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
	//int timeoutConstant = (1.0/dwSelectedBaudrate*9*BUFFER_SIZE)/0.001;
	COMMTIMEOUTS Timeouts = {0, 0, 100/*timeoutConstant*/, 0, 0};
	bSuccess = SetCommTimeouts(hPort, &Timeouts);
	if(!bSuccess)
	{
		errMessage = COMM_EM_STTMFL;
		return false;
	}

	_isReadingContinued = true;

	LastTimeCount = GetTickCount();
	ReadThread = std::thread(&Communication::Read, std::ref(*this));
	TimeThread = std::thread(&Communication::CountTime, std::ref(*this));//launches the thread
	//when the function returns - the thread terminates

	return true;
}

void Communication::Disconnect()
{//when receiving and sending will be asynchronous it will be necessary
	//to check port statuses (or overlapped, or events) to ensure it's not
	//in the middle of an operation
	if(hPort != 0)
	{
		_isReadingContinued = false;
		while(_isWriteThreadRunning);
		WriteThread.join();
		ReadThread.join();
		TimeThread.join();
		CloseHandle(hPort);
		hPort = 0;
	}
}

Communication& Communication::operator =(Communication& comm)
{
	if(this != &comm)
	{
		LastTimeCount = comm.GetLastTime();
		*BytesReceivedHandlers = *comm.BytesReceivedHandlers;  //look for error here
		BRHandlersNumber = comm.BRHandlersNumber;
		*BaudrateChangedHandlers = *comm.BaudrateChangedHandlers;
		BCHandlersNumber = comm.BCHandlersNumber;
		hPort = comm.GetPort();
		_writeSync = comm.GetSync(); //somehow input mutex here
		ReadBuf = comm.GetReadBuf();
		ReadThread = std::move(comm.ReadThread);
		TimeThread = std::move(comm.TimeThread);
		WriteThread = std::move(comm.WriteThread); 
	}
	return *this;
}

bool Communication::Write(char* line, unsigned int lineSize)
{
	while(_isWriteThreadRunning || _isWriteThreadSet)
	{
		Sleep(0);
	}

	if(_writeSync.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_writeSync.hEvent);
	}
	ZeroMemory(&_writeSync, sizeof(OVERLAPPED));

	HANDLE hWriteCompEvent = CreateEvent(NULL, false, false, NULL);	
	_writeSync.hEvent = hWriteCompEvent;
	
	if(!WriteFile(hPort, line, lineSize, NULL, &_writeSync))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			//MessageBox(NULL, L"WriteFile error", L"Error", MB_OK);
			return false;
		}
	}

	_isWriteThreadSet = true;
	WriteThread = std::thread(&Communication::WriteWait, std::ref(*this));
	WriteThread.detach();
	//WriteFile(hPort, line, lineSize, NULL, &_writeSync);
	//GetOverlappedResult(hPort, &_writeSync, &charsWritten, true);

	return true;
	/*else
	{
		lpszEditLine = (wchar_t*)realloc(lpszEditLine, (dwCharsNumber + 3)*sizeof(wchar_t));
		lpszEditLine[dwCharsNumber]=L'\r';
		lpszEditLine[dwCharsNumber+1]=L'\n';
		lpszEditLine[dwCharsNumber+2]=L'\0';
		HWND hSendEdit = GetDlgItem(hWnd, IDC_SEND_EDIT);
		AppendTextToEdit(lpszEditLine, hSendEdit);
	}*/
}

//пересечение потоков; один поток модифицирует _writeSync пока другой использует его
void Communication::WriteWait()
{
	_isWriteThreadRunning = true;

	unsigned long charsWritten;
	if(!GetOverlappedResult(hPort, &_writeSync, &charsWritten, true))
	{
		MessageBox(NULL, L"GetOverlappedResult error on write", L"Error", MB_OK);
		_isWriteThreadRunning = false;
		_isWriteThreadSet = false;
		return;
	}

	/*_isWriteThreadRunning = false;
	int a = 1;
	int b = 45;
	if(b > 78)
	{
		MessageBox(0, L"sdfsd", L"sdfsdf", MB_OK);
	}*/
	_isWriteThreadRunning = false;
	_isWriteThreadSet = false;

	return;
}

void Communication::Read()
{
	unsigned long bytesRead;
	BOOL isSuccess;
	unsigned long error;
	HANDLE hReadCompEvent = CreateEvent(NULL, false, false, NULL);
	OVERLAPPED sync = {0};
	sync.hEvent = hReadCompEvent;
	while(_isReadingContinued)//remake; wait for some inner symbol of the class; set that symbol on close and join the thread;
	{
		isSuccess = ReadFile(hPort, ReadBuf, ReadBufSize, NULL, &sync);
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
			//ReadBuf[bytesRead] = '\0';
			//printf("%s", ReadBuf);
			//size_t charsConverted = 0;
			//wchar_t* convertedArr = new wchar_t(bytesRead + 1);
			//mbstowcs_s(&charsConverted, convertedArr, bytesRead + 1, ReadBuf, _TRUNCATE);
			BytesPerSecond += bytesRead;
			for (unsigned int i = 0; i < BRHandlersNumber; i++)
			{
				(BytesReceivedHandlers[i])(ReadBuf, bytesRead);
			}
		}
	}
	//to transmit a message to the window to close the handle of the thread
	return;
}

void Communication::CountTime()
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
	//		for (unsigned int i = 0; i < BCHandlersNumber; i++)
	//		{
	//			(BaudrateChangedHandlers[i])(_baudrate);
	//		}
	//	}
	}
}