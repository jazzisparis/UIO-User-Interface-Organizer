#pragma once
#pragma warning(disable: 4018 4221 4244 4267 4288 4302 4305 4311 4312 4733 4838 4996)

#include <windows.h>
#include <new>
#include <cstdio>
#include <type_traits>
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

#define ADDR_GameHeapAlloc			0xAA3E40
#define ADDR_GameHeapFree			0xAA4060
#define ADDR_CreateTileFromXML		0xA01B00
#define ADDR_GetXMLFileData			0xA1CE70
#define ADDR_GetAvailableLinkedNode	0x43A010
#define ADDR_ResolveXMLIncludes		0xA02D40
#define ADDR_InitXMLtoTileData		0xA0AC80
#define ADDR_strcmp					0xEC6DA0
#define ADDR_GetRandomInt			0xAA5230

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

#define GAME_HEAP_ALLOC __asm mov ecx, 0x11F6238 CALL_EAX(ADDR_GameHeapAlloc)
#define GAME_HEAP_FREE  __asm mov ecx, 0x11F6238 CALL_EAX(ADDR_GameHeapFree)

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

#define GameHeapAlloc(size) ThisCall<void*, UInt32>(ADDR_GameHeapAlloc, (void*)0x11F6238, size)
#define GameHeapFree(ptr) ThisCall<void, void*>(ADDR_GameHeapFree, (void*)0x11F6238, ptr)

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

bool __fastcall StrBeginsCI(const char *lstr, const char *rstr);

char* __fastcall FindChr(const char *str, char chr);

char* __fastcall FindChrR(const char *str, char chr);

char* __fastcall SubStrCI(const char *srcStr, const char *subStr, int length);

char* __fastcall GetNextToken(char *str, char delim);

char* __fastcall CopyString(const char *key);

int __fastcall StrToInt(const char *str);

double __vectorcall StrToDbl(const char *str);

bool __fastcall FileExists(const char *path);

class DirectoryIterator
{
	HANDLE				handle;
	WIN32_FIND_DATA		fndData;

public:
	DirectoryIterator(const char *path) : handle(FindFirstFile(path, &fndData)) {}
	~DirectoryIterator() {Close();}

	bool IsFile() const {return !(fndData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);}
	bool IsFolder() const
	{
		if (IsFile()) return false;
		if (fndData.cFileName[0] != '.') return true;
		if (fndData.cFileName[1] != '.') return fndData.cFileName[1] != 0;
		return fndData.cFileName[2] != 0;
	}
	void Close()
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}

	explicit operator bool() const {return handle != INVALID_HANDLE_VALUE;}
	const char* operator*() const {return fndData.cFileName;}
	void operator++() {if (!FindNextFile(handle, &fndData)) Close();}
};

class FileStream
{
	FILE		*theFile;

public:
	FileStream() : theFile(nullptr) {}
	FileStream(const char *filePath) : theFile(fopen(filePath, "rb")) {}
	~FileStream() {if (theFile) fclose(theFile);}

	bool Open(const char *filePath)
	{
		theFile = fopen(filePath, "rb");
		return theFile != nullptr;
	}

	bool OpenWrite(const char *filePath)
	{
		theFile = fopen(filePath, "wb");
		return theFile != nullptr;
	}

	void Close()
	{
		fclose(theFile);
		theFile = nullptr;
	}

	operator FILE*() const {return theFile;}
	explicit operator bool() const {return theFile != nullptr;}

	UInt32 GetLength()
	{
		fseek(theFile, 0, SEEK_END);
		UInt32 result = ftell(theFile);
		rewind(theFile);
		return result;
	}

	void ReadBuf(void *outData, UInt32 inLength)
	{
		fread(outData, inLength, 1, theFile);
	}

	void WriteBuf(const void *inData, UInt32 inLength)
	{
		fwrite(inData, inLength, 1, theFile);
	}
};

void PrintLog(const char *fmt, ...);

class LineIterator
{
	char	*dataPtr;

public:
	LineIterator(const char *filePath, char *buffer);

	explicit operator bool() const {return *dataPtr != 3;}
	char* operator*() {return dataPtr;}
	void operator++()
	{
		while (*dataPtr)
			dataPtr++;
		while (!*dataPtr)
			dataPtr++;
	}
};

void __stdcall SafeWrite8(UInt32 addr, UInt32 data);
void __stdcall SafeWrite16(UInt32 addr, UInt32 data);
void __stdcall SafeWrite32(UInt32 addr, UInt32 data);
void __stdcall SafeWriteBuf(UInt32 addr, void * data, UInt32 len);
#define SAFE_WRITE_BUF(addr, data) SafeWriteBuf(addr, data, sizeof(data) - 1)

void __stdcall WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt);
void __stdcall WriteRelCall(UInt32 patchAddr, UInt32 jumpTgt);

static const bool kValidForNumeric[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const double kDblPId180 = 0.017453292519943296, kDbl180dPI = 57.295779513082320877;
double __vectorcall dPow(double base, double exponent);
double __vectorcall dSin(double angle);
double __vectorcall dCos(double angle);
double __vectorcall dTan(double angle);
double __vectorcall dASin(double value);
double __vectorcall dACos(double value);
double __vectorcall dATan(double value);
double __vectorcall dLog(double value);