#ifndef BASE_H_
#define BASE_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MicroSecondPerSecond	1000000
#define ErrorCode			uint16_t
#define ErrorCheck(errCode)	(if(errCode) return errCode;)
#define arraySize(a)		(sizeof((a)) / sizeof((a[0])))
#define DeltaTimeIn_uS(t1,t2)	(((t2) - (t1)) / 1000)
#define uS2S(time)			((time)/MicroSecondPerSecond)
#define S2uS(time)			((time)*MicroSecondPerSecond)
#define mS2uS(time)			((time)*1000)
#define mS2nS(time)			((time)*1000000)
#define Max(x,y)            ((x)>=(y)?(x):(y))
#define Min(x,y)            ((x)>=(y)?(y):(x))
#define Abs(x)            	((x)>=0?(x):(-x))
#define IsInRange(x, min, max) ((max)>=(min)?((x)>=(min) && (x)<=(max)):((x)>=(max) && (x)<=(min)))
#define Saturation(x, lowerLimit, upperLimit)   ((upperLimit)>=(lowerLimit)?(Min((upperLimit),Max((x), (lowerLimit)))):(Min((lowerLimit),Max((x), (upperLimit)))))

/* 源列表 start */
#define  COM_TYPE_PC 		1
#define	 COM_TYPE_IO		2
#define	 COM_TYPE_FIELDBUS	3
#define  COM_TYPE_OP 		4
#define  COM_TYPE_SCANSER 	5
#define  COM_TYPE_WEBAPI 	6
#define  COM_TYPE_PYTHON 	7
#define  COM_TYPE_INTERIOR 	8
/* 源列表  end  */








typedef unsigned char	    byte;

typedef uint8_t             uint8;
typedef int8_t              int8;
typedef uint16_t            uint16;
typedef int16_t             int16;
typedef int32_t             int32;
typedef uint32_t            uint32;
typedef uint64_t            uint64;
typedef int64_t		        int64;
typedef float               float32;
typedef double              float64;

typedef char				str16[16];
typedef char				str32[32];
typedef char				str64[64];
typedef char				str128[128];
typedef char				str255[256];

extern int	SYS_BASE_TIME_uS;
extern double SYS_BASE_TIME;
extern volatile uint64 gSysCounter;
extern bool    gRunFlag;


#define CLOCK_MONOTONIC 0

typedef uint64 timestamp;

typedef enum _AccessType {
    ACCESS_RO = 1,
    ACCESS_WO = 2,
    ACCESS_RW = 3,
}EAccessType;

typedef enum _ESaveFlag {
        Save_None = 0,
        Save_Factory = 1,
        Save_User = 2,
        Save_Period = 3,
        Save_EEPROM = 4,
        Save_M4 = 5,
}ESaveFlag;

typedef	enum {//!!! : uint8_t
	DT_None = 0,
	DT_Bool = 1,
	DT_I8 = 2,
	DT_U8 = 3,
	DT_I16 = 4,
	DT_U16 = 5,
	DT_I32 = 6,
	DT_U32 = 7,
	DT_I64 = 8,
	DT_U64 = 9,
	DT_F32 = 10,
	DT_F64 = 11,
	DT_Str = 12,
	DT_WStr = 13,
	DT_ByteArray = 14,
	DT_RemapArray = 15,
	DT_DynaValue = 16
}EDataType;


typedef struct SDOControlType{
	bool 	PCWrite;//此标志用于放开pc控制修改数据
	uint8_t *pControlSource;
	struct {
	 void *pbuf;
	 uint16_t buf_len;
	 uint16_t cycle_us;//周期时间间隔
	}oversampling;
	uint16_t (*pu16HasScr_CallBack)(uint16_t);	/* 回调函数 */
	uint16_t (*pu16AfterSdoCpy_CallBack)(char*,uint16_t);	/* 回调函数 */
	void    (*pvReadSdo_CallBack)(void*,uint8_t);
}sSDOControlType;

typedef struct _SDOEntryDesc{
         uint8	uSubIndex;
         const char *	Name;
         const char *	Alias;
         EDataType DataType;
         EAccessType ObjAccess;
         ESaveFlag SaveFlag;
	sSDOControlType *pControlType; //
	void*	pVar;
	double	Minimum;
	double	Maximum;
	const char * 	Desc;
	const char *  ExtString;
        uint8	BitOffset;
	uint8	BitLength;
	uint8   change;//用于标识sdo被改变过
}SDOEntryDesc;


//typedef struct _SDOEntryDesc {
//	uint8	uSubIndex;
//	str255	Name;
//	str255	Alias;
//	EDataType DataType;
//	EAccessType ObjAccess;
//	ESaveFlag SaveFlag;
//	sSDOControlType *pControlType; //
//	void*	pVar;
//	double	Minimum;
//	double	Maximum;
//
//	str255 	Desc;
//	str255  ExtString;
//	uint8	BitOffset;
//	uint8	BitLength;
//	uint8   change;//用于标识sdo被改变过
//}SDOEntryDesc;

typedef struct _SDOEntryGroup {
	uint16	uIndex;
	SDOEntryDesc* pSdoEntryList;
	uint8	uSdoEntryCount;
}SDOEntryGroup;


typedef struct _ByteArray {
	uint32	uContentLength;
	byte*	pContent;
}ByteArray;


typedef struct _RemapDescr {
	uint16		nMemberCounter;
	SDOEntryDesc* MemberEntry[16];
}RemapDescr;

typedef	enum _EDynaVarType {
	DynaConst = 0,
	DynaLink
}EDynaVarType;


typedef struct _DynaVar {
	EDynaVarType		Type;
	EDataType			DataType;
	void*				pConstVar;
	str32				szLinkSdoKey;
	SDOEntryDesc*		pLinkSdo;
	float64				fValue;
}DynaVar;

typedef void (CycleFunction)(void);
typedef struct {
	int32       RunPeriod_uS;
	double		NextPeriod_uS;
    CycleFunction* pFunc;
    float64		ExecTime_uS;
    float32		MaxExecTime_uS;
    float32		MeanExecTime_uS;
    float32		MinExecTime_uS;

    float64		SysTickExecTime_uS;
    float32		SysTickMaxExecTime_uS;
    float32		SysTickMeanExecTime_uS;
    float32		SysTickMinExecTime_uS;
    struct{
    	float32		Sum;
    	int			Num;
    }_ExecTimeAuxCalc;
    float32		Jitter_uS;
    float32		MaxJitter_uS;
    float32		MinJitter_uS;

    float32		SysTickJitter_uS;
    float32		SysTickMaxJitter_uS;
    float32		SysTickMinJitter_uS;


    bool        bResetFlag;
    float32     CPULoad;
}ThreadExecInfo;

#if !defined(ATTR_PLACE_AT)

/* alway_inline */
#define ATTR_ALWAYS_INLINE __attribute__((always_inline))

/* alignment */
#define ATTR_ALIGN(alignment) __attribute__((aligned(alignment)))
#define ATTR_PACKED __attribute__((packed, aligned(1)))

/* place var_declare at section_name, e.x. PLACE_AT(".target_section", var); */
#define ATTR_PLACE_AT(section_name) __attribute__((section(section_name)))

#define ATTR_PLACE_AT_WITH_ALIGNMENT(section_name, alignment) \
ATTR_PLACE_AT(section_name) ATTR_ALIGN(alignment)

#define ATTR_PLACE_AT_NONCACHEABLE ATTR_PLACE_AT(".noncacheable")
#define ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(alignment) \
    ATTR_PLACE_AT_NONCACHEABLE ATTR_ALIGN(alignment)

#define ATTR_PLACE_AT_NONCACHEABLE_BSS ATTR_PLACE_AT(".noncacheable.bss")
#define ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(alignment) \
    ATTR_PLACE_AT_NONCACHEABLE_BSS ATTR_ALIGN(alignment)

/* initialize variable x with y using PLACE_AT_NONCACHEABLE_INIT(x) = {y}; */
#define ATTR_PLACE_AT_NONCACHEABLE_INIT ATTR_PLACE_AT(".noncacheable.init")
#define ATTR_PLACE_AT_NONCACHEABLE_INIT_WITH_ALIGNMENT(alignment) \
    ATTR_PLACE_AT_NONCACHEABLE_INIT ATTR_ALIGN(alignment)

/* .fast_ram section */
#define ATTR_PLACE_AT_FAST_RAM ATTR_PLACE_AT(".fast_ram")
#define ATTR_PLACE_AT_FAST_RAM_WITH_ALIGNMENT(alignment) \
    ATTR_PLACE_AT_FAST_RAM ATTR_ALIGN(alignment)

#define ATTR_PLACE_AT_FAST_RAM_BSS ATTR_PLACE_AT(".fast_ram.bss")
#define ATTR_PLACE_AT_FAST_RAM_BSS_WITH_ALIGNMENT(alignment) \
    ATTR_PLACE_AT_FAST_RAM_BSS ATTR_ALIGN(alignment)

#define ATTR_PLACE_AT_FAST_RAM_INIT ATTR_PLACE_AT(".fast_ram.init")
#define ATTR_PLACE_AT_FAST_RAM_INIT_WITH_ALIGNMENT(alignment) \
    ATTR_PLACE_AT_FAST_RAM_INIT ATTR_ALIGN(alignment)

#define ATTR_RAMFUNC ATTR_PLACE_AT(".fast")
#define ATTR_RAMFUNC_WITH_ALIGNMENT(alignment) \
    ATTR_RAMFUNC ATTR_ALIGN(alignment)

#define ATTR_SHARE_MEM ATTR_PLACE_AT(".sh_mem")

#define NOP() __asm volatile("nop")
#define WFI() __asm volatile("wfi")

#endif

// typedef int (*fs_open_t)(const char *file, int flags);
// typedef int (*fs_close_t)(int fd);
// typedef int (*fs_read_t)(int fd, void *buf, size_t len);
// typedef int (*fs_write_t)(int fd, const void *buf, size_t len);
// typedef int (*fs_unlink_t)(const char *pathname);
// typedef uint32_t (*fs_file_len_t)(const char *pathname);
// typedef struct {
//     fs_open_t open;
//     fs_close_t close;
//     fs_read_t read;
//     fs_write_t write;
//     fs_unlink_t unlink;
// 	fs_file_len_t file_len;
// }FileSystemFunc_t;

/** @todo CZG此处函数均在littleFS中实现，需要退耦合 */
int open(const char *file, int flags);
int close(int fd);
int read(int fd, void *buf, size_t len);
int write(int fd, const void *buf, size_t len);
uint32_t file_len(const char *name);


typedef void (*getTimeStampNs_t)(uint64_t * ts);
extern getTimeStampNs_t Libcore_GetTimeStampNs;
extern uint8_t *g_pPrintMode;
void Libcore_RegisterFunctions(getTimeStampNs_t pf, uint8_t *pPrintMode);

#ifdef __cplusplus
}
#endif



#endif /* BASE_H_ */
