#include "RingBuffer.h"
#include "string.h"
#include "stdint.h"

// 大小端转换函数（假设小端系统转大端）
static uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

static uint32_t ntohl(uint32_t netlong) {
    // 与htonl相同，因为是对称操作
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
    // 预留一个字节作为满状态判断，所以实际可用空间是容量减1再减去已用空间
    return (pBuf->ByteCapacity - 1 - ByteRingBuf_ActualSize(pBuf));
}

int ByteRingBuf_TryWrite(sByteRingBuffer* pBuf, byte* pSrcData, int iSrcDataSize) {
    if (pBuf == NULL || pSrcData == NULL || iSrcDataSize <= 0 || 
        pBuf->pByteBuf == NULL || pBuf->ByteCapacity == 0) {
        return 0;
    }

    // 从当前实际位置开始尝试写入
    pBuf->iVirtualRear = pBuf->iRear;
    int freeSize = ByteRingBuf_FreeSize(pBuf);
    int writeSize = (iSrcDataSize < freeSize) ? iSrcDataSize : freeSize;
    
    if (writeSize <= 0) return 0;

    int end = pBuf->ByteCapacity - pBuf->iVirtualRear;
    if (end >= writeSize) {
        // 单次拷贝
        memcpy(pBuf->pByteBuf + pBuf->iVirtualRear, pSrcData, writeSize);
        pBuf->iVirtualRear += writeSize;
    } else {
        // 分两次拷贝
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

    // 从当前实际位置开始尝试读取
    pBuf->iVirtualFront = pBuf->iFront;
    int actualSize = ByteRingBuf_ActualSize(pBuf);
    int readSize = (iDataSizeToRead < actualSize) ? iDataSizeToRead : actualSize;
    
    if (readSize <= 0) return 0;

    int end = pBuf->ByteCapacity - pBuf->iVirtualFront;
    if (end >= readSize) {
        // 单次拷贝
        memcpy(pDstData, pBuf->pByteBuf + pBuf->iVirtualFront, readSize);
        pBuf->iVirtualFront += readSize;
    } else {
        // 分两次拷贝
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
    
    // 写入字符串内容
    if (strLen > 0) {
        totalWrite = ByteRingBuf_Write(pBuf, (byte*)str, strLen);
        // 如果字符串没写完，返回已写入长度
        if (totalWrite < strLen) {
            return totalWrite;
        }
    }
    
    // 写入换行符
    byte newline = '\n';
    int nlWrite = ByteRingBuf_Write(pBuf, &newline, 1);
    return totalWrite + nlWrite;
}

int ByteRingBuf_ReadLine(sByteRingBuffer* pBuf, char* str, int strCapacity) {
    if (pBuf == NULL || str == NULL || strCapacity <= 0) return 0;
    
    int maxRead = strCapacity - 1; // 预留终止符位置
    int readCount = 0;
    byte currentByte;
    
    while (readCount < maxRead) {
        // 检查是否还有数据
        if (ByteRingBuf_ActualSize(pBuf) == 0) break;
        
        // 读取一个字节
        int read = ByteRingBuf_Read(pBuf, &currentByte, 1);
        if (read != 1) break;
        
        // 遇到换行符停止
        if (currentByte == '\n') break;
        
        str[readCount++] = (char)currentByte;
    }
    
    // 添加字符串终止符
    str[readCount] = '\0';
    return readCount;
}

int ByteRingBuf_WriteFrame(sByteRingBuffer* pBuf, void* pSrcByte, int iSrcContentByteSize) {
    if (pBuf == NULL || pSrcByte == NULL || iSrcContentByteSize < 0) return 0;
    
    // 计算帧总大小：4字节头部 + 内容大小
    int frameSize = 4 + iSrcContentByteSize;
    if (ByteRingBuf_FreeSize(pBuf) < frameSize) {
        return 0; // 空间不足
    }
    
    // 写入头部（大端格式）
    uint32_t len = htonl((uint32_t)iSrcContentByteSize);
    int headWrite = ByteRingBuf_Write(pBuf, (byte*)&len, 4);
    if (headWrite != 4) return 0;
    
    // 写入内容
    int contentWrite = ByteRingBuf_Write(pBuf, (byte*)pSrcByte, iSrcContentByteSize);
    return (contentWrite == iSrcContentByteSize) ? frameSize : (4 + contentWrite);
}

int ByteRingBuf_ReadFrame(sByteRingBuffer* pBuf, void* pDest, int iDestCapacity) {
    if (pBuf == NULL || pDest == NULL || iDestCapacity <= 0) return 0;
    
    // 先尝试读取头部
    uint32_t lenBigEndian;
    int tryHead = ByteRingBuf_TryRead(pBuf, (byte*)&lenBigEndian, 4);
    if (tryHead != 4) {
        ByteRingBuf_RestoreTryRead(pBuf);
        return 0; // 头部不完整
    }
    
    // 解析内容长度
    uint32_t contentLen = ntohl(lenBigEndian);
    if (contentLen > (uint32_t)iDestCapacity) {
        ByteRingBuf_RestoreTryRead(pBuf);
        return 0; // 目标缓冲区不足
    }
    
    // 检查总数据是否足够
    if (ByteRingBuf_ActualSize(pBuf) < 4 + (int)contentLen) {
        ByteRingBuf_RestoreTryRead(pBuf);
        return 0; // 数据不完整
    }
    
    // 确认读取头部
    ByteRingBuf_FlashTryRead(pBuf);
    
    // 读取内容
    int contentRead = ByteRingBuf_Read(pBuf, (byte*)pDest, contentLen);
    return (contentRead == (int)contentLen) ? (4 + contentRead) : contentRead;
}
