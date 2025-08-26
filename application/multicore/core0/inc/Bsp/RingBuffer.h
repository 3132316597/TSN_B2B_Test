#ifndef __RingBuffer_H
#define __RingBuffer_H

#include "stdio.h"
#include "base.h"

/// <summary>
/// 注意：
/// 1. 使用前须对sByteRingBuffer的pByteBuf对象赋初值
/// 2. iFront, iRear为正式标志位，iVirtualFront、iVirtualRear在TryWrite/ TryRead中使用
/// 3. 数据的完整性依赖iFront, iRear标志位，不可对一段数据连续分多次使用Write或Read。
/// 4. 不支持多线程读或者写，只能一读一写。
/// </summary>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32	ByteCapacity;
	byte*	pByteBuf;  
	int		iFront;
	int		iRear;
	int		iVirtualFront;
	int		iVirtualRear;
}sByteRingBuffer;


void ByteRingBuf_Reset(sByteRingBuffer* pBuf);
bool ByteRingBuf_IsEmpty(sByteRingBuffer* pBuf);
bool ByteRingBuf_IsFull(sByteRingBuffer* pBuf);

int ByteRingBuf_ActualSize(sByteRingBuffer* pBuf);
int ByteRingBuf_FreeSize(sByteRingBuffer* pBuf);

//try write, but will not change front flag
int ByteRingBuf_TryWrite(sByteRingBuffer* pBuf, byte* pSrcData, int iSrcDataSize);
void ByteRingBuf_FlashTryWrite(sByteRingBuffer* pBuf);
void ByteRingBuf_RestoreTryRead(sByteRingBuffer* pBuf);
int ByteRingBuf_Write(sByteRingBuffer* pBuf, byte* pSrcData, int iSrcDataSize);

//try Read, but will not change rear flag
int ByteRingBuf_TryRead(sByteRingBuffer* pBuf, byte* pDstData, int iDataSizeToRead);
void ByteRingBuf_FlashTryRead(sByteRingBuffer* pBuf);
void ByteRingBuf_RestoreTryRead(sByteRingBuffer* pBuf);
int ByteRingBuf_Read(sByteRingBuffer* pBuf, byte* pDstData, int iDataSizeToRead);

int ByteRingBuf_WriteLine(sByteRingBuffer* pBuf, char* str);
int ByteRingBuf_ReadLine(sByteRingBuffer* pBuf, char* str, int strCapacity);

//frame type: 4byte head + nbyte body. head is the length of body
int ByteRingBuf_WriteFrame(sByteRingBuffer* pBuf, void* pSrcByte, int iSrcContentByteSize);
int ByteRingBuf_ReadFrame(sByteRingBuffer* pBuf, void* pDest, int iDestCapacity);


#ifdef __cplusplus
}
#endif
#endif
