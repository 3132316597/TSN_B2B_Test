#ifndef __RingBuffer_H
#define __RingBuffer_H

#include "stdio.h"
#include "base.h"

/// <summary>
/// ע�⣺
/// 1. ʹ��ǰ���sByteRingBuffer��pByteBuf���󸳳�ֵ
/// 2. iFront, iRearΪ��ʽ��־λ��iVirtualFront��iVirtualRear��TryWrite/ TryRead��ʹ��
/// 3. ���ݵ�����������iFront, iRear��־λ�����ɶ�һ�����������ֶ��ʹ��Write��Read��
/// 4. ��֧�ֶ��̶߳�����д��ֻ��һ��һд��
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
