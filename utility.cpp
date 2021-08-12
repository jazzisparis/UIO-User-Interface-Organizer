#include "utility.h"

memcpy_t _memcpy = memcpy, _memmove = memmove;

#define FAST_SLEEP_COUNT 10000UL

__declspec(naked) PrimitiveCS *PrimitiveCS::Enter()
{
	__asm
	{
		push	ebx
		mov		ebx, ecx
		call	GetCurrentThreadId
		cmp		[ebx], eax
		jnz		doSpin
	done:
		mov		eax, ebx
		pop		ebx
		retn
		NOP_0xA
	doSpin:
		mov		ecx, eax
		xor		eax, eax
		lock cmpxchg [ebx], ecx
		test	eax, eax
		jz		done
		push	esi
		push	edi
		mov		esi, ecx
		mov		edi, FAST_SLEEP_COUNT
	spinHead:
		dec		edi
		mov		edx, edi
		shr		edx, 0x1F
		push	edx
		call	Sleep
		xor		eax, eax
		lock cmpxchg [ebx], esi
		test	eax, eax
		jnz		spinHead
		pop		edi
		pop		esi
		mov		eax, ebx
		pop		ebx
		retn
	}
}

__declspec(naked) void __fastcall MemZero(void *dest, UInt32 bsize)
{
	__asm
	{
		push	edi
		test	ecx, ecx
		jz		done
		mov		edi, ecx
		xor		eax, eax
		mov		ecx, edx
		shr		ecx, 2
		jz		write1
		rep stosd
	write1:
		and		edx, 3
		jz		done
		mov		ecx, edx
		rep stosb
	done:
		pop		edi
		retn
	}
}

__declspec(naked) UInt32 __fastcall StrLen(const char *str)
{
	__asm
	{
		test	ecx, ecx
		jz		nullPtr
		push	ecx
		test	ecx, 3
		jz		iter4
		ALIGN 16
	iter1:
		mov		al, [ecx]
		inc		ecx
		test	al, al
		jz		done1
		test	ecx, 3
		jnz		iter1
		ALIGN 16
	iter4:
		mov		eax, [ecx]
		mov		edx, 0x7EFEFEFF
		add		edx, eax
		not		eax
		xor		eax, edx
		add		ecx, 4
		test	eax, 0x81010100
		jz		iter4
		mov		eax, [ecx-4]
		test	al, al
		jz		done4
		test	ah, ah
		jz		done3
		test	eax, 0xFF0000
		jz		done2
		test	eax, 0xFF000000
		jnz		iter4
	done1:
		lea		eax, [ecx-1]
		pop		ecx
		sub		eax, ecx
		retn
	done2:
		lea		eax, [ecx-2]
		pop		ecx
		sub		eax, ecx
		retn
	done3:
		lea		eax, [ecx-3]
		pop		ecx
		sub		eax, ecx
		retn
	done4:
		lea		eax, [ecx-4]
		pop		ecx
		sub		eax, ecx
		retn
	nullPtr:
		xor		eax, eax
		retn
	}
}

__declspec(naked) char* __fastcall StrCopy(char *dest, const char *src)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		push	ecx
		mov		ecx, edx
		call	StrLen
		pop		edx
		push	eax
		inc		eax
		push	eax
		push	ecx
		push	edx
		call	_memmove
		add		esp, 0xC
		pop		ecx
		add		eax, ecx
		retn
	nullTerm:
		mov		[eax], 0
	done:
		retn
	}
}

__declspec(naked) char* __fastcall StrNCopy(char *dest, const char *src, UInt32 length)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		cmp		dword ptr [esp+4], 0
		jz		nullTerm
		push	esi
		mov		esi, ecx
		mov		ecx, edx
		call	StrLen
		mov		edx, [esp+8]
		cmp		edx, eax
		cmova	edx, eax
		push	edx
		push	ecx
		push	esi
		add		esi, edx
		call	_memmove
		add		esp, 0xC
		mov		eax, esi
		pop		esi
	nullTerm:
		mov		[eax], 0
	done:
		retn	4
	}
}

__declspec(naked) char* __fastcall StrLenCopy(char *dest, const char *src, UInt32 length)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		mov		ecx, [esp+4]
		test	ecx, ecx
		jz		nullTerm
		push	ecx
		push	edx
		push	eax
		call	_memmove
		add		esp, 0xC
		add		eax, [esp+4]
	nullTerm:
		mov		[eax], 0
	done:
		retn	4
	}
}

alignas(16) static const UInt8 kCaseConverter[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

__declspec(naked) UInt32 __fastcall StrHash(const char *inKey)
{
	__asm
	{
		push	esi
		mov		eax, 0x1505
		test	ecx, ecx
		jz		done
		mov		esi, ecx
		xor		ecx, ecx
		ALIGN 16
	iterHead:
		mov		cl, [esi]
		test	cl, cl
		jz		done
		mov		edx, eax
		shl		edx, 5
		add		eax, edx
		movzx	edx, kCaseConverter[ecx]
		add		eax, edx
		inc		esi
		jmp		iterHead
	done:
		pop		esi
		retn
	}
}

__declspec(noinline) char __fastcall StrCompare(const char *lstr, const char *rstr)
{
	if (!lstr) return rstr ? -1 : 0;
	if (!rstr) return 1;
	UInt8 lchr, rchr;
	while (*lstr)
	{
		lchr = kCaseConverter[*(UInt8*)lstr];
		rchr = kCaseConverter[*(UInt8*)rstr];
		if (lchr == rchr)
		{
			lstr++;
			rstr++;
			continue;
		}
		return (lchr < rchr) ? -1 : 1;
	}
	return *rstr ? -1 : 0;
}

__declspec(noinline) char __fastcall StrBeginsCS(const char *lstr, const char *rstr)
{
	if (!lstr || !rstr) return 0;
	UInt32 length = StrLen(rstr);
	while (length >= 4)
	{
		if (*(UInt32*)lstr != *(UInt32*)rstr)
			return 0;
		lstr += 4;
		rstr += 4;
		length -= 4;
	}
	while (length)
	{
		if (*lstr != *rstr)
			return 0;
		lstr++;
		rstr++;
		length--;
	}
	return *lstr ? 1 : 2;
}

__declspec(noinline) char __fastcall StrBeginsCI(const char *lstr, const char *rstr)
{
	if (!lstr || !rstr) return 0;
	UInt32 length = StrLen(rstr);
	while (length)
	{
		if (kCaseConverter[*(UInt8*)lstr] != kCaseConverter[*(UInt8*)rstr])
			return 0;
		lstr++;
		rstr++;
		length--;
	}
	return *lstr ? 1 : 2;
}

__declspec(naked) char* __fastcall FindChr(const char *str, char chr)
{
	__asm
	{
	iterHead:
		mov		al, [ecx]
		test	al, al
		jz		retnNULL
		cmp		al, dl
		jz		found
		inc		ecx
		jmp		iterHead
		ALIGN 16
	found:
		mov		eax, ecx
		retn
		ALIGN 16
	retnNULL:
		xor		eax, eax
		retn
	}
}

__declspec(naked) char* __fastcall FindChrR(const char *str, char chr)
{
	__asm
	{
		push	edx
		call	StrLen
		pop		edx
		add		eax, ecx
		jmp		iterHead
		mov		eax, 0
	iterHead:
		cmp		eax, ecx
		jz		retnNULL
		dec		eax
		cmp		[eax], dl
		jz		done
		jmp		iterHead
	retnNULL:
		xor		eax, eax
	done:
		retn
	}
}

__declspec(naked) char* __fastcall SubStr(const char *srcStr, const char *subStr, UInt32 length)
{
	__asm
	{
		push	ebx
		push	esi
		push	edi
		test	ecx, ecx
		jz		retnNULL
		test	edx, edx
		jz		retnNULL
		cmp		[edx], 0
		jz		retnNULL
		cmp		dword ptr [esp+0x10], 0
		jz		retnNULL
		mov		esi, ecx
		mov		edi, edx
		xor		eax, eax
		mov		ebx, eax
		jmp		subHead
	mainNext:
		dec		dword ptr [esp+0x10]
		js		retnNULL
		inc		esi
		mov		ecx, esi
		mov		edx, edi
	subHead:
		mov		al, [edx]
		test	al, al
		jnz		proceed
		mov		eax, esi
		jmp		done
	proceed:
		mov		bl, kCaseConverter[eax]
		mov		al, [ecx]
		cmp		bl, kCaseConverter[eax]
		jnz		mainNext
		inc		ecx
		inc		edx
		jmp		subHead
	retnNULL:
		xor		eax, eax
	done:
		pop		edi
		pop		esi
		pop		ebx
		retn	4
	}
}

__declspec(noinline) char* __fastcall GetNextToken(char *str, char delim)
{
	if (!str) return NULL;
	bool found = false;
	char chr;
	while (chr = *str)
	{
		if (chr == delim)
		{
			*str = 0;
			found = true;
		}
		else if (found)
			break;
		str++;
	}
	return str;
}

__declspec(noinline) char* __fastcall GetNextToken(char *str, const char *delims)
{
	if (!str) return NULL;
	bool table[0x100];
	MemZero(table, 0x100);
	UInt8 curr;
	while (curr = *delims)
	{
		table[curr] = true;
		delims++;
	}
	bool found = false;
	while (curr = *str)
	{
		if (table[curr])
		{
			*str = 0;
			found = true;
		}
		else if (found)
			break;
		str++;
	}
	return str;
}

__declspec(naked) int __fastcall StrToInt(const char *str)
{
	__asm
	{
		push	esi
		push	edi
		xor		eax, eax
		test	ecx, ecx
		jz		done
		mov		esi, ecx
		xor		ecx, ecx
		xor		edi, edi
		cmp		[esi], '-'
		setz	dl
		jnz		charIter
		inc		esi
		ALIGN 16
	charIter:
		mov		cl, [esi]
		sub		cl, '0'
		cmp		cl, 9
		ja		iterEnd
		lea		eax, [eax+eax*4]
		lea		eax, [ecx+eax*2]
		cmp		eax, edi
		jl		overflow
		mov		edi, eax
		inc		esi
		jmp		charIter
		ALIGN 16
	iterEnd:
		test	dl, dl
		jz		done
		neg		eax
	done:
		pop		edi
		pop		esi
		retn
	overflow:
		movzx	eax, dl
		add		eax, 0x7FFFFFFF
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) double __vectorcall StrToDbl(const char *str)
{
	static const double kValueBounds[] = {4294967296, -4294967296}, kFactor10Div[] = {1.0e-09, 1.0e-08, 1.0e-07, 1.0e-06, 1.0e-05, 0.0001, 0.001, 0.01, 0.1};
	__asm
	{
		push	esi
		push	edi
		pxor	xmm0, xmm0
		test	ecx, ecx
		jz		done
		mov		esi, ecx
		xor		eax, eax
		xor		ecx, ecx
		xor		edi, edi
		cmp		[esi], '-'
		setz	dl
		jnz		intIter
		inc		esi
		ALIGN 16
	intIter:
		mov		cl, [esi]
		sub		cl, '0'
		cmp		cl, 9
		ja		intEnd
		lea		eax, [eax+eax*4]
		lea		eax, [ecx+eax*2]
		cmp		eax, edi
		jb		overflow
		mov		edi, eax
		inc		esi
		jmp		intIter
		ALIGN 16
	intEnd:
		test	eax, eax
		jz		noInt
		movd	xmm0, eax
		cvtdq2pd	xmm0, xmm0
		jns		noOverflow
		addsd	xmm0, kValueBounds
	noOverflow:
		shl		dl, 1
		xor		eax, eax
	noInt:
		cmp		cl, 0xFE
		jnz		addSign
		mov		dh, 9
		ALIGN 16
	fracIter:
		inc		esi
		mov		cl, [esi]
		sub		cl, '0'
		cmp		cl, 9
		ja		fracEnd
		lea		eax, [eax+eax*4]
		lea		eax, [ecx+eax*2]
		dec		dh
		jnz		fracIter
	fracEnd:
		test	eax, eax
		jz		addSign
		movd	xmm1, eax
		cvtdq2pd	xmm1, xmm1
		shl		dl, 1
		mov		cl, dh
		mulsd	xmm1, kFactor10Div[ecx*8]
		addsd	xmm0, xmm1
	addSign:
		test	dl, 6
		jz		done
		pcmpeqd	xmm1, xmm1
		psllq	xmm1, 0x3F
		orpd	xmm0, xmm1
	done:
		pop		edi
		pop		esi
		retn
	overflow:
		movzx	eax, dl
		movq	xmm0, kValueBounds[eax*8]
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) char* __fastcall CopyString(const char *key)
{
	__asm
	{
		call	StrLen
		inc		eax
		push	eax
		push	ecx
		push	eax
		call	malloc
		pop		ecx
		push	eax
		call	_memcpy
		add		esp, 0xC
		retn
	}
}

bool __fastcall FileExists(const char *path)
{
	UInt32 attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

FileStream::FileStream(const char *filePath)
{
	theFile = fopen(filePath, "rb");
}

bool FileStream::Open(const char *filePath)
{
	theFile = fopen(filePath, "rb");
	return theFile != nullptr;
}

bool FileStream::OpenWrite(const char *filePath)
{
	theFile = fopen(filePath, "wb");
	return theFile != nullptr;
}

UInt32 FileStream::GetLength()
{
	fseek(theFile, 0, SEEK_END);
	UInt32 result = ftell(theFile);
	rewind(theFile);
	return result;
}

void FileStream::ReadBuf(void *outData, UInt32 inLength)
{
	fread(outData, inLength, 1, theFile);
}

void FileStream::WriteBuf(const void *inData, UInt32 inLength)
{
	fwrite(inData, inLength, 1, theFile);
}

int FileStream::WriteFmtStr(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int iWritten = vfprintf(theFile, fmt, args);
	va_end(args);
	return iWritten;
}

extern FileStream s_log;

void PrintLog(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(s_log.theFile, fmt, args);
	va_end(args);
	fflush(s_log.theFile);
}

LineIterator::LineIterator(const char *filePath, char *buffer)
{
	dataPtr = buffer;
	FileStream sourceFile(filePath);
	if (!sourceFile)
	{
		*buffer = 3;
		return;
	}
	UInt32 length = sourceFile.GetLength();
	sourceFile.ReadBuf(buffer, length);
	*(UInt16*)(buffer + length) = 0x300;
	while (length)
	{
		length--;
		if ((*buffer == '\n') || (*buffer == '\r'))
			*buffer = 0;
		buffer++;
	}
	while (!*dataPtr)
		dataPtr++;
}

void LineIterator::Next()
{
	while (*dataPtr)
		dataPtr++;
	while (!*dataPtr)
		dataPtr++;
}

__declspec(naked) void __stdcall SafeWrite8(UInt32 addr, UInt32 data)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	4
		push	dword ptr [esp+0x14]
		call	VirtualProtect
		mov		eax, [esp+8]
		mov		edx, [esp+0xC]
		mov		[eax], dl
		mov		edx, [esp]
		push	esp
		push	edx
		push	4
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall SafeWrite16(UInt32 addr, UInt32 data)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	4
		push	dword ptr [esp+0x14]
		call	VirtualProtect
		mov		eax, [esp+8]
		mov		edx, [esp+0xC]
		mov		[eax], dx
		mov		edx, [esp]
		push	esp
		push	edx
		push	4
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall SafeWrite32(UInt32 addr, UInt32 data)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	4
		push	dword ptr [esp+0x14]
		call	VirtualProtect
		mov		eax, [esp+8]
		mov		edx, [esp+0xC]
		mov		[eax], edx
		mov		edx, [esp]
		push	esp
		push	edx
		push	4
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall SafeWriteBuf(UInt32 addr, void *data, UInt32 len)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	dword ptr [esp+0x18]
		push	dword ptr [esp+0x14]
		call	VirtualProtect
		push	dword ptr [esp+0x10]
		push	dword ptr [esp+0x10]
		push	dword ptr [esp+0x10]
		call	_memcpy
		add		esp, 0xC
		mov		edx, [esp]
		push	esp
		push	edx
		push	dword ptr [esp+0x18]
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	0xC
	}
}

__declspec(naked) void __stdcall WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	5
		push	dword ptr [esp+0x14]
		call	VirtualProtect
		mov		eax, [esp+8]
		mov		edx, [esp+0xC]
		sub		edx, eax
		sub		edx, 5
		mov		byte ptr [eax], 0xE9
		mov		[eax+1], edx
		mov		edx, [esp]
		push	esp
		push	edx
		push	5
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall WriteRelCall(UInt32 patchAddr, void *procPtr)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	5
		push	dword ptr [esp+0x14]
		call	VirtualProtect
		mov		eax, [esp+8]
		mov		edx, [esp+0xC]
		sub		edx, eax
		sub		edx, 5
		mov		byte ptr [eax], 0xE8
		mov		[eax+1], edx
		mov		edx, [esp]
		push	esp
		push	edx
		push	5
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

double __vectorcall dPow(double base, double exponent)
{
	return pow(base, exponent);
}

double __vectorcall dSin(double angle)
{
	return sin(angle);
}

double __vectorcall dCos(double angle)
{
	return cos(angle);
}

double __vectorcall dTan(double angle)
{
	return tan(angle);
}

double __vectorcall dASin(double value)
{
	return asin(value);
}

double __vectorcall dACos(double value)
{
	return acos(value);
}

double __vectorcall dATan(double value)
{
	return atan(value);
}

double __vectorcall dLog(double value)
{
	return log(value);
}