#ifndef PACKET_MAKER_H
#define PACKET_MAKER_H

#define PM_POS_DATA 0
#define PM_DATA_SIZE 500

#define PM_POS_SIGNS PM_DATA_SIZE 
#define PM_SIGNS_SIZE 1 //may be set before pushing data bytes
#define PM_SIGN_FIRST 1
#define PM_SIGN_LAST 2

#define PM_POS_DLEN (PM_POS_SIGNS + PM_SIGNS_SIZE)
#define PM_DLEN_SIZE sizeof(unsigned long)

#define PM_POS_CHECK (PM_POS_DLEN + PM_DLEN_SIZE)
#define PM_CHECK_SIZE 3
#define PM_CHECK_POS1 (PM_POS_DATA + 50)
#define PM_CHECK_POS2 (PM_POS_DATA + 100)
#define PM_CHECK_POS3 (PM_POS_DATA + 300)

#define PM_PACKET_SIZE (PM_DATA_SIZE + PM_SIGNS_SIZE + PM_DLEN_SIZE + PM_CHECK_SIZE)

#include "string.h"
#include "Log.h"

//packet form:
//1 byte of signs (first packet, data packet, last packet)
//sizeof(unsigned int) bytes which form the number of meaning data bytes in the packet
//3 bytes to check corruption of the packet
//PM_DATA_SIZE bytes of data

//message is the sum of packets
union Converter
{
	unsigned long Num;
	char Array[sizeof(unsigned long)];
};

class PacketMaker
{
	char* _recBuf[2];
	unsigned char _currentRecBuf;
	bool _isRecReserveFree;
	unsigned int _recInPos;
	unsigned int _recBufLeft;

	bool _isRecPackFirst;


	char* _sentBuf[2];
	volatile bool _isSentBufUsed[2]; //устанавливается, когда запись в буффер завершена; снимается, когда чтение из буффера завершено
							//в буффер нельзя писать, если для него установлен этот флаг; буффер нельзя получить, если для него не установлен это флаг
	
	unsigned char _writtenSentBuf; //тот, в который идет запись
	unsigned char _sentSentBuf; //тот, который будет возвращен при следующем вызове GetSentPacket
	unsigned int _sentInPos;
	unsigned int _sentBufLeft;


	Converter _converter;

	//Functions
	unsigned char _reserveBuf(unsigned char);
	unsigned char _switchCurrentBuf(unsigned char*);
	void _finishSentPacket(bool isLast);

public:
	//linked objects
	Log* CommLog;

	//Constructors and destructors
	PacketMaker();
	~PacketMaker();

	//Access funcs
	void BeginReceivedMessage();

	void BeginSentMessage();
	void EndSentMessage();

	bool IsReceivedDataReady();

	char* GetSentPacket(volatile bool** isPacketUsed);
	bool IsSentPackUsed(){return _isSentBufUsed[_sentSentBuf];}

	//Funcs

	bool IsRecPacketConsistent();

	bool PushReceivedBytes(char*, unsigned int);
	void PushSentData(char*, unsigned int, bool);

	unsigned long GetReceivedData(unsigned long, char*);
};

#endif