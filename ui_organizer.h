#pragma once

#include "utility.h"
#include "containers.h"
#include "gameapi.h"

UInt32
kHeapAllocAddr =			0xAA3E40,
kHeapFreeAddr =				0xAA4060,
kInitInterfaceMngrAddr =	0x70A130,
kAddrSub43A010 =			0x43A010,
kAddrSubA0AC80 =			0xA0AC80,
kAddrSubA0AE70 =			0xA0AE70,
kAddrSubA0AF10 =			0xA0AF10,
kAddrSubAE8010 =			0xAE8010;

struct XMLFileData
{
	UInt32		length;
	char		*data;

	void Destroy()
	{
		GameHeapFree(data);
		GameHeapFree(this);
	}
};

typedef XMLFileData* (*_GetXMLFileData)(const char *filePath, UInt32);
_GetXMLFileData GetXMLFileData = (_GetXMLFileData)0xA1CE70;

typedef bool (*_ResolveIncludes)(XMLFileData *fileData);
_ResolveIncludes ResolveIncludes = (_ResolveIncludes)0xA02D40;

struct CacheOffset
{
	UInt32		begin;
	UInt32		length;

	CacheOffset() : begin(0), length(0) {}
	CacheOffset(UInt32 _begin, UInt32 _length) : begin(_begin), length(_length) {}
};

FileStream s_log;
UnorderedMap<const char*, CacheOffset> s_cachedBaseFiles(0x80);
UnorderedMap<const char*, char*> s_cachedParsedData(0x20);
bool s_debugMode = false, s_debugPrint = false;
char s_debugPath[0x80] = "uio_debug\\";
const char *s_logPath = "ui_organizer.log",
kTempCacheFile[] = "uio_temp_cache",
*kErrorStrings[] =
{
	"<!>\t\tExpected: 'name=' or 'src='\n",
	"<!>\t\tExpected: 'trait='\n",
	"<!>\t\tExpected: '='\n",
	"<!>\t\tEmpty attribute name\n"
},
kResolveError[] = "\n<!> Errors encountered while loading ";
const UInt8 kErrorLengths[] = {0x21, 0x18, 0x13, 0x1A};

UnorderedSet<const char*> s_logCreated(0x40);
NiTMap *g_gameSettingsMap = NULL;

void __stdcall InitDebugLog(const char *filePath, XMLFileData *fileData)
{
	if (!s_logCreated.Insert(filePath))
		return;
	const char *fileName = FindChrR(filePath, '\\');
	if (fileName) fileName++;
	else fileName = filePath;
	char *pathEnd = StrCopy(s_debugPath + 10, fileName);
	if (!s_log.Create(s_debugPath))
		return;
	s_log.WriteBuf(fileData->data, fileData->length);
	s_log.Close();
	memcpy(pathEnd, ".log", 5);
	s_debugPrint = s_log.Create(s_debugPath);
}

void __fastcall PrintTraitEntry(UInt32 *args)
{
	char buffer[0x200];
	int length;
	switch (args[2])
	{
		case 3:
			length = sprintf_s(buffer, 0x200, "%04X\t#%04X\n", args[0], args[1]);
			break;
		case 1:
			length = sprintf_s(buffer, 0x200, "%04X\t%.4f\n", args[0], *(float*)(args + 1));
			break;
		default:
			length = sprintf_s(buffer, 0x200, "%04X\t\"%s\"\n", args[0], (char*)args[1]);
	}
	s_log.WriteBuf(buffer, length);
}

void __fastcall PrintErrorInvID(const char *name)
{
	static char errorUnkID[0x80] = "<!>\t\tUnknown tag name: '";
	char *endPtr = StrLenCopy(StrCopy(errorUnkID + 24, name), "'\n", 2);
	s_log.WriteBuf(errorUnkID, endPtr - errorUnkID);
}

__declspec(naked) XMLFileData* __cdecl GetXMLFileDataHook(const char *filePath, UInt32 arg2)
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		sub		esp, 0x10
		push	esi
		push	edi
		xor		esi, esi
		mov		edi, esi
		lea		eax, [ebp-4]
		mov		[eax], esi
		mov		ecx, [ebp+8]
		cmp		dword ptr [ecx], '_pij'
		jz		notCached
		push	eax
		push	ecx
		mov		ecx, offset s_cachedBaseFiles
		call	UnorderedMap<const char*, CacheOffset>::Insert
		test	al, al
		jnz		notCached
		mov		eax, [ebp-4]
		cmp		dword ptr [eax+4], 0
		jz		createData
		push	dword ptr [eax]
		push	offset kTempCacheFile
		lea		ecx, [ebp-0x10]
		call	FileStream::OpenAt
		test	al, al
		jz		createData
		mov		eax, [ebp-4]
		mov		esi, [eax+4]
		lea		eax, [esi+1]
		push	eax
		call	GameHeapAlloc
		mov		edi, eax
		push	esi
		push	eax
		lea		ecx, [ebp-0x10]
		call	FileStream::ReadBuf
		lea		ecx, [ebp-0x10]
		call	FileStream::Close
		mov		byte ptr [edi+esi], 0
		jmp		createData
	notCached:
		push	0
		push	dword ptr [ebp+8]
		call	GetXMLFileData
		add		esp, 8
		test	eax, eax
		jz		createData
		cmp		dword ptr [ebp-4], 0
		jz		done
		mov		edi, [eax]
		test	edi, edi
		jz		done
		mov		esi, eax
		push	offset kTempCacheFile
		lea		ecx, [ebp-0x10]
		call	FileStream::OpenWrite
		test	al, al
		mov		eax, esi
		jz		done
		lea		ecx, [ebp-0x10]
		mov		edx, [ecx+4]
		mov		eax, [ebp-4]
		mov		[eax], edx
		mov		[eax+4], edi
		push	edi
		push	dword ptr [esi+4]
		call	FileStream::WriteBuf
		lea		ecx, [ebp-0x10]
		call	FileStream::Close
		mov		eax, esi
		jmp		done
	createData:
		push	8
		call	GameHeapAlloc
		mov		[eax], esi
		mov		[eax+4], edi
	done:
		pop		edi
		pop		esi
		mov		esp, ebp
		pop		ebp
		retn
	}
}

__declspec(naked) void __fastcall FreeExtraString(void *xString)
{
	__asm
	{
		mov		eax, 0x11F32DC
		mov		edx, [eax]
		cmp		edx, 0xA
		jnb		doFree
		mov		[eax+edx*4-0x40], ecx
		inc		edx
		mov		[eax], edx
		retn
	doFree:
		push	dword ptr [ecx+8]
		push	ecx
		call	GameHeapFree
		call	GameHeapFree
		retn
	}
}

__declspec(naked) void __stdcall ResolveTrait(UInt32 valueType, const char *valueStr, UInt8 dataType)
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		sub		esp, 0x1C
		mov		eax, [ecx+0xC]
		test	eax, eax
		cmovz	eax, [ecx]
		mov		[ebp-4], eax
		cmp		s_debugPrint, 0
		jz		skipLog
		lea		ecx, [ebp+8]
		call	PrintTraitEntry
	skipLog:
		mov		eax, 0x80000000
		cmp		[ebp+0x10], 3
		cmovz	eax, [ebp+0xC]
		mov		[ebp-0x1C], eax
		cvtsi2ss	xmm0, eax
		movd	[ebp-8], xmm0
		mov		eax, 0x11F32DC
		mov		edx, [eax]
		test	edx, edx
		jz		arrEmpty
		dec		edx
		mov		[eax], edx
		mov		ecx, [eax+edx*4-0x40]
		jmp		fillValues
	arrEmpty:
		push	0x14
		call	GameHeapAlloc
		mov		ecx, eax
		xor		edx, edx
		mov		[ecx+8], edx
		mov		[ecx+0xC], edx
	fillValues:
		mov		[ebp-0xC], ecx
		mov		edx, [ebp+8]
		mov		[ecx], edx
		mov		edx, [ebp-8]
		mov		[ecx+4], edx
		mov		edx, [ebp-0x1C]
		mov		[ecx+0x10], edx
		mov		dl, [ebp+0x10]
		cmp		dl, 3
		jz		doneStr
		mov		eax, [ebp+0xC]
		cmp		dl, 1
		jnz		notNumeric
		mov		[ecx+4], eax
		mov		eax, [ecx+8]
		test	eax, eax
		jz		doneStr
		xor		edx, edx
		mov		[ecx+8], edx
		mov		[ecx+0xC], edx
		push	eax
		call	GameHeapFree
		jmp		doneStr
	notNumeric:
		push	eax
		add		ecx, 8
		call	String::Set
	doneStr:
		mov		eax, [ebp-4]
		mov		ecx, [eax+0xC]
		xor		edx, edx
		mov		[ebp-0x10], edx
		mov		[ebp-0x14], edx
		mov		[ebp-0x18], edx
		test	ecx, ecx
		jz		jmpLbl_0
		mov		eax, [ecx+8]
		mov		[ebp-0x10], eax
		mov		ecx, [ecx+4]
		test	ecx, ecx
		jz		jmpLbl_0
		mov		eax, [ecx+8]
		mov		[ebp-0x14], eax
		mov		ecx, [ecx+4]
		test	ecx, ecx
		jz		jmpLbl_0
		mov		eax, [ecx+8]
		mov		[ebp-0x18], eax
		mov		ecx, [ecx+4]
	jmpLbl_0:
		mov		ecx, [ebp-4]
		mov		ecx, [ecx+4]
		mov		[ebp-0x1C], ecx
		cmp		dword ptr [ebp+8], 0xBBA
		jnz		jmpLbl_3
		mov		eax, [ebp-0x10]
		test	eax, eax
		jz		jmpLbl_2
		cmp		dword ptr [eax], 0
		jnz		jmpLbl_2
		cmp		dword ptr [eax+4], 0x4479C000
		jnz		jmpLbl_1
		cmp		dword ptr [ecx+0xC], 0
		jnz		jmpLbl_18
		push	dword ptr [ebp+0xC]
		call	kAddrSubA0AF10
		mov		ecx, [ebp-0x1C]
		mov		[ecx+0xC], eax
		test	eax, eax
		jnz		jmpLbl_18
		push	dword ptr [ebp+0xC]
		mov		ecx, [ebp-0x1C]
		call	kAddrSubA0AE70
		mov		ecx, [ebp-0x1C]
		mov		[ecx+0xC], eax
		jmp		jmpLbl_18
	jmpLbl_1:
		cvttss2si	edx, dword ptr [eax+4]
		cmp		edx, 0x385
		jl		jmpLbl_2
		cmp		edx, 0x38C
		jg		jmpLbl_2
		mov		dword ptr [eax], 6
		push	dword ptr [ebp+0xC]
		lea		ecx, [eax+8]
		call	String::Set
		mov		ecx, [ebp-0xC]
		call	FreeExtraString
		jmp		done
	jmpLbl_2:
		mov		eax, [ebp-0xC]
		mov		dword ptr [eax], 8
		mov		dword ptr [eax+0x10], 0xBBA
		jmp		jmpLbl_17
	jmpLbl_3:
		cmp		dword ptr [ebp+8], 1
		jnz		jmpLbl_11
		cmp		dword ptr [ebp-8], 0x4479C000
		jnz		jmpLbl_4
		mov		ecx, [ebp-0x1C]
		cmp		dword ptr [ecx+0xC], 0
		jz		jmpLbl_4
		mov		dword ptr [ecx+0xC], 0
		mov		ecx, [ebp-0xC]
		call	FreeExtraString
		jmp		done
	jmpLbl_4:
		mov		ecx, [ebp-0x10]
		test	ecx, ecx
		jz		jmpLbl_11
		cmp		dword ptr [ecx], 0xBB9
		jnz		jmpLbl_10
		mov		ecx, [ebp-0x14]
		test	ecx, ecx
		jz		jmpLbl_10
		mov		eax, [ecx]
		cmp		eax, 4
		jz		jmpLbl_5
		cmp		eax, 2
		jnz		jmpLbl_10
	jmpLbl_5:
		mov		edx, [ecx+4]
		cmp		edx, [ebp-8]
		jnz		jmpLbl_10
		mov		eax, [ebp-0xC]
		cmp		dword ptr [eax+0x10], 0x2710
		jge		jmpLbl_6
		cvttss2si	edx, dword ptr [eax+4]
		cmp		edx, 0xFA1
		jl		jmpLbl_7
		cmp		edx, 0x101D
		jg		jmpLbl_8
	jmpLbl_6:
		mov     dword ptr [ecx], 8
		jmp		jmpLbl_9
	jmpLbl_7:
		cmp		edx, 0x7D0
		jl		jmpLbl_8
		cmp		edx, 0x7E9
		jg		jmpLbl_8
		mov     dword ptr [ecx], 9
		jmp		jmpLbl_9
	jmpLbl_8:
		mov     dword ptr [ecx], 0xFFFFFFFF
	jmpLbl_9:
		cvttss2si	edx, dword ptr [ecx+4]
		mov		[ecx+0x10], edx
		mov		eax, [ebp-0x10]
		mov		edx, [eax+4]
		mov		[ecx+4], edx
		push	dword ptr [eax+8]
		add		ecx, 8
		call	String::Set
		jmp		jmpLbl_18
	jmpLbl_10:
		mov		ecx, [ebp-0x10]
		cmp		dword ptr [ecx], 0xBBC
		jnz		jmpLbl_11
		mov		eax, [ebp-0x14]
		test	eax, eax
		jz		jmpLbl_11
		cmp		dword ptr [eax], 0xBBB
		jnz		jmpLbl_11
		mov		ecx, [ebp-0x18]
		test	ecx, ecx
		jz		jmpLbl_11
		cmp		dword ptr [ecx], 2
		jnz		jmpLbl_11
		mov		edx, [ecx+4]
		cmp		edx, [ebp-8]
		jnz		jmpLbl_11
		mov		dword ptr [ecx], 0xA
		cvttss2si	edx, dword ptr [ecx+4]
		mov		[ecx+0x10], edx
		mov		eax, [ebp-0x10]
		mov		edx, [eax+4]
		mov		[ecx+4], edx
		mov		eax, [ebp-0x14]
		push	dword ptr [eax+8]
		add		ecx, 8
		call	String::Set
		mov		ecx, [ebp-4]
		add		ecx, 8
		call	kAddrSubAE8010
		mov		ecx, eax
		call	FreeExtraString
		jmp		jmpLbl_18
	jmpLbl_11:
		mov		ecx, [ebp-0xC]
		mov		eax, [ecx]
		cmp		eax, 1
		ja		jmpLbl_17
		mov		edx, [ecx+0x10]
		cmp		edx, 0x2710
		jge		jmpLbl_12
		cmp		edx, 0xFA1
		jl		jmpLbl_14
		cmp		edx, 0x101D
		jg		jmpLbl_16
	jmpLbl_12:
		test	eax, eax
		jnz		jmpLbl_13
		mov		dword ptr [ecx], 4
		jmp		jmpLbl_17
	jmpLbl_13:
		mov		dword ptr [ecx], 5
		jmp		jmpLbl_17
	jmpLbl_14:
		cmp		edx, 0x7D0
		jl		jmpLbl_16
		cmp		edx, 0x7E9
		jg		jmpLbl_16
		test	eax, eax
		jnz		jmpLbl_15
		mov		dword ptr [ecx], 2
		jmp		jmpLbl_17
	jmpLbl_15:
		cmp		dword ptr [ebp-0x10], 0
		jz		jmpLbl_17
		mov		dword ptr [ecx], 3
		jmp		jmpLbl_17
	jmpLbl_16:
		test	eax, eax
		jz		jmpLbl_17
		cvttss2si	edx, dword ptr [ecx+4]
		cmp		edx, 0x385
		jl		jmpLbl_17
		cmp		edx, 0x38C
		jg		jmpLbl_17
		mov		dword ptr [ecx], 7
	jmpLbl_17:
		mov		ecx, [ebp-4]
		add		ecx, 8
		mov		[ebp-0x1C], ecx
		call	kAddrSub43A010
		mov		ecx, [ebp-0xC]
		mov		[eax+8], ecx
		mov		ecx, [ebp-0x1C]
		mov		dword ptr [eax], 0
		mov		edx, [ecx+4]
		mov		[eax+4], edx
		mov		edx, [ecx+4]
		test	edx, edx
		cmovz	edx, ecx
		mov		[edx], eax
		mov		[ecx+4], eax
		inc		dword ptr [ecx+8]
		jmp		done
	jmpLbl_18:
		mov		ecx, [ebp-4]
		add		ecx, 8
		call	kAddrSubAE8010
		mov		ecx, eax
		call	FreeExtraString
		mov		ecx, [ebp-0xC]
		call	FreeExtraString
	done:
		mov		esp, ebp
		pop		ebp
		retn	0xC
	}
}

__declspec(naked) NiTMap::Entry* __stdcall AddTraitName(const char *name)
{
	__asm
	{
		mov		eax, [esp+4]
		cmp		[eax], '_'
		jz		isVar
		mov		ecx, 0x11F32F4
		jmp		NiTMap::Lookup
	isVar:
		push	ebp
		mov		ebp, esp
		sub		esp, 8
		lea		edx, [ebp-4]
		push	edx
		push	eax
		mov		ecx, 0x11F32F4
		call	NiTMap::Insert
		test	al, al
		jz		done
		mov		eax, 0x11A6D74
		mov		edx, [eax]
		inc		dword ptr [eax]
		mov		eax, [ebp-4]
		mov		[eax+8], edx
		mov		dl, '_'
		mov		ecx, [ebp+8]
		inc		ecx
		call	FindChrR
		test	eax, eax
		jz		done
		cmp		eax, ecx
		jz		done
		lea		ecx, [eax+1]
		mov		eax, 0xFFFFFFFF
		cmp		[ecx], 0
		jz		doneIdx
		call	StrToInt
	doneIdx:
		push	eax
		lea		eax, [ebp-8]
		push	eax
		mov		eax, [ebp-4]
		push	dword ptr [eax+8]
		mov		ecx, 0x11F3308
		call	NiTMap::Insert
		pop		eax
		mov		ecx, [ebp-8]
		mov		[ecx+8], eax
	done:
		mov		eax, [ebp-4]
		mov		esp, ebp
		pop		ebp
		retn	4
	}
}

__declspec(naked) char __stdcall GetGMSTValue(char *result)
{
	__asm
	{
		push	esi
		mov		esi, ecx
	findSmCol:
		mov		dl, [ecx]
		test	dl, dl
		jz		noSmCol
		cmp		dl, ';'
		jz		gotSmCol
		inc		ecx
		jmp		findSmCol
	gotSmCol:
		mov		[ecx], 0
	noSmCol:
		push	esi
		mov		ecx, g_gameSettingsMap
		call	NiTMap::Lookup
		test	eax, eax
		jz		done
		mov		eax, [eax+8]
		test	eax, eax
		jz		done
		mov		ecx, [esp+8]
		mov		dl, [esi]
		or		dl, 0x20
		cmp		dl, 's'
		jnz		notStr
		mov		eax, [eax+4]
		mov		[ecx], eax
		mov		al, 2
		jmp		done
	notStr:
		cmp		dl, 'f'
		jnz		notFlt
		mov		eax, [eax+4]
		mov		[ecx], eax
		mov		al, 1
		jmp		done
	notFlt:
		cmp		dl, 'i'
		setz	al
		jnz		done
		cvtsi2ss	xmm0, [eax+4]
		movd	[ecx], xmm0
	done:
		pop		esi
		retn	4
	}
}

__declspec(naked) void* __cdecl ResolveXMLFileHook(const char *filePath, char *inData, UInt32 inLength)
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		sub		esp, 0x1C
		push	esi
		push	edi
		push	0x14
		call	GameHeapAlloc
		mov		[ebp-4], eax
		mov		ecx, eax
		call	kAddrSubA0AC80
		push	dword ptr [ebp+8]
		mov		ecx, offset s_cachedParsedData
		call	UnorderedMap<const char*, char*>::GetPtr
		mov		edi, eax
		test	eax, eax
		jz		notCached
		mov		eax, [eax]
		test	eax, eax
		jz		notCached
		mov		edi, [eax]
		test	edi, edi
		jz		done
		lea		esi, [eax+4]
	cacheIter:
		movzx	eax, byte ptr [esi+1]
		cmp		al, 3
		jnz		notID
		movzx	ecx, word ptr [esi+4]
		jmp		doneType
	notID:
		cmp		al, 1
		jnz		notFlt
		mov		ecx, [esi+4]
		jmp		doneType
	notFlt:
		lea		ecx, [esi+4]
	doneType:
		push	eax
		push	ecx
		movzx	eax, word ptr [esi+2]
		push	eax
		mov		ecx, [ebp-4]
		call	ResolveTrait
		movzx	eax, byte ptr [esi]
		add		esi, eax
		dec		edi
		jnz		cacheIter
		jmp		done
	notCached:
		push	0
		push	dword ptr [ebp+8]
		call	GetXMLFileDataHook
		add		esp, 8
		mov		[ebp-8], eax
		cmp		dword ptr [eax+4], 0
		jz		skipClose
		mov		esi, eax
		push	eax
		call	ResolveIncludes
		pop		ecx
		cmp		s_debugMode, 0
		jz		notDebug
		push	esi
		push	dword ptr [ebp+8]
		call	InitDebugLog
	notDebug:
		test	edi, edi
		jz		skipCache
		mov		eax, [esi]
		and		eax, 0xFFFFFFF0
		push	eax
		call	malloc
		pop		ecx
		test	eax, eax
		cmovz	edi, eax
		jz		skipCache
		mov		[edi], eax
		mov		dword ptr [eax], 0
		mov		[ebp-0xC], eax
		lea		edi, [eax+4]
	skipCache:
		mov		esi, [esi+4]
		pxor	xmm0, xmm0
		movdqu	xmmword ptr [ebp-0x1C], xmm0
		jmp		getChar
	mainIter:
		inc		esi
	getChar:
		mov		al, [esi]
		test	al, al
		jz		mainEnd
		cmp		al, '<'
		jnz		notOpen
		cmp		[ebp-0xD], 0
		setnz	dl
		mov		[ebp-0x10], dl
		jnz		doneOpen
		mov		[ebp-0xD], 1
		mov		ecx, [ebp-0x14]
		test	ecx, ecx
		jz		doneOpen
		mov		[esi], 0
		mov		[ebp-0x17], 2
		cmp		[ecx], '&'
		jnz		notTag
		cmp		[ecx+1], '-'
		jnz		numTag
		lea		eax, [ebp-0x14]
		push	eax
		add		ecx, 2
		call	GetGMSTValue
		test	al, al
		jz		doneOpen
		mov		[ebp-0x17], al
		jmp		doneValue
	numTag:
		push	ecx
		mov		ecx, 0x11F32F4
		call	NiTMap::Lookup
		test	eax, eax
		jz		doneValue
		cvtsi2ss	xmm0, [eax+8]
		jmp		isNum
	notTag:
		mov		dl, [ecx]
		cmp		dl, ' '
		jb		doneOpen
		mov		eax, ecx
	charIter:
		cmp		dl, '9'
		ja		doneValue
		cmp		dl, '-'
		jb		doneValue
		cmp		dl, '/'
		jz		doneValue
		inc		eax
		mov		dl, [eax]
		test	dl, dl
		jnz		charIter
		call	StrToDbl
		cvtsd2ss	xmm0, xmm0
	isNum:
		mov		[ebp-0x17], 1
		movss	[ebp-0x14], xmm0
	doneValue:
		movzx	eax, byte ptr [ebp-0x17]
		push	eax
		mov		edx, [ebp-0x14]
		push	edx
		push	0xBB9
		test	edi, edi
		jz		resValue
		mov		[edi+1], al
		mov		word ptr [edi+2], 0xBB9
		lea		ecx, [edi+4]
		cmp		al, 2
		jz		cpyStrVal1
		mov		[ecx], edx
		add		ecx, 4
		jmp		writeVal1
	cpyStrVal1:
		push	0xFA
		call	StrNCopy
		lea		ecx, [eax+1]
	writeVal1:
		mov		eax, ecx
		sub		eax, edi
		mov		[edi], al
		mov		edi, ecx
		mov		eax, [ebp-0xC]
		inc		dword ptr [eax]
	resValue:
		mov		ecx, [ebp-4]
		call	ResolveTrait
	doneOpen:
		mov		dword ptr [ebp-0x14], 0
		jmp		mainIter
	notOpen:
		cmp		al, '>'
		jnz		notClose
		cmp		[ebp-0xD], 0
		jz		doneOpen
		cmp		[ebp-0xF], 0
		jnz		doneClose
		mov		ecx, [ebp-0x14]
		test	ecx, ecx
		jz		doneClose
		mov		[esi], 0
		push	ecx
		call	AddTraitName
		test	eax, eax
		jz		invID0
		mov		eax, [eax+8]
		movzx	ecx, byte ptr [ebp-0xE]
		push	3
		push	eax
		push	ecx
		test	edi, edi
		jz		resTrait
		mov		[edi], 6
		mov		[edi+1], 3
		mov		[edi+2], cx
		mov		[edi+4], ax
		add		edi, 6
		mov		edx, [ebp-0xC]
		inc		dword ptr [edx]
	resTrait:
		mov		ecx, [ebp-4]
		call	ResolveTrait
		jmp		doneClose
	invID0:
		cmp		s_debugPrint, 0
		jz		doneClose
		mov		[ebp-0x16], 1
		mov		ecx, [ebp-0x14]
		call	PrintErrorInvID
	doneClose:
		mov		dword ptr [ebp-0x10], 0
		mov		dword ptr [ebp-0x14], 0
		mov		[ebp-0x15], 0
		jmp		mainIter
	notClose:
		cmp		[ebp-0xD], 0
		jz		isClosed
		cmp		al, '/'
		jnz		notSlash
		mov		[ebp-0xE], 1
		mov		[ebp-0xF], 0
		mov		[esi], 0
		mov		[ebp-0x15], 0
		jmp		mainIter
	notSlash:
		cmp		[ebp-0x10], 0
		jnz		mainIter
		cmp		al, ' '
		jnz		notSpace
		mov		[ebp-0x15], 1
		mov		[esi], 0
		jmp		mainIter
	notSpace:
		cmp		dword ptr [ebp-0x14], 0
		jnz		procAction
		mov		[ebp-0x14], esi
		mov		[ebp-0x15], 0
		jmp		mainIter
	procAction:
		cmp		[ebp-0x15], 0
		jz		mainIter
		mov		[ebp-0x15], 0
		mov		[ebp-0xF], 1
		push	dword ptr [ebp-0x14]
		mov		ecx, 0x11F32F4
		call	NiTMap::Lookup
		test	eax, eax
		jnz		validID0
		mov		ecx, [ebp-0x14]
		mov		dword ptr [ebp-0x14], 0
		jmp		logErrorID
	validID0:
		mov		eax, [eax+8]
		push	3
		push	eax
		push	0
		test	edi, edi
		jz		resName
		mov		[edi], 6
		mov		[edi+1], 3
		mov		word ptr [edi+2], 0
		mov		[edi+4], ax
		add		edi, 6
		mov		edx, [ebp-0xC]
		inc		dword ptr [edx]
	resName:
		mov		ecx, [ebp-4]
		call	ResolveTrait
		mov		ecx, [esi]
		or		ecx, 0x20202020
		cmp		ecx, '=crs'
		jnz		notSrc
		mov		ecx, 0xBBB
		add		esi, 3
		jmp		findDQ0
	notSrc:
		cmp		ecx, 'eman'
		jnz		onError0
		mov		ecx, 0xBBA
		add		esi, 4
		jmp		verifyEQ
	needTrait:
		mov		ecx, [esi+2]
		or		ecx, 0x20202020
		cmp		ecx, 'iart'
		jnz		onError1
		mov		ecx, 0xBBC
		add		esi, 7
	verifyEQ:
		cmp		[esi], '='
		jnz		onError2
	findDQ0:
		inc		esi
		mov		dl, [esi]
		test	dl, dl
		jz		mainEnd
		cmp		dl, '\"'
		jnz		findDQ0
		lea		eax, [esi+1]
	findDQ1:
		inc		esi
		mov		dl, [esi]
		test	dl, dl
		jz		mainEnd
		cmp		dl, '\"'
		jnz		findDQ1
		cmp		eax, esi
		jz		onError3
		mov		[esi], 0
		mov		[ebp-0x1C], ecx
		mov		edx, 2
		cmp		cx, 0xBBC
		jnz		notTrait
		push	eax
		push	eax
		call	AddTraitName
		pop		ecx
		test	eax, eax
		jnz		validID1
	logErrorID:
		mov		[ebp-0x10], 1
		cmp		s_debugPrint, 0
		jz		mainIter
		mov		[ebp-0x16], 1
		call	PrintErrorInvID
		jmp		mainIter
	validID1:
		mov		eax, [eax+8]
		mov		ecx, 0xBBC
		mov		edx, 3
	notTrait:
		push	edx
		push	eax
		push	ecx
		test	edi, edi
		jz		resOper
		mov		[edi+1], dl
		mov		[edi+2], cx
		lea		ecx, [edi+4]
		cmp		dl, 2
		jz		cpyStrVal2
		mov		[ecx], ax
		add		ecx, 2
		jmp		writeVal2
	cpyStrVal2:
		mov		dl, [eax]
		mov		[ecx], dl
		inc		eax
		inc		ecx
		test	dl, dl
		jnz		cpyStrVal2
	writeVal2:
		mov		eax, ecx
		sub		eax, edi
		mov		[edi], al
		mov		edi, ecx
		mov		eax, [ebp-0xC]
		inc		dword ptr [eax]
	resOper:
		mov		ecx, [ebp-4]
		call	ResolveTrait
		cmp		dword ptr [ebp-0x1C], 0xBBB
		jnz		mainIter
		cmp		[esi+1], ' '
		jz		needTrait
	onError1:
		mov		dl, 1
		jmp		logError
	onError0:
		xor		dl, dl
		jmp		logError
	onError2:
		mov		dl, 2
		jmp		logError
	onError3:
		mov		dl, 3
	logError:
		mov		[ebp-0x10], 1
		cmp		s_debugPrint, 0
		jz		mainIter
		mov		[ebp-0x16], 1
		movzx	edx, dl
		movzx	eax, kErrorLengths[edx]
		push	eax
		mov		eax, kErrorStrings[edx*4]
		push	eax
		mov		ecx, offset s_log
		call	FileStream::WriteBuf
		jmp		mainIter
	isClosed:
		cmp		dword ptr [ebp-0x14], 0
		jnz		mainIter
		mov		[ebp-0x14], esi
		jmp		mainIter
	mainEnd:
		cmp		s_debugPrint, 0
		jz		skipClose
		mov		s_debugPrint, 0
		mov		ecx, offset s_log
		mov		esi, ecx
		call	FileStream::Close
		cmp		[ebp-0x16], 0
		jz		skipClose
		push	s_logPath
		mov		ecx, esi
		call	FileStream::OpenWrite
		test	al, al
		jz		skipClose
		push	0x26
		push	offset kResolveError
		mov		ecx, esi
		call	FileStream::WriteBuf
		mov		ecx, [ebp+8]
		call	StrLen
		push	eax
		push	ecx
		mov		ecx, esi
		call	FileStream::WriteBuf
		mov		ecx, esi
		call	FileStream::Close
	skipClose:
		mov		eax, [ebp-8]
		push	eax
		push	dword ptr [eax+4]
		call	GameHeapFree
		call	GameHeapFree
	done:
		mov		eax, [ebp-4]
		pop		edi
		pop		esi
		mov		esp, ebp
		pop		ebp
		retn
	}
}

typedef UnorderedSet<char*> IncludePaths;
typedef UnorderedMap<char*, IncludePaths> IncludeRefs;

struct TileName
{
	char		*name;

	TileName(char *inName) : name(CopyString(inName)) {}
	~TileName() {free(name);}
};

char kMenuNames[] =
"BarterMenu\0BlackJackMenu\0CaravanMenu\0CharGenMenu\0CompanionWheelMenu\0ComputersMenu\0ContainerMenu\0DialogMenu\0HackingMenu\0\
LevelUpMenu\0LoadingMenu\0LockPickMenu\0LoveTesterMenu\0MessageMenu\0QuantityMenu\0RaceSexMenu\0RecipeMenu\0RepairServicesMenu\0\
RouletteMenu\0SleepWaitMenu\0SlotMachineMenu\0SpecialBookMenu\0StartMenu\0TraitMenu\0TraitSelectMenu\0TutorialMenu\0VATSMenu\0\
HUDMainMenu\0InventoryMenu\0ItemModMenu\0MapMenu\0RepairMenu\0StatsMenu";
const UInt32 kMenuFiles[] =
{
	0x10707B4, 0x1070C38, 0x107170C, 0x1071C5C, 0x1071DFC, 0x10720E8, 0x1072258, 0x107266C, 0x1072990, 0x1073D9C, 0x1073F04,
	0x10744A0, 0x10746DC, 0x1075724, 0x10758C4, 0x10759BC, 0x107054C, 0x1075E9C, 0x1076254, 0x1076468, 0x1076670, 0x1076874,
	0x1076D8C, 0x1077A5C, 0x1077B04, 0x1077BA4, 0x1077D50, 0x107343C, 0x1073A70, 0x1073C34, 0x1074DF8, 0x1075D14, 0x107730C
};
char kTileTypes[] = "menu\0rect\0image\0text\0hotrect\0nif\0radial\0template";

void UIOLoad()
{
	UnorderedMap<char*, const char*> menuNameToFile(0x40);
	char *bufferPtr = kMenuNames;
	UInt32 length = 0;
	do
	{
		menuNameToFile[bufferPtr] = (const char*)kMenuFiles[length];
		bufferPtr += StrLen(bufferPtr) + 1;
	}
	while (++length < 33);

	char prefabsPath[0x80];
	memcpy(prefabsPath, "Data\\menus\\prefabs\\duinvsettings.xml", 37);
	bool darnUI = FileExists(prefabsPath);
	if (darnUI) s_log.WriteBuf("* Darnified UI detected.\n\n", 26);

	DataHandler *dataHandler = *(DataHandler**)0x11C3F2C;
	UnorderedMap<char*, IncludeRefs> injectLists;
	char uioPath[0x50], menusPath[0x80], *readBuffer = (char*)malloc(0x2000), *menuFile, *tileName, *pathStr, *delim;
	bool condLine, skipCond, evalRes, skipExpr, isOper, operAND, not, tempRes;
	memcpy(uioPath, "Data\\uio\\public\\*.txt", 22);
	DirectoryIterator pubIter(uioPath);
	uioPath[9] = 0;
	memcpy(menusPath, "Data\\menus\\", 12);

	s_log.WriteBuf("--------------------------------\n*** Checking supported files ***\n--------------------------------\n", 99);

	do
	{
		if (!uioPath[9])
			memcpy(uioPath + 9, "supported.txt", 14);
		else
		{
			if (pubIter.End()) break;
			StrCopy(StrLenCopy(uioPath + 9, "public\\", 7), pubIter.Get());
			pubIter.Next();
		}

		PrintLog("\n>>>>> Processing file '%s'", uioPath + 9);
		LineIterator lineIter(uioPath, readBuffer);

		if (lineIter.End())
		{
			s_log.WriteBuf("\tERROR: Could not open file > Skipping.\n", 40);
			continue;
		}

		condLine = false;
		do
		{
			bufferPtr = lineIter.Get();
			lineIter.Next();
			if (!condLine)
			{
				menuFile = GetNextToken(bufferPtr, ':');
				if (!*menuFile) continue;
				skipCond = condLine = true;
				PrintLog("\t> Checking '%s'", bufferPtr);
				StrCopy(prefabsPath + 19, bufferPtr);
				if (!FileExists(prefabsPath))
				{
					s_log.WriteBuf("\t\t! File not found > Skipping.\n", 31);
					continue;
				}
				tileName = GetNextToken(menuFile, ':');
				if (*menuFile == '@')
				{
					if (!*tileName)
					{
						s_log.WriteBuf("\t\t! Missing target element > Skipping.\n", 39);
						continue;
					}
					StrCopy(menusPath + 11, menuFile + 1);
					menuFile = menusPath;
				}
				else
				{
					if (!*tileName) tileName = menuFile;
					menuFile = const_cast<char*>(menuNameToFile.Get(menuFile));
					if (!menuFile)
					{
						s_log.WriteBuf("\t\t! Invalid target menu > Skipping.\n", 36);
						continue;
					}
				}
				pathStr = bufferPtr;
				skipCond = false;
			}
			else
			{
				condLine = false;
				if (skipCond) continue;
				evalRes = isOper = operAND = skipExpr = false;
				do
				{
					delim = GetNextToken(bufferPtr, "\t ");
					if (isOper)
					{
						if (*(UInt16*)bufferPtr == '&&')
						{
							operAND = true;
							if (!evalRes) skipExpr = true;
						}
						else if (*(UInt16*)bufferPtr == '||')
						{
							operAND = false;
							if (evalRes) skipExpr = true;
						}
						else break;
					}
					else if (!skipExpr)
					{
						if (not = *bufferPtr == '!')
							bufferPtr++;
						if (StrEqualCI(bufferPtr, "true"))
							tempRes = true;
						else if (*bufferPtr == '#')
						{
							StrCopy(prefabsPath + 19, bufferPtr + 1);
							tempRes = FileExists(prefabsPath);
						}
						else if (*bufferPtr == '?')
							tempRes = dataHandler->IsModLoaded(bufferPtr + 1);
						else if (StrEqualCI(bufferPtr, "darnui"))
							tempRes = darnUI;
						else break;

						if (not) tempRes = !tempRes;
						if (operAND)
							evalRes &= tempRes;
						else evalRes |= tempRes;
					}
					else skipExpr = false;

					isOper = !isOper;
					bufferPtr = delim;
				}
				while (*bufferPtr);

				if (!evalRes)
					s_log.WriteBuf("\t\t! Conditions evaluated as FALSE > Skipping.\n", 46);
				else if (injectLists[menuFile][tileName].Insert(pathStr))
					PrintLog("\t\t+ Adding to '%s' @ %s", menuFile + 11, tileName);
				else s_log.WriteBuf("\t\t! File already added > Skipping.\n", 35);
			}
		}
		while (!lineIter.End());
	}
	while (true);

	free(readBuffer);

	g_gameSettingsMap = (NiTMap*)(*(UInt8**)0x11C8048 + 0x10C);

	length = 0;
	do
	{
		s_cachedParsedData[(const char*)kMenuFiles[length]];
	}
	while (++length < 27);
	s_cachedParsedData["Data\\NVSE\\plugins\\textinput\\texteditmenu.xml"];

	s_log.WriteBuf("\n---------------------------\n*** Patching menu files ***\n---------------------------\n", 85);

	FileStream cacheFile;
	if (!cacheFile.Create(kTempCacheFile, FILE_ATTRIBUTE_HIDDEN))
	{
		s_log.WriteBuf("\nERROR: Could not create cache file > Aborting.\n", 48);
		return;
	}

	if (injectLists.Empty())
	{
		s_log.WriteBuf("\nNo files requiring patching > Aborting.\n", 41);
		return;
	}

	UnorderedSet<const char*> tileTypes;
	bufferPtr = kTileTypes;
	length = 0;
	do
	{
		tileTypes.Insert(bufferPtr);
		bufferPtr += StrLen(bufferPtr) + 1;
	}
	while (++length < 8);

	UInt32 lastOffset = 0, currOffset;
	XMLFileData *fileData, *inclData;
	char *bgnPtr, *endPtr, srchType;
	Vector<TileName> openTiles(10);

	for (auto menuIter = injectLists.Begin(); menuIter; ++menuIter)
	{
		PrintLog("\n>>>>> Processing file '%s'", menuIter.Key() + 11);

		fileData = GetXMLFileData(menuIter.Key(), 0);
		if (!fileData)
		{
			s_log.WriteBuf("\tERROR: Could not open file > Skipping.\n", 40);
			continue;
		}

		bufferPtr = fileData->data;
		while ((bgnPtr = FindChr(bufferPtr, '<')) && (endPtr = FindChr(++bgnPtr, '>')))
		{
			length = endPtr - bgnPtr;
			if (length >= 4)
			{
				if (*bgnPtr == '/')
				{
					*endPtr = 0;
					if (!openTiles.Empty() && tileTypes.HasKey(bgnPtr + 1))
					{
						tileName = openTiles.Top().name;
						auto findTile = menuIter().Find(tileName);
						if (findTile)
						{
							for (auto inclIter = findTile().Begin(); inclIter; ++inclIter)
							{
								StrCopy(prefabsPath + 19, *inclIter);
								inclData = GetXMLFileData(prefabsPath, 0);
								if (!inclData)
								{
									PrintLog("\t! ERROR: Could not open '%s' > Skipping.", *inclIter);
									continue;
								}
								PrintLog("\t+ Injecting '%s' @ %s", *inclIter, tileName);
								cacheFile.WriteBuf(inclData->data, inclData->length);
								inclData->Destroy();
							}
							findTile.Remove();
						}
						openTiles.Pop();
					}
					*endPtr = '>';

					if (menuIter().Empty())
					{
						cacheFile.WriteBuf(bufferPtr, fileData->length - (bufferPtr - fileData->data));
						break;
					}
				}
				else if (length >= 12)
				{
					if (delim = SubStr(bgnPtr + 4, "name=", length - 4))
					{
						srchType = 1;
						delim += 5;
					}
					else if ((length >= 20) && !openTiles.Empty() && StrBeginsCI(bgnPtr, "include src"))
					{
						srchType = 2;
						delim = bgnPtr + 12;
					}
					else srchType = 0;
					if (srchType && (bgnPtr = FindChr(delim, '\"')) && (++bgnPtr < endPtr) && (delim = FindChr(bgnPtr, '\"')) && (delim < endPtr) && (delim > bgnPtr))
					{
						*delim = 0;
						if (srchType == 1)
							openTiles.Append(bgnPtr);
						else
						{
							auto findTile = menuIter().Find(openTiles.Top().name);
							if ((findTile && findTile().HasKey(bgnPtr)) || StrBeginsCI(bgnPtr, "includes_"))
							{
								PrintLog("\t- Removing duplicate reference '%s'", bgnPtr);
								bufferPtr = endPtr + 1;
								continue;
							}
						}
						*delim = '\"';
					}
				}
			}

			endPtr++;
			cacheFile.WriteBuf(bufferPtr, endPtr - bufferPtr);
			bufferPtr = endPtr;
		}

		fileData->Destroy();
		openTiles.Clear();

		currOffset = cacheFile.GetOffset();
		s_cachedBaseFiles.Emplace(menuIter.Key(), lastOffset, currOffset - lastOffset);
		lastOffset = currOffset;

		for (auto notFound = menuIter().Begin(); notFound; ++notFound)
			PrintLog("\t! Menu element '%s' not found > Skipping.", notFound.Key());
	}
}

class InterfaceManager;
__declspec(naked) InterfaceManager* __fastcall InitInterfaceMngrHook(InterfaceManager *interfaceMgr)
{
	__asm
	{
		call	kInitInterfaceMngrAddr
		push	eax
		call	UIOLoad
		mov		ecx, offset s_log
		call	FileStream::Close
		pop		eax
		retn
	}
}

struct PluginInfo
{
	enum
	{
		kInfoVersion = 1
	};

	UInt32			infoVersion;
	const char		*name;
	UInt32			version;
};

struct NVSEInterface
{
	struct CommandInfo;

	UInt32			nvseVersion;
	UInt32			runtimeVersion;
	UInt32			editorVersion;
	UInt32			isEditor;
	bool			(*RegisterCommand)(CommandInfo *info);
	void			(*SetOpcodeBase)(UInt32 opcode);
	void*			(*QueryInterface)(UInt32 id);
	UInt32			(*GetPluginHandle)(void);
	bool			(*RegisterTypedCommand)(CommandInfo *info, UInt8 retnType);
	const char* 	(*GetRuntimeDirectory)(void);
	UInt32			isNogore;
};

bool NVSEPlugin_Query(const NVSEInterface *nvse, PluginInfo *info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "UI Organizer Plugin";
	info->version = 210;
	return !nvse->isEditor;
}

bool NVSEPlugin_Load(const NVSEInterface *nvse)
{
	s_log.Create(s_logPath);
	PrintLog("UI Organizer v2.10\nRuntime version = %08X\nNVSE version = %08X\n", nvse->runtimeVersion, nvse->nvseVersion);

	if (GetPrivateProfileIntA("General", "bDebugMode", 0, "Data\\uio\\settings.ini"))
	{
		s_debugMode = true;
		s_logPath = "uio_debug\\uio_debug.log";
		PrintLog("Debug mode enabled > Log moved to %s", s_logPath);
		s_log.Close();
		CreateDirectoryA(s_debugPath, NULL);
		s_log.Create(s_logPath);
	}

	bool isNogore = nvse->isNogore != 0;
	if (isNogore)
	{
		kHeapAllocAddr = 0xAA3EF0;
		kHeapFreeAddr = 0xAA4110;
		kInitInterfaceMngrAddr = 0x70A080;
		GetXMLFileData = (_GetXMLFileData)0xA1CA30;
		ResolveIncludes = (_ResolveIncludes)0xA02C10;
		kAddrSub43A010 = 0x439EC0;
		kAddrSubA0AC80 = 0xA0AB50;
		kAddrSubA0AE70 = 0xA0AD40;
		kAddrSubA0AF10 = 0xA0ADE0;
		kAddrSubAE8010 = 0xAE84A0;
	}

	WriteRelCall(isNogore ? 0x709F85 : 0x70A035, InitInterfaceMngrHook);
	WriteRelCall(isNogore ? 0xA01A57 : 0xA01B87, ResolveXMLFileHook);
	WriteRelCall(isNogore ? 0xA02E14 : 0xA02F44, GetXMLFileDataHook);
	return true;
}