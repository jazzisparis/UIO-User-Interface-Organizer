#pragma once
#pragma warning(disable: 4244)

#include <windows.h>
#include <new>
#include <utility>
#include <cstdio>
#include <type_traits>
#include <initializer_list>
#include <intrin.h>

typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned long UInt32;
typedef signed long SInt32;
typedef unsigned long long UInt64;
typedef signed long long SInt64;

typedef void* (__cdecl *memcpy_t)(void*, const void*, size_t);
extern memcpy_t _memcpy, _memmove;

#define CALL_EAX(addr) __asm mov eax, addr __asm call eax
#define JMP_EAX(addr)  __asm mov eax, addr __asm jmp eax

// These are used for 10h aligning segments in ASM code (massive performance gain, particularly with loops).
#define EMIT(bt) __asm _emit bt
#define NOP_0x1 EMIT(0x90)
//	"\x90"
#define NOP_0x2 EMIT(0x66) NOP_0x1
//	"\x66\x90"
#define NOP_0x3 EMIT(0x0F) EMIT(0x1F) EMIT(0x00)
//	"\x0F\x1F\x00"
#define NOP_0x4 EMIT(0x0F) EMIT(0x1F) EMIT(0x40) EMIT(0x00)
//	"\x0F\x1F\x40\x00"
#define NOP_0x5 EMIT(0x0F) EMIT(0x1F) EMIT(0x44) EMIT(0x00) EMIT(0x00)
//	"\x0F\x1F\x44\x00\x00"
#define NOP_0x6 EMIT(0x66) NOP_0x5
//	"\x66\x0F\x1F\x44\x00\x00"
#define NOP_0x7 EMIT(0x0F) EMIT(0x1F) EMIT(0x80) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00)
//	"\x0F\x1F\x80\x00\x00\x00\x00"
#define NOP_0x8 EMIT(0x0F) EMIT(0x1F) EMIT(0x84) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00)
//	"\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0x9 EMIT(0x66) NOP_0x8
//	"\x66\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0xA EMIT(0x66) NOP_0x9
//	"\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0xB EMIT(0x66) NOP_0xA
//	"\x66\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0xC NOP_0x8 NOP_0x4
#define NOP_0xD NOP_0x8 NOP_0x5
#define NOP_0xE NOP_0x7 NOP_0x7
#define NOP_0xF NOP_0x8 NOP_0x7

#define GAME_HEAP_ALLOC __asm mov ecx, 0x11F6238 CALL_EAX(0xAA3E40)
#define GAME_HEAP_FREE  __asm mov ecx, 0x11F6238 CALL_EAX(0xAA4060)

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret ThisCall(UInt32 _addr, void *_this, Args ...args)
{
	return ((T_Ret (__thiscall *)(void*, Args...))_addr)(_this, std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret StdCall(UInt32 _addr, Args ...args)
{
	return ((T_Ret (__stdcall *)(Args...))_addr)(std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret CdeclCall(UInt32 _addr, Args ...args)
{
	return ((T_Ret (__cdecl *)(Args...))_addr)(std::forward<Args>(args)...);
}

#define GameHeapAlloc(size) ThisCall<void*, UInt32>(0xAA3E40, (void*)0x11F6238, size)
#define GameHeapFree(ptr) ThisCall<void, void*>(0xAA4060, (void*)0x11F6238, ptr)

class PrimitiveCS
{
	UInt32	owningThread;

public:
	PrimitiveCS() : owningThread(0) {}

	PrimitiveCS *Enter();
	__forceinline void Leave() {owningThread = 0;}
};

void __fastcall MemZero(void *dest, UInt32 bsize);

UInt32 __fastcall StrLen(const char *str);

char* __fastcall StrCopy(char *dest, const char *src);

char* __fastcall StrNCopy(char *dest, const char *src, UInt32 length);

char* __fastcall StrLenCopy(char *dest, const char *src, UInt32 length);

UInt32 __fastcall StrHash(const char *inKey);

char __fastcall StrCompare(const char *lstr, const char *rstr);

char __fastcall StrBeginsCS(const char *lstr, const char *rstr);

char __fastcall StrBeginsCI(const char *lstr, const char *rstr);

char* __fastcall FindChr(const char *str, char chr);

char* __fastcall FindChrR(const char *str, char chr);

char* __fastcall SubStr(const char *srcStr, const char *subStr, UInt32 length);

char* __fastcall GetNextToken(char *str, char delim);

char* __fastcall GetNextToken(char *str, const char *delims);

char* __fastcall CopyString(const char *key);

int __fastcall StrToInt(const char *str);

double __vectorcall StrToDbl(const char *str);

bool __fastcall FileExists(const char *path);

class DirectoryIterator
{
	HANDLE				handle;
	WIN32_FIND_DATAA	fndData;

public:
	DirectoryIterator(const char *path) : handle(FindFirstFileA(path, &fndData)) {}
	~DirectoryIterator() {Close();}

	bool End() const {return handle == INVALID_HANDLE_VALUE;}
	void Next() {if (!FindNextFileA(handle, &fndData)) Close();}
	bool IsFile() const {return !(fndData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);}
	bool IsFolder() const
	{
		if (IsFile()) return false;
		if (fndData.cFileName[0] != '.') return true;
		if (fndData.cFileName[1] != '.') return fndData.cFileName[1] != 0;
		return fndData.cFileName[2] != 0;
	}
	const char *Get() const {return fndData.cFileName;}
	void Close()
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}
};

class FileStream
{
protected:
	HANDLE		theFile;
	UInt32		streamLength;
	UInt32		streamOffset;

public:
	FileStream() : theFile(INVALID_HANDLE_VALUE) {}
	~FileStream() {if (Good()) Close();}

	bool Good() const {return theFile != INVALID_HANDLE_VALUE;}
	UInt32 GetLength() const {return streamLength;}
	UInt32 GetOffset() const {return streamOffset;}
	bool HitEOF() const {return streamOffset >= streamLength;}

	bool Open(const char *filePath);
	bool OpenAt(const char *filePath, UInt32 inOffset);
	bool Create(const char *filePath, UInt32 attr = FILE_ATTRIBUTE_NORMAL);
	bool OpenWrite(const char *filePath);

	void Close()
	{
		CloseHandle(theFile);
		theFile = INVALID_HANDLE_VALUE;
	}

	void ReadBuf(void *outData, UInt32 inLength);

	void WriteBuf(const void *inData, UInt32 inLength);
	void SetOffset(UInt32 inOffset);
};

void PrintLog(const char *fmt, ...);

class LineIterator
{
protected:
	char	*dataPtr;

public:
	LineIterator(const char *filePath, char *buffer);

	bool End() const {return *dataPtr == 3;}
	void Next();
	char *Get() {return dataPtr;}
};

void __stdcall SafeWrite8(UInt32 addr, UInt32 data);
void __stdcall SafeWrite16(UInt32 addr, UInt32 data);
void __stdcall SafeWrite32(UInt32 addr, UInt32 data);
void __stdcall SafeWriteBuf(UInt32 addr, void * data, UInt32 len);
#define SAFE_WRITE_BUF(addr, data) SafeWriteBuf(addr, data, sizeof(data) - 1)

void __stdcall WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt);
void __stdcall WriteRelCall(UInt32 patchAddr, void *procPtr);


static const double kDblPId180 = 0.017453292519943295;
double __vectorcall dPow(double base, double exponent);
double __vectorcall dSin(double angle);
double __vectorcall dCos(double angle);
double __vectorcall dTan(double angle);
double __vectorcall dLog(double angle);