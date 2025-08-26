#include "RingBuffer.h"
#include "string.h"
#include "stdint.h"

// ��С��ת������������С��ϵͳת��ˣ�
static uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

static uint32_t ntohl(uint32_t netlong) {
    // ��htonl��ͬ����Ϊ�ǶԳƲ���
    return htonl(netlong);
}

void ByteRingBuf_Reset(sByteRingBuffer* pBuf) {
    if (pBuf == NULL) return;
    pBuf->iFront = 0;
    pBuf->iRear = 0;
    pBuf->iVirtualFront = 0;
    pBuf->iVirtualRear = 0;
}

bool ByteRingBuf_IsEmpty(sByteRingBuffer* pBuf) {
    if (pBuf == NULL) return true;
    return (pBuf->iFront == pBuf->iRear);
}

bool ByteRingBuf_IsFull(sByteRingBuffer* pBuf) {
    if (pBuf == NULL || pBuf->ByteCapacity == 0) return false;
    return ((pBuf->iRear + 1) % pBuf->ByteCapacity) == pBuf->iFront;
}

int ByteRingBuf_ActualSize(sByteRingBuffer* pBuf) {
    if (pBuf == NULL || pBuf->ByteCapacity == 0) return 0;
    return (pBuf->iRear - pBuf->iFront + pBuf->ByteCapacity) % pBuf->ByteCapacity;
}

int ByteRingBuf_FreeSize(sByteRingBuffer* pBuf) {
    if (pBuf == NULL || pBuf->ByteCapacity == 0) return 0;
    // Ԥ��һ���ֽ���Ϊ��״̬�жϣ�����ʵ�ʿ��ÿռ���������1�ټ�ȥ���ÿռ�
    return (pBuf->ByteCapacity - 1 - ByteRingBuf_ActualSize(pBuf));
}

int ByteRingBuf_TryWrite(sByteRingBuffer* pBuf, byte* pSrcData, int iSrcDataSize) {
    if (pBuf == NULL || pSrcData == NULL || iSrcDataSize <= 0 || 
        pBuf->pByteBuf == NULL || pBuf->ByteCapacity == 0) {
        return 0;
    }

    // �ӵ�ǰʵ��λ�ÿ�ʼ����д��
    pBuf->iVirtualRear = pBuf->iRear;
    int freeSize = ByteRingBuf_FreeSize(pBuf);
    int writeSize = (iSrcDataSize < freeSize) ? iSrcDataSize : freeSize;
    
    if (writeSize <= 0) return 0;

    int end = pBuf->ByteCapacity - pBuf->iVirtualRear;
    if (end >= writeSize) {
        // ���ο���
        memcpy(pBuf->pByteBuf + pBuf->iVirtualRear, pSrcData, writeSize);
        pBuf->iVirtualRear += writeSize;
    } else {
        // �����ο���
        memcpy(pBuf->pByteBuf + pBuf->iVirtualRear, pSrcData, end);
        memcpy(pBuf->pByteBuf, pSrcData + end, writeSize - end);
        pBuf->iVirtualRear = writeSize - end;
    }

    return writeSize;
}

void ByteRingBuf_FlashTryWrite(sByteRingBuffer* pBuf) {
    if (pBuf == NULL) return;
    pBuf->iRear = pBuf->iVirtualRear;
}

void ByteRingBuf_RestoreTryRead(sByteRingBuffer* pBuf) {
    if (pBuf == NULL) return;
    pBuf->iVirtualFront = pBuf->iFront;
}

int ByteRingBuf_Write(sByteRingBuffer* pBuf, byte* pSrcData, int iSrcDataSize) {
    int writeSize = ByteRingBuf_TryWrite(pBuf, pSrcData, iSrcDataSize);
    ByteRingBuf_FlashTryWrite(pBuf);
    return writeSize;
}

int ByteRingBuf_TryRead(sByteRingBuffer* pBuf, byte* pDstData, int iDataSizeToRead) {
    if (pBuf == NULL || pDstData == NULL || iDataSizeToRead <= 0 || 
        pBuf->pByteBuf == NULL || pBuf->ByteCapacity == 0) {
        return 0;
    }

    // �ӵ�ǰʵ��λ�ÿ�ʼ���Զ�ȡ
    pBuf->iVirtualFront = pBuf->iFront;
    int actualSize = ByteRingBuf_ActualSize(pBuf);
    int readSize = (iDataSizeToRead < actualSize) ? iDataSizeToRead : actualSize;
    
    if (readSize <= 0) return 0;

    int end = pBuf->ByteCapacity - pBuf->iVirtualFront;
    if (end >= readSize) {
        // ���ο���
        memcpy(pDstData, pBuf->pByteBuf + pBuf->iVirtualFront, readSize);
        pBuf->iVirtualFront += readSize;
    } else {
        // �����ο���
        memcpy(pDstData, pBuf->pByteBuf + pBuf->iVirtualFront, end);
        memcpy(pDstData + end, pBuf->pByteBuf, readSize - end);
        pBuf->iVirtualFront = readSize - end;
    }

    return readSize;
}

void ByteRingBuf_FlashTryRead(sByteRingBuffer* pBuf) {
    if (pBuf == NULL) return;
    pBuf->iFront = pBuf->iVirtualFront;
}

int ByteRingBuf_Read(sByteRingBuffer* pBuf, byte* pDstData, int iDataSizeToRead) {
    int readSize = ByteRingBuf_TryRead(pBuf, pDstData, iDataSizeToRead);
    ByteRingBuf_FlashTryRead(pBuf);
    return readSize;
}

int ByteRingBuf_WriteLine(sByteRingBuffer* pBuf, char* str) {
    if (pBuf == NULL || str == NULL) return 0;
    
    int strLen = strlen(str);
    int totalWrite = 0;
    
    // д���ַ�������
    if (strLen > 0) {
        totalWrite = ByteRingBuf_Write(pBuf, (byte*)str, strLen);
        // ����ַ���ûд�꣬������д�볤��
        if (totalWrite < strLen) {
            return totalWrite;
        }
    }
    
    // д�뻻�з�
    byte newline = '\n';
    int nlWrite = ByteRingBuf_Write(pBuf, &newline, 1);
    return totalWrite + nlWrite;
}

int ByteRingBuf_ReadLine(sByteRingBuffer* pBuf, char* str, int strCapacity) {
    if (pBuf == NULL || str == NULL || strCapacity <= 0) return 0;
    
    int maxRead = strCapacity - 1; // Ԥ����ֹ��λ��
    int readCount = 0;
    byte currentByte;
    
    while (readCount < maxRead) {
        // ����Ƿ�������
        if (ByteRingBuf_ActualSize(pBuf) == 0) break;
        
        // ��ȡһ���ֽ�
        int read = ByteRingBuf_Read(pBuf, &currentByte, 1);
        if (read != 1) break;
        
        // �������з�ֹͣ
        if (currentByte == '\n') break;
        
        str[readCount++] = (char)currentByte;
    }
    
    // ����ַ�����ֹ��
    str[readCount] = '\0';
    return readCount;
}

int ByteRingBuf_WriteFrame(sByteRingBuffer* pBuf, void* pSrcByte, int iSrcContentByteSize) {
    if (pBuf == NULL || pSrcByte == NULL || iSrcContentByteSize < 0) return 0;
    
    // ����֡�ܴ�С��4�ֽ�ͷ�� + ���ݴ�С
    int frameSize = 4 + iSrcContentByteSize;
    if (ByteRingBuf_FreeSize(pBuf) < frameSize) {
        return 0; // �ռ䲻��
    }
    
    // д��ͷ������˸�ʽ��
    uint32_t len = htonl((uint32_t)iSrcContentByteSize);
    int headWrite = ByteRingBuf_Write(pBuf, (byte*)&len, 4);
    if (headWrite != 4) return 0;
    
    // д������
    int contentWrite = ByteRingBuf_Write(pBuf, (byte*)pSrcByte, iSrcContentByteSize);
    return (contentWrite == iSrcContentByteSize) ? frameSize : (4 + contentWrite);
}

int ByteRingBuf_ReadFrame(sByteRingBuffer* pBuf, void* pDest, int iDestCapacity) {
    if (pBuf == NULL || pDest == NULL || iDestCapacity <= 0) return 0;
    
    // �ȳ��Զ�ȡͷ��
    uint32_t lenBigEndian;
    int tryHead = ByteRingBuf_TryRead(pBuf, (byte*)&lenBigEndian, 4);
    if (tryHead != 4) {
        ByteRingBuf_RestoreTryRead(pBuf);
        return 0; // ͷ��������
    }
    
    // �������ݳ���
    uint32_t contentLen = ntohl(lenBigEndian);
    if (contentLen > (uint32_t)iDestCapacity) {
        ByteRingBuf_RestoreTryRead(pBuf);
        return 0; // Ŀ�껺��������
    }
    
    // ����������Ƿ��㹻
    if (ByteRingBuf_ActualSize(pBuf) < 4 + (int)contentLen) {
        ByteRingBuf_RestoreTryRead(pBuf);
        return 0; // ���ݲ�����
    }
    
    // ȷ�϶�ȡͷ��
    ByteRingBuf_FlashTryRead(pBuf);
    
    // ��ȡ����
    int contentRead = ByteRingBuf_Read(pBuf, (byte*)pDest, contentLen);
    return (contentRead == (int)contentLen) ? (4 + contentRead) : contentRead;
}
