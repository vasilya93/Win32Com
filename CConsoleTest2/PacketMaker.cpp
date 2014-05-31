#include "PacketMaker.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"

//---------------------------------------Constructors -------------------------------------

PacketMaker::PacketMaker()
{
	_recBuf[0] = new char[PM_PACKET_SIZE];
	_recBuf[1] = new char[PM_PACKET_SIZE];
	_currentRecBuf = 0;
	_isRecReserveFree = true;
	_recInPos = 0;
	_recBufLeft = PM_DATA_SIZE;

	_isRecPackFirst = true;

	_sentBuf[0] = new char[PM_PACKET_SIZE];
	_sentBuf[1] = new char[PM_PACKET_SIZE];
	_writtenSentBuf = 0;
	_sentSentBuf = 0;
	_isSentBufUsed[0] = false;
	_isSentBufUsed[1] = false;
	_sentInPos = 0;
	_sentBufLeft = PM_DATA_SIZE;
}

//---------------------------------------Destructors --------------------------------------

PacketMaker::~PacketMaker()
{
	delete _recBuf[0];
	delete _recBuf[1];

	delete _sentBuf[0];
	delete _sentBuf[1];
}

//---------------------------------------Access funcs --------------------------------------

void PacketMaker::BeginReceivedMessage() //set some signs in the class, which will allow to identify, that this packet should be decrypted
{										//as the first packet in message; will elaborate when signs of packet will be specified 
	_currentRecBuf = 0;
	_isRecReserveFree = true;
	_recInPos = 0;
	_recBufLeft = PM_DATA_SIZE;

	_isRecPackFirst = true;
}

void PacketMaker::BeginSentMessage()
{
	_writtenSentBuf = 0;
	_sentSentBuf = 0;
	_sentInPos = 0;
	_sentBufLeft = PM_DATA_SIZE;

	_isSentBufUsed[0] = false;
	_isSentBufUsed[1] = false;
}

void PacketMaker::EndSentMessage()
{
}

char* PacketMaker::GetSentPacket(bool** isPacketUsed)
{
	if(!_isSentBufUsed[_sentSentBuf])
	{
		return NULL;
	}
	else
	{
		*isPacketUsed = &(_isSentBufUsed[_sentSentBuf]);
		return _sentBuf[_switchCurrentBuf(&_sentSentBuf)];
	}
}

//-----------------------------------------Functions ----------------------------------------

void PacketMaker::PushSentData(char* data, unsigned int dataSize, bool isLastData)
{
	assert(dataSize <= 2*PM_DATA_SIZE);
	if(dataSize < _sentBufLeft) //����� �������� ����� ������ � ������, ���� �� ���������
	{
		memcpy_s(&(_sentBuf[_writtenSentBuf][PM_POS_DATA + _sentInPos]), _sentBufLeft, data, dataSize);
		_sentBufLeft -= dataSize;
		_sentInPos += dataSize;
		if(isLastData)
		{
			_finishSentPacket(true);
		}
	}
	else if(dataSize == _sentBufLeft)
	{
		memcpy_s(&(_sentBuf[_writtenSentBuf][PM_POS_DATA + _sentInPos]), _sentBufLeft, data, dataSize);
		_sentBufLeft = 0;
		_sentInPos += dataSize;
		_finishSentPacket(isLastData);
	}
	else
	{
		memcpy_s(&(_sentBuf[_writtenSentBuf][PM_POS_DATA + _sentInPos]), _sentBufLeft, data, _sentBufLeft);
		unsigned long bytesWritten = _sentBufLeft;
		_sentBufLeft = 0;
		_sentInPos += dataSize;
		_finishSentPacket(false);

		memcpy_s(&(_sentBuf[_writtenSentBuf][PM_POS_DATA + _sentInPos]), _sentBufLeft, &(data[bytesWritten]), dataSize - bytesWritten);
		_sentBufLeft -= (dataSize - bytesWritten);
		_sentInPos += (dataSize - bytesWritten);
		if(isLastData)
		{
			_finishSentPacket(true);
		}
	}
}


//---------------------------------------Threads --------------------------------------


//------------------------------------Private funcs -----------------------------------

unsigned char PacketMaker::_switchCurrentBuf(unsigned char* currentBuf)
{
	unsigned char storage = *currentBuf;
	switch(storage)
	{
	case 0:
		*currentBuf = 1;
		break;
	case 1:
		*currentBuf = 0;
		break;
	}
	return storage;
}

void PacketMaker::_finishSentPacket(bool isLastData)
{
	if(isLastData)
	{
		_sentBuf[_writtenSentBuf][PM_POS_SIGNS] |= PM_SIGN_LAST;
	}

	_sentBuf[_writtenSentBuf][PM_POS_CHECK] = _sentBuf[_writtenSentBuf][PM_CHECK_POS1];
	_sentBuf[_writtenSentBuf][PM_POS_CHECK + 1] = _sentBuf[_writtenSentBuf][PM_CHECK_POS2];
	_sentBuf[_writtenSentBuf][PM_POS_CHECK + 2] = _sentBuf[_writtenSentBuf][PM_CHECK_POS3];

	_converter.Num = _sentInPos;
	for (int i = 0; i < PM_DLEN_SIZE; i++)
	{
		_sentBuf[_writtenSentBuf][PM_POS_DLEN + i] = _converter.Array[i];
	}

	_isSentBufUsed[_writtenSentBuf] = true;
	_switchCurrentBuf(&_writtenSentBuf);
	_sentInPos = 0;
	_sentBufLeft = PM_DATA_SIZE;
	while(_isSentBufUsed[_writtenSentBuf]);
}