// CConsoleTest2.cpp : Defines the entry point for the console application.
//
#include "tchar.h"
#include "stdio.h"
#include "string.h"
#include "windows.h"
#include "thread"
#include "SerialComm.h"
#include "FileIO.h"
#include "ConsoleWriter.h"
#include "CommManager.h"
#include "assert.h"

#define MAIN_ANSWER_SIZE 50

int _tmain(int argc, _TCHAR* argv[])
{
	FileIOSubscribable* consoleWriter = new ConsoleWriter;
	SerialCommSubscribable* consoleWriterSer = new ConsoleWriter;

	SerialComm comm;
	comm.AttachHandlerHost(consoleWriterSer);

	FileIO fileIO;
	fileIO.AttachBRHandler(consoleWriter);
	fileIO.SetReadFile(L"C:\\Users\\vasil_000\\Desktop\\financier.txt");

	CommManager manag;


	char answer[MAIN_ANSWER_SIZE];
	wchar_t port[MAIN_ANSWER_SIZE];
	bool result = true;

	while(result)
	{
		scanf_s("%49s", answer, MAIN_ANSWER_SIZE); //negated scanset option of scanf
		if(!strcmp("exit", answer))
		{
			result = false;
		}
		//actions for individual com-port class
		else if(!strcmp("connectser", answer))
		{
			scanf_s("%49s", answer, MAIN_ANSWER_SIZE);
			size_t convNum;
			mbstowcs_s(&convNum, port, MAIN_ANSWER_SIZE, answer, _TRUNCATE);
			unsigned int error;
			comm.Connect(port, 3000000, error);
		}
		else if(!strcmp("send", answer))
		{
			scanf_s("%49s", answer, MAIN_ANSWER_SIZE);
			bool isLineBusy = true;
			comm.Write(answer, strlen(answer), &isLineBusy);
		}

		//actions for com-manger class
		else if(!strcmp("connect", answer))
		{
			scanf_s("%49s", answer, MAIN_ANSWER_SIZE);
			size_t convNum;
			mbstowcs_s(&convNum, port, MAIN_ANSWER_SIZE, answer, _TRUNCATE);
			unsigned int error;
			manag.Connect(port, 3000000, error);
		}
		else if(!strcmp("file", answer))
		{
			manag.TransmitFile(L"financier.txt");
		}
		else if(!strcmp("save", answer))
		{
			unsigned long bytesNum, firstTick, lastTick;
			manag.GetTransactParams(bytesNum, firstTick, lastTick);
			unsigned long interval = lastTick - firstTick;
			printf("Bytes received: %u\r\n", bytesNum);
			printf("Time interval: %u\r\n", interval);
			if(interval!=0)
			{
				printf("Approx. baudrate: %u\r\n", (bytesNum*9*1000)/(interval));
			}
			manag.WriteFile(L"received.txt");
		}
	}

	delete consoleWriter;
	delete consoleWriterSer;
	return 0;
}