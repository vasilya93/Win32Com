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
	_recBufLeft = MANAG_SIZE_RECBUF;
}

//--------------------------------Destructors -------------------------------

CommManager::~CommManager()
{
	delete _sentFileBuf;
	delete _recFileBuf;
}

//--------------------------------Public methods -------------------------------

void CommManager::RewriteFile(wchar_t* fileName)
{
	/*_fileIO.SetReadFile(fileName);
	_isLastPacket = false;
	_areFileBytesWritten = true;
	do
	{
	_areFileBytesRead = false;
	_fileIO.ReadFileBytes(500);
	while(!_areFileBytesRead);
	_areFileBytesWritten = false;
	_fileIO.WriteBytes(*_currentSentPacket, _sentPacketSize, true);
	while(!_areFileBytesWritten);
	}	while (!_isLastPacket);*/
}

void CommManager::TransmitFile(wchar_t* fileName)
{
	_fileIO.SetReadFile(fileName);
	_fileIO.ReadFileBytes(FILE_READ_FULL);
}

//----------------------------------Handlers ---------------------------------

void CommManager::SerialBytesReceivedHandler(char* bytes, unsigned int bytesNum)
{
	//_fileIO.WriteBytes(bytes, bytesNum, true);
	//_packetMaker.PushReceivedBytes(bytes, bytesNum);
	//if(_packetMaker.IsReceivedDataReady())
	//{
	//	
	//}
	memcpy_s(&(_recFileBuf[_recBufCounter]), _recBufLeft, bytes, bytesNum);
	_recBufCounter += bytesNum;
	_recBufLeft -= bytesNum;
	_isRecBufEmpty = false;
}

void CommManager::SerialBaudrateChangedHandler(unsigned long newBaud)
{
}

void CommManager::SerialNothingReceivedHandler()
{
	if(!_isRecBufEmpty)
	{
		_fileIO.WriteBytes(_recFileBuf, _recBufCounter, false);
		_isRecBufEmpty = true;
		_recBufCounter = 0;
		_recBufLeft = MANAG_SIZE_RECBUF;
	}
}

void CommManager::FileBytesReadHandler(char* bytes, unsigned long bytesNum, bool isLast)
{
	_bytesLeftToSend = bytesNum;
	_sentBufCounter = 0;
	_sentFileBuf = new char[_bytesLeftToSend];
	memcpy_s(_sentFileBuf, _bytesLeftToSend, bytes, _bytesLeftToSend);
	std::thread writeThread(&CommManager::_writeThread, std::ref(*this));
	writeThread.detach();
}

void CommManager::FileBytesWrittenHandler()
{
	//_areFileBytesWritten = true;
}

//--------------------------------Private methods -------------------------------


//--------------------------------Threads -------------------------------

void CommManager::_writeThread()
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
				_serialComm.Write(writtenLine, PM_PACKET_SIZE, isSentPackUsed);
			}
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
				_serialComm.Write(writtenLine, PM_PACKET_SIZE, isSentPackUsed);
			}
			if(_packetMaker.IsSentPackUsed())
			{
				bool *isSentPackUsed = NULL;
				char* writtenLine = _packetMaker.GetSentPacket(&isSentPackUsed);
				_serialComm.Write(writtenLine, PM_PACKET_SIZE, isSentPackUsed);

			}
		}
	}
}