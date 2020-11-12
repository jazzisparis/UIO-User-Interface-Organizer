#pragma once
#pragma warning(disable: 4244)

#include <windows.h>
#include <new>
#include <utility>
#include <cstdio>
#include <type_traits>

typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned long UInt32;
typedef signed long SInt32;
typedef unsigned long long UInt64;
typedef signed long long SInt64;

typedef void* (__cdecl *memcpy_t)(void*, const void*, size_t);
extern const memcpy_t _memcpy, _memmove;

template <typename T> __forceinline void RawAssign(const T &lhs, const T &rhs)
{
	struct Helper
	{
		UInt8	bytes[sizeof(T)];
	};
	*(Helper*)&lhs = *(Helper*)&rhs;
}

void __fastcall MemZero(void *dest, UInt32 bsize);

UInt32 __fastcall StrLen(const char *str);

char* __fastcall StrCopy(char *dest, const char *src);

char* __fastcall StrNCopy(char *dest, const char *src, UInt32 length);

char* __fastcall StrLenCopy(char *dest, const char *src, UInt32 length);

UInt32 __fastcall StrHash(const char *inKey);

bool __fastcall StrEqualCS(const char *lstr, const char *rstr);

bool __fastcall StrEqualCI(const char *lstr, const char *rstr);

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

void __stdcall WriteRelCall(UInt32 patchAddr, void *procPtr);