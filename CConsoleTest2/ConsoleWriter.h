#ifndef CONSOLE_WRITER_H
#define CONSOLE_WRITER_H

#include "FileIO.h"
#include "SerialComm.h"
#include "stdio.h"

class ConsoleWriter : public FileIOSubscribable, public SerialCommSubscribable
{
public:
	ConsoleWriter() : FileIOSubscribable(), SerialCommSubscribable()
	{}

	~ConsoleWriter(){}

	void FileBytesReadHandler(char* line, unsigned long charNum, bool isLast)
	{
		printf("%s\r\n\r\n", line);
		printf("///////////////////////////////////////////////////////\r\n\r\n");
	}

	void FileBytesWrittenHandler()
	{
	}

	void SerialBytesReceivedHandler(char* bytes, unsigned int bytesNum)
	{
		printf("Received: %s\r\n", bytes);
	}

	void SerialBaudrateChangedHandler(unsigned long baudrate)
	{
	}

	void SerialNothingReceivedHandler()
	{
	}
};

#endif