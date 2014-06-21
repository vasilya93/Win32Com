#include "Log.h"
#include "FileIO.h"
#include "string.h"

Log::Log()
{
	FileWriter = new FileIO();
	_log = new char[LOG_SIZE];
	_counter = 0;
	_left = LOG_SIZE;
}

Log::Log(FileIO* fileIO)
{
	FileWriter = fileIO;
	_log = new char[LOG_SIZE];
	_counter = 0;
	_left = LOG_SIZE;
}

Log::Log(const Log& log)
{
	FileWriter = log.FileWriter;
	_log = log._log;
	_counter = log._counter;
	_left = log._left;
}

Log::~Log()
{
	delete _log;
}

Log& Log::operator=(const Log& log)
{
	if (&log == this)
	{
		return *this;
	}
	_log = log._log;
	_counter = log._counter;
	_left = log._left;
	FileWriter = log.FileWriter; //need to copy address, not instance; so looks wrong
	return *this;
}

bool Log::AddMessage(char* message)
{
	unsigned long mesLen = strlen(message);
	_logMutex.lock();
	if(mesLen + 1 < _left)
	{
		strcpy_s(&(_log[_counter]),_left, message);
		_counter += mesLen;
		_left -= mesLen;
		_logMutex.unlock();	
		return true;
	}
	else if (_left != 0)
	{
		mesLen = strlen("\r\nLog overflow");
		strcpy_s(&(_log[LOG_SIZE - mesLen - 1]), mesLen + 1, "\r\nLog overflow");
		_counter = LOG_SIZE;
		_left = 0;
		_logMutex.unlock();	
		return false;
	}
	else
	{
		_logMutex.unlock();	
		return false;
	}
}

void Log::Clear()
{
	_logMutex.lock();
	_left = LOG_SIZE;
	_counter = 0;
	_logMutex.unlock();
}

void Log::SaveToFile()
{
	_logMutex.lock();
	unsigned long bufSize = _counter;
	char* buffer = new char[bufSize];
	memcpy_s(buffer, bufSize, _log, bufSize);
	_logMutex.unlock();

	FileWriter->SetWriteFile(L"log.txt");
	FileWriter->WriteBytes(buffer, bufSize, false, true);
}

