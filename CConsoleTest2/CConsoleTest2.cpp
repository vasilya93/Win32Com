// CConsoleTest2.cpp : Defines the entry point for the console application.
//
#include "tchar.h"
#include "stdio.h"
#include "string.h"
#include "windows.h"
#include "thread"
#include "Communication.h"

#define BUFFER_SIZE 50

void MyHandler(char*, unsigned int);

void MyBaudHandler(unsigned long baudrate);

int _tmain(int argc, _TCHAR* argv[])
{
	Communication comm;
	comm.AttachBRHandler(&MyHandler);
	comm.AttachBCHandler(&MyBaudHandler);
	unsigned int error;
	if(!comm.Connect(L"COM4", 115200, error))
	{
		printf("%s", "Connection failed!\r\n");
	}

	char answer[BUFFER_SIZE];
	bool result = true;

	while(result)
	{
		scanf_s("%49s", answer, BUFFER_SIZE); //negated scanset option of scanf
		if(!strcmp("exit", answer))
		{
			result = false;
		}
		else
		{
			if(!comm.Write(answer, strlen(answer)))
				printf("write failed\r\n");
		}
	}

	return 0;
}

void MyHandler(char* line, unsigned int symbolsNumber)
{
	line[symbolsNumber] = '\0';
	printf("Received: %s\r\n", line);
}

void MyBaudHandler(unsigned long baudrate)
{
	//printf("%d\r\n", baudrate);
}