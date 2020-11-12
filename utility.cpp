#include "utility.h"

const memcpy_t _memcpy = memcpy, _memmove = memmove;

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
		jnz		proceed
		xor		eax, eax
		retn
	proceed:
		push	ecx
		test	ecx, 3
		jz		iter4
	iter1:
		mov		al, [ecx]
		inc		ecx
		test	al, al
		jz		done1
		test	ecx, 3
		jnz		iter1
		nop
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
		jmp		done
	done2:
		lea		eax, [ecx-2]
		jmp		done
	done3:
		lea		eax, [ecx-3]
		jmp		done
	done4:
		lea		eax, [ecx-4]
	done:
		pop		ecx
		sub		eax, ecx
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
		jnz		proceed
		mov		[eax], 0
	done:
		retn
	proceed:
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
		xor		eax, eax
		test	ecx, ecx
		jz		done
		mov		esi, ecx
		mov		ecx, eax
		jmp		iterHead
	done:
		pop		esi
		retn
		nop
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
	}
}

__declspec(naked) bool __fastcall StrEqualCS(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retn0
		test	edx, edx
		jz		retn0
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		push	eax
		mov		ecx, edi
		call	StrLen
		pop		edx
		cmp		eax, edx
		jnz		retn0
		test	edx, edx
		jz		retn1
		mov		ecx, edx
		shr		ecx, 2
		jz		comp1
		repe cmpsd
		jnz		retn0
	comp1:
		and		edx, 3
		jz		retn1
		mov		ecx, edx
		repe cmpsb
		jnz		retn0
	retn1:
		mov		al, 1
		pop		edi
		pop		esi
		retn
	retn0:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) bool __fastcall StrEqualCI(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retnFalse
		test	edx, edx
		jz		retnFalse
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		push	eax
		mov		ecx, edi
		call	StrLen
		pop		edx
		cmp		eax, edx
		jnz		retnFalse
		test	edx, edx
		jz		retnTrue
		xor		eax, eax
		mov		ecx, eax
	iterHead:
		mov		al, [esi]
		mov		cl, kCaseConverter[eax]
		mov		al, [edi]
		cmp		cl, kCaseConverter[eax]
		jnz		retnFalse
		inc		esi
		inc		edi
		dec		edx
		jnz		iterHead
	retnTrue:
		mov		al, 1
		pop		edi
		pop		esi
		retn
	retnFalse:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) char __fastcall StrCompare(const char *lstr, const char *rstr)
{
	__asm
	{
		push	ebx
		test	ecx, ecx
		jnz		proceed
		test	edx, edx
		jz		retnEQ
		jmp		retnLT
	proceed:
		test	edx, edx
		jz		retnGT
		xor		eax, eax
		mov		ebx, eax
		jmp		iterHead
		and		esp, 0xEFFFFFFF
		lea		esp, [esp]
		fnop
	iterHead:
		mov		al, [ecx]
		test	al, al
		jz		iterEnd
		mov		bl, kCaseConverter[eax]
		mov		al, [edx]
		cmp		bl, kCaseConverter[eax]
		jz		iterNext
		jl		retnLT
		jmp		retnGT
	iterNext:
		inc		ecx
		inc		edx
		jmp		iterHead
	iterEnd:
		cmp		[edx], 0
		jz		retnEQ
	retnLT:
		mov		al, -1
		pop		ebx
		retn
	retnGT:
		mov		al, 1
		pop		ebx
		retn
	retnEQ:
		xor		al, al
		pop		ebx
		retn
	}
}

__declspec(naked) char __fastcall StrBeginsCS(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retn0
		test	edx, edx
		jz		retn0
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		push	eax
		mov		ecx, edi
		call	StrLen
		pop		edx
		cmp		eax, edx
		jg		retn0
		test	edx, edx
		jz		retn1
		mov		ecx, eax
		shr		ecx, 2
		jz		comp1
		repe cmpsd
		jnz		retn0
	comp1:
		and		eax, 3
		jz		retn1
		mov		ecx, eax
		repe cmpsb
		jnz		retn0
	retn1:
		cmp		[esi], 0
		setz	al
		inc		al
		pop		edi
		pop		esi
		retn
	retn0:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) char __fastcall StrBeginsCI(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retn0
		test	edx, edx
		jz		retn0
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		push	eax
		mov		ecx, edi
		call	StrLen
		pop		edx
		cmp		eax, edx
		jg		retn0
		test	edx, edx
		jz		iterEnd
		xor		ecx, ecx
		mov		edx, ecx
		jmp		iterHead
		and		esp, 0xEFFFFFFF
	iterHead:
		mov		cl, [edi]
		test	cl, cl
		jz		iterEnd
		mov		dl, kCaseConverter[ecx]
		mov		cl, [esi]
		cmp		dl, kCaseConverter[ecx]
		jnz		retn0
		inc		esi
		inc		edi
		dec		eax
		jnz		iterHead
	iterEnd:
		cmp		[esi], 0
		setz	al
		inc		al
		pop		edi
		pop		esi
		retn
	retn0:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) char* __fastcall FindChr(const char *str, char chr)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jnz		iterHead
		retn
	retnNULL:
		xor		eax, eax
	done:
		retn
		and		esp, 0xEFFFFFFF
	iterHead:
		cmp		[eax], 0
		jz		retnNULL
		cmp		[eax], dl
		jz		done
		inc		eax
		jmp		iterHead
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

__declspec(naked) char* __fastcall GetNextToken(char *str, char delim)
{
	__asm
	{
		push	ebx
		mov		eax, ecx
		xor		bl, bl
		jmp		iterHead
	done:
		pop		ebx
		retn
		and		esp, 0xEFFFFFFF
		nop
	iterHead:
		mov		cl, [eax]
		test	cl, cl
		jz		done
		cmp		cl, dl
		jz		chrEQ
		test	bl, bl
		jnz		done
		jmp		iterNext
	chrEQ:
		test	bl, bl
		jnz		iterNext
		mov		bl, 1
		mov		[eax], 0
	iterNext:
		inc		eax
		jmp		iterHead
	}
}

__declspec(naked) char* __fastcall GetNextToken(char *str, const char *delims)
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		sub		esp, 0x100
		push	esi
		push	edi
		mov		esi, ecx
		lea		edi, [ebp-0x100]
		xor		eax, eax
		mov		ecx, 0x40
		rep stosd
		lea		edi, [ebp-0x100]
	dlmIter:
		mov		al, [edx]
		test	al, al
		jz		mainHead
		mov		byte ptr [edi+eax], 1
		inc		edx
		jmp		dlmIter
		nop
	mainHead:
		mov		al, [esi]
		test	al, al
		jz		done
		cmp		byte ptr [edi+eax], 0
		jz		wasFound
		inc		ecx
		mov		[esi], 0
	mainNext:
		inc		esi
		jmp		mainHead
	wasFound:
		test	cl, cl
		jz		mainNext
	done:
		mov		eax, esi
		pop		edi
		pop		esi
		mov		esp, ebp
		pop		ebp
		retn
	}
}

__declspec(naked) int __fastcall StrToInt(const char *str)
{
	__asm
	{
		xor		eax, eax
		test	ecx, ecx
		jnz		proceed
		retn
	proceed:
		push	esi
		mov		esi, ecx
		mov		ecx, eax
		cmp		[esi], '-'
		setz	dl
		jnz		charIter
		inc		esi
	charIter:
		mov		cl, [esi]
		sub		cl, '0'
		cmp		cl, 9
		ja		iterEnd
		imul	eax, 0xA
		jo		overflow
		add		eax, ecx
		inc		esi
		jmp		charIter
	overflow:
		mov		eax, 0x7FFFFFFF
	iterEnd:
		test	dl, dl
		jz		done
		neg		eax
	done:
		pop		esi
		retn
	}
}

__declspec(naked) double __vectorcall StrToDbl(const char *str)
{
	static const double kFactor10Div[] = {10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
	static const double kValueBounds[] = {4294967296, -4294967296};
	alignas(16) static const UInt64 kSignMask[] = {0x8000000000000000, 0x8000000000000000};
	__asm
	{
		pxor	xmm0, xmm0
		test	ecx, ecx
		jnz		proceed
		retn
	proceed:
		push	ebx
		push	esi
		push	edi
		mov		esi, ecx
		mov		edi, 0xA
		xor		eax, eax
		mov		ebx, eax
		mov		ecx, eax
		cmp		[esi], '-'
		setz	cl
		jnz		intIter
		inc		esi
	intIter:
		mov		bl, [esi]
		sub		bl, '0'
		cmp		bl, 9
		ja		intEnd
		mul		edi
		jo		overflow
		add		eax, ebx
		inc		esi
		jmp		intIter
	overflow:
		movsd	xmm0, kValueBounds[ecx*8]
		jmp		done
	intEnd:
		test	eax, eax
		jz		noInt
		cvtsi2sd	xmm0, eax
		jns		noOverflow
		addsd	xmm0, kValueBounds
	noOverflow:
		shl		cl, 1
		xor		eax, eax
	noInt:
		cmp		bl, 0xFE
		jnz		addSign
		mov		ch, 0xFF
	fracIter:
		inc		esi
		mov		bl, [esi]
		sub		bl, '0'
		cmp		bl, 9
		ja		fracEnd
		mul		edi
		add		eax, ebx
		inc		ch
		cmp		ch, 8
		jb		fracIter
	fracEnd:
		test	eax, eax
		jz		addSign
		cvtsi2sd	xmm1, eax
		shl		cl, 1
		mov		bl, ch
		divsd	xmm1, kFactor10Div[ebx*8]
		addsd	xmm0, xmm1
	addSign:
		test	cl, 6
		jz		done
		xorpd	xmm0, kSignMask
	done:
		pop		edi
		pop		esi
		pop		ebx
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

bool FileStream::Open(const char *filePath)
{
	theFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (theFile == INVALID_HANDLE_VALUE)
		return false;
	streamLength = GetFileSize(theFile, NULL);
	streamOffset = 0;
	return true;
}

bool FileStream::OpenAt(const char *filePath, UInt32 inOffset)
{
	theFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (theFile == INVALID_HANDLE_VALUE)
		return false;
	streamLength = GetFileSize(theFile, NULL);
	if (inOffset > streamLength)
	{
		Close();
		return false;
	}
	streamOffset = inOffset;
	if (streamOffset)
		SetFilePointer(theFile, streamOffset, NULL, FILE_BEGIN);
	return true;
}

bool FileStream::Create(const char *filePath, UInt32 attr)
{
	theFile = CreateFileA(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, attr, NULL);
	streamOffset = streamLength = 0;
	return theFile != INVALID_HANDLE_VALUE;
}

bool FileStream::OpenWrite(const char *filePath)
{
	theFile = CreateFileA(filePath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (theFile == INVALID_HANDLE_VALUE)
		return false;
	streamOffset = streamLength = GetFileSize(theFile, NULL);
	SetFilePointer(theFile, streamLength, NULL, FILE_BEGIN);
	return true;
}

void FileStream::ReadBuf(void *outData, UInt32 inLength)
{
	UInt32 bytesRead;
	ReadFile(theFile, outData, inLength, &bytesRead, NULL);
	streamOffset += bytesRead;
}

__declspec(noinline) void FileStream::WriteBuf(const void *inData, UInt32 inLength)
{
	if (streamOffset > streamLength)
		SetEndOfFile(theFile);
	UInt32 bytesWritten;
	WriteFile(theFile, inData, inLength, &bytesWritten, NULL);
	streamOffset += bytesWritten;
	if (streamLength < streamOffset)
		streamLength = streamOffset;
}

void FileStream::SetOffset(UInt32 inOffset)
{
	if (inOffset > streamLength)
		streamOffset = streamLength;
	else streamOffset = inOffset;
	SetFilePointer(theFile, streamOffset, NULL, FILE_BEGIN);
}

extern FileStream s_log;

void PrintLog(const char *fmt, ...)
{
	char buffer[0x200];
	va_list args;
	va_start(args, fmt);
	int length = vsprintf_s(buffer, 0x200, fmt, args);
	if (length >= 0)
	{
		buffer[length] = '\n';
		s_log.WriteBuf(buffer, length + 1);
	}
	va_end(args);
}

LineIterator::LineIterator(const char *filePath, char *buffer)
{
	dataPtr = buffer;
	FileStream sourceFile;
	if (!sourceFile.Open(filePath))
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