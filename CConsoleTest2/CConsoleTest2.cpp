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
		else if(!strcmp("connect", answer))
		{
			scanf_s("%49s", answer, MAIN_ANSWER_SIZE);
			size_t convNum;
			mbstowcs_s(&convNum, port, MAIN_ANSWER_SIZE, answer, _TRUNCATE);
			unsigned int error;
			manag.Connect(port, 115200, error);
		}
		else if(!strcmp("connectser", answer))
		{
			scanf_s("%49s", answer, MAIN_ANSWER_SIZE);
			size_t convNum;
			mbstowcs_s(&convNum, port, MAIN_ANSWER_SIZE, answer, _TRUNCATE);
			unsigned int error;
			comm.Connect(port, 115200, error);
		}
		else if(!strcmp("send", answer))
		{
			scanf_s("%49s", answer, MAIN_ANSWER_SIZE);
			bool isLineBusy = true;
			comm.Write(answer, strlen(answer), &isLineBusy);
		}
		else if(!strcmp("file", answer))
		{
			manag.TransmitFile(L"C:\\Users\\vasil_000\\Desktop\\chapter1.txt");
		}
		else if(!strcmp("rewrite", answer))
		{
			manag.RewriteFile(L"C:\\Users\\vasil_000\\Desktop\\DSC05756.rar"); //DSC05756.jpg
		}
	}

	delete consoleWriter;
	delete consoleWriterSer;
	return 0;
}