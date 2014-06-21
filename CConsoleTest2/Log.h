#ifndef LOGGER_H
#define LOGGER_H

#define LOG_SIZE (1024 * 1024 * 5)

#include "FileIO.h"
#include "mutex"

class Log
{
	char* _log;
	unsigned long _counter;
	unsigned long _left;

	std::mutex _logMutex;

public:
//Linked objects
	FileIO* FileWriter;

//-------------------------------Constructors -------------------------------
	Log();
	Log(const Log& log);
	Log(FileIO* fileIO);
	~Log();

//-------------------------------Access funcs -------------------------------

	char* GetLog(){return _log;}
	unsigned long GetCounter(){return _counter;}
	unsigned long GetLeft(){return _left;}

//-------------------------------Operators -------------------------------

	Log& operator=(const Log& log);

//-------------------------------Functions -------------------------------
	
	bool AddMessage(char* message);
	void Clear();
	void SaveToFile();
};

#endif