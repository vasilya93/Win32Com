#ifndef LOGGER_H
#define LOGGER_H

#define LOG_SIZE (1024 * 1024 * 5)

class Log
{
	char* _log;
	unsigned long _counter;
	unsigned long _left;

public:
	Log();
	~Log();
	
	bool AddMessage(char* message);

	void Clear();

	void SaveToFile();
}

#endif