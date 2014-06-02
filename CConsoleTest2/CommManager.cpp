#include "CommManager.h"

//--------------------------------Constructors -------------------------------
CommManager::CommManager() : FileIOSubscribable(), SerialCommSubscribable()
{
	_fileIO.AttachBRHandler(this);
	_fileIO.SetWriteFile(L"C:\\Users\\vasil_000\\Desktop\\received.txt");
	_serialComm.AttachHandlerHost(this);
	_sentFileBuf = new char[0];
	_sentBufCounter = 0;
	_bytesLeftToSend = 0;

	_recFileBuf = new char[MANAG_SIZE_RECBUF];
	_isRecBufEmpty = true;
	_recBufCounter = 0;
	_recBufHandledCounter = 0;
	_recBufLeft = MANAG_SIZE_RECBUF;

	_writeFileBuf = new char[MANAG_SIZE_RECBUF];
	_writeBufCounter = 0;
	_writeBufLeft = MANAG_SIZE_RECBUF;

	_doWriteToSerial = false;
	_doWriteToFile = false;

	_isMainThreadRunning = true;
	_mainThread = std::thread(&CommManager::_mainThreadFunc, std::ref(*this));
}

//--------------------------------Destructors -------------------------------

CommManager::~CommManager()
{
	_isMainThreadRunning = false;
	_mainThread.join();
	delete _sentFileBuf;
	delete _recFileBuf;
}

//--------------------------------Public methods -------------------------------

void CommManager::TransmitFile(wchar_t* fileName)
{
	_fileIO.SetReadFile(fileName);
	_fileIO.ReadFileBytes(FILE_READ_FULL);
}

void CommManager::WriteFile(wchar_t* fileName)
{
	_fileIO.SetWriteFile(fileName);
	_doWriteToFile = true;
}

//----------------------------------Handlers ---------------------------------

void CommManager::SerialBytesReceivedHandler(char* bytes, unsigned int bytesNum)
{
	memcpy_s(&(_recFileBuf[_recBufCounter]), _recBufLeft, bytes, bytesNum); //парсинг делать сразу по приему, но при этом не загромождать принимающий поток
	_recBufCounter += bytesNum;
	_recBufLeft -= bytesNum;
	_isRecBufEmpty = false;
}

void CommManager::SerialBaudrateChangedHandler(unsigned long newBaud)
{
}

void CommManager::SerialNothingReceivedHandler()
{
	/*if(!_isRecBufEmpty)
	{
	_fileIO.WriteBytes(_recFileBuf, _recBufCounter, false);
	_isRecBufEmpty = true;
	_recBufCounter = 0;
	_recBufLeft = MANAG_SIZE_RECBUF;
	}*/
}

void CommManager::FileBytesReadHandler(char* bytes, unsigned long bytesNum, bool isLast)
{
	_bytesLeftToSend = bytesNum;
	_sentBufCounter = 0;
	delete _sentFileBuf;
	_sentFileBuf = new char[_bytesLeftToSend];
	memcpy_s(_sentFileBuf, _bytesLeftToSend, bytes, _bytesLeftToSend);
	_doWriteToSerial = true;
}

void CommManager::FileBytesWrittenHandler()
{
	_isRecBufEmpty = true;
	_recBufCounter = 0;
	_recBufLeft = MANAG_SIZE_RECBUF;
}

//--------------------------------Private methods -------------------------------


//--------------------------------Threads -------------------------------

void CommManager::_mainThreadFunc()
{
	while(_isMainThreadRunning)
	{
		if(_doWriteToFile)
		{
			_doWriteToFile = false;
			_writeRecBufToFile();
		}
		if(_doWriteToSerial)
		{
			_doWriteToSerial = false;
			_writeSentBufToSerial();
		}
	}
}

//-------------------------Thread auxiliary funcs ------------------------

void CommManager::_writeSentBufToSerial()
{
	_packetMaker.BeginSentMessage();
	while(_bytesLeftToSend > 0)
	{
		if(_bytesLeftToSend > PM_DATA_SIZE)
		{
			_packetMaker.PushSentData(&(_sentFileBuf[_sentBufCounter]), PM_DATA_SIZE, false);
			_sentBufCounter += PM_DATA_SIZE;
			_bytesLeftToSend -= PM_DATA_SIZE;
			if(_packetMaker.IsSentPackUsed())
			{
				bool *isSentPackUsed = NULL;
				char* writtenLine = _packetMaker.GetSentPacket(&isSentPackUsed);
				_serialComm.Write(writtenLine, PM_PACKET_SIZE, isSentPackUsed); // wait for the end of transmission
			}																	// все равно основной поток будет ждать
		}
		else	
		{	
			_packetMaker.PushSentData(&(_sentFileBuf[_sentBufCounter]), _bytesLeftToSend, true);
			_sentBufCounter = 0;
			_bytesLeftToSend = 0;
			if(_packetMaker.IsSentPackUsed())
			{
				bool *isSentPackUsed = NULL;
				char* writtenLine = _packetMaker.GetSentPacket(&isSentPackUsed);
				_serialComm.Write(writtenLine, PM_PACKET_SIZE, isSentPackUsed); // wait for the end of transmission
			}
			if(_packetMaker.IsSentPackUsed())
			{
				bool *isSentPackUsed = NULL;
				char* writtenLine = _packetMaker.GetSentPacket(&isSentPackUsed);
				_serialComm.Write(writtenLine, PM_PACKET_SIZE, isSentPackUsed); // wait for the end of transmission, хорошо ли это? наверное нет

			}
		}
	}
}

void CommManager::_writeRecBufToFile()
{
	_corruptedPacketsNum = 0;
	_packetMaker.BeginReceivedMessage();
	while(_recBufCounter > _recBufHandledCounter)
	{
		if((_recBufCounter - _recBufHandledCounter) < PM_PACKET_SIZE)
		{
			_corruptedPacketsNum++;
			break;
		}
		_packetMaker.PushReceivedBytes(&(_recFileBuf[_recBufHandledCounter]), PM_PACKET_SIZE);
		_recBufHandledCounter += PM_PACKET_SIZE;
		if(_packetMaker.IsReceivedDataReady() && _packetMaker.IsRecPacketConsistent()) //нужно, чтобы первая проверка выполнялась первее, чем вторая))
		{
			unsigned long dataSize = _packetMaker.GetReceivedData(_writeBufLeft, &(_writeFileBuf[_writeBufCounter]));
			_writeBufCounter += dataSize;
			_writeBufLeft -= dataSize;
		}
	}
	_fileIO.WriteBytes(_writeFileBuf, _writeBufCounter, false); // need to make sure that the buffer is not rewriten, paste buffer
	_recBufHandledCounter = 0;
	_writeBufCounter = 0;
	_writeBufLeft = MANAG_SIZE_RECBUF;
}