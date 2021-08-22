#pragma once

#include "utility.h"
#include "containers.h"
#include "gameapi.h"

struct XMLFileData
{
	UInt32		length;
	char		*data;

	__forceinline static XMLFileData *Get(const char *filePath)
	{
		return CdeclCall<XMLFileData*>(ADDR_GetXMLFileData, filePath, 0);
	}

	void Destroy()
	{
		GameHeapFree(data);
		GameHeapFree(this);
	}
};

#define XML_CACHE_BASE 0x80000UL
#define XML_CACHE_GROW 0x40000UL

struct XMLCache
{
	char		*cache;
	UInt32		allocated;
	UInt32		length;

	XMLCache(UInt32 _alloc = XML_CACHE_BASE) : allocated(_alloc), length(0) {cache = (char*)malloc(allocated);}

	void WriteInclude(const char *filePath)
	{
		if ((allocated - length) < 0x80)
		{
			allocated += XML_CACHE_GROW;
			cache = (char*)realloc(cache, allocated);
		}
		char *endPtr = StrCopy((char*)memcpy(cache + length, "<include src=\"", 14) + 14, filePath);
		*(UInt32*)endPtr = '>/\"';
		length = endPtr - cache + 3;
	}

	void __fastcall Write(void *dataPtr, UInt32 size)
	{
		UInt32 end = length;
		length += size;
		if (length > allocated)
		{
			do
			{
				allocated += XML_CACHE_GROW;
			}
			while (length > allocated);
			cache = (char*)realloc(cache, allocated);
		}
		memcpy(cache + end, dataPtr, size);
	}

	void Read(void *dataPtr, UInt32 begin, UInt32 size)
	{
		memcpy(dataPtr, cache + begin, size);
	}
}
s_baseFilesCache;

struct CacheOffset
{
	UInt32		length;
	union
	{
		char	*buffer;
		UInt32	begin;
	};
	bool		mainCache;
	bool		resolved;

	CacheOffset() : length(0), begin(0), mainCache(true), resolved(true) {}
	CacheOffset(bool _main) : mainCache(_main), resolved(false)
	{
		length = s_baseFilesCache.length;
		buffer = (char*)memcpy(GameHeapAlloc(length + 1), s_baseFilesCache.cache, length);
		buffer[length] = 0;
	}
};

bool s_debugMode = false;

struct ParsedData
{
	XMLtoTileData	*data;
	bool			cached;
	bool			debugLog;

	ParsedData() : data(nullptr), cached(true), debugLog(s_debugMode) {}
	ParsedData(bool _cached) : data(nullptr), cached(_cached), debugLog(s_debugMode) {}
	ParsedData(bool _cached, bool _debug) : data(nullptr), cached(_cached), debugLog(_debug) {}
};

FileStream s_log, s_debugLog;
UnorderedMap<const char*, CacheOffset> s_cachedBaseFiles(0x80);
UnorderedMap<const char*, ParsedData> s_cachedParsedData(0x20);
char s_debugPath[0x80] = "uio_debug\\";
const char *s_logPath = "ui_organizer.log",
kTempXMLFile[] = "jip_temp.xml",
*kErrorStrings[] =
{
	"<!>\t\tExpected: 'name=' or 'src='\n",
	"<!>\t\tExpected: 'trait='\n",
	"<!>\t\tExpected: '='\n",
	"<!>\t\tEmpty attribute name\n"
},
kErrorInvID[] = "<!>\t\tUnknown tag name: '%s'\n",
kResolveError[] = "\n<!> Errors encountered while loading %s";

NiTMap *g_gameSettingsMap = NULL;

FILE* __stdcall InitDebugLog(const char *filePath, XMLFileData *fileData)
{
	const char *fileName = FindChrR(filePath, '\\');
	if (fileName) fileName++;
	else fileName = filePath;
	char *pathEnd = StrCopy(s_debugPath + 10, fileName);
	if (!s_debugLog.OpenWrite(s_debugPath))
		return nullptr;
	s_debugLog.WriteBuf(fileData->data, fileData->length);
	s_debugLog.Close();
	memcpy(pathEnd, ".log", 5);
	s_debugLog.OpenWrite(s_debugPath);
	return s_debugLog;
}

void __fastcall PrintTraitEntry(UInt32 *args)
{
	switch (args[2])
	{
		case 3:
			fprintf(s_debugLog, "%04X\t#%04X\n", args[0], args[1]);
			break;
		case 1:
			fprintf(s_debugLog, "%04X\t%.4f\n", args[0], *(float*)(args + 1));
			break;
		default:
			fprintf(s_debugLog, "%04X\t\"%s\"\n", args[0], (char*)args[1]);
	}
}

XMLFileData *s_externFileData = nullptr;

__declspec(naked) Tile* __stdcall UIOInjectComponent(Tile *parentTile, const char *dataStr)
{
	__asm
	{
		push	dword ptr [esp+8]
		push	offset kTempXMLFile
		CALL_EAX(ADDR_GetXMLFileData)
		mov		s_externFileData, eax
		test	eax, eax
		jz		done
		cmp		dword ptr [eax], 0
		jz		done
		push	eax
		CALL_EAX(ADDR_ResolveXMLIncludes)
		pop		ecx
		mov		ecx, [esp+0xC]
		CALL_EAX(ADDR_CreateTileFromXML)
		pop		ecx
		retn	8
	done:
		add		esp, 8
		retn	8
	}
}

__declspec(naked) XMLFileData* __cdecl GetXMLFileDataHook(const char *filePath, UInt32 arg2)
{
	__asm
	{
		push	ebx
		push	esi
		push	edi
		xor		esi, esi
		xor		edi, edi
		push	esi
		push	esp
		push	dword ptr [esp+0x18]
		mov		ecx, offset s_cachedBaseFiles
		call	UnorderedMap<const char*, CacheOffset>::Insert
		pop		ebx
		test	al, al
		jnz		notCached
		mov		esi, [ebx]
		test	esi, esi
		cmovz	ebx, esi
		jz		notCached
		cmp		[ebx+9], 0
		jnz		resolved
		mov		[ebx+9], 1
		push	ebx
		CALL_EAX(ADDR_ResolveXMLIncludes)
		pop		ecx
		mov		esi, [ebx]
		cmp		[ebx+8], 0
		jz		notMain
		mov		edi, [ebx+4]
		mov		ecx, offset s_baseFilesCache
		mov		eax, [ecx+8]
		mov		[ebx+4], eax
		push	esi
		mov		edx, edi
		call	XMLCache::Write
		jmp		createData
		ALIGN 16
	resolved:
		cmp		[ebx+8], 0
		jz		notMain
		push	esi
		mov		edx, s_baseFilesCache.cache
		add		edx, [ebx+4]
		push	edx
		lea		edx, [esi+1]
		push	edx
		GAME_HEAP_ALLOC
		push	eax
		call	_memcpy
		add		esp, 0xC
		mov		edi, eax
		mov		byte ptr [edi+esi], 0
		jmp		createData
		ALIGN 16
	notMain:
		mov		edi, [ebx+4]
		mov		dword ptr [ebx], 0
		ALIGN 16
	createData:
		push	8
		GAME_HEAP_ALLOC
		mov		[eax], esi
		mov		[eax+4], edi
		pop		edi
		pop		esi
		pop		ebx
		retn
		ALIGN 16
	notCached:
		push	0
		push	dword ptr [esp+0x14]
		CALL_EAX(ADDR_GetXMLFileData)
		add		esp, 8
		test	eax, eax
		jz		createData
		push	eax
		CALL_EAX(ADDR_ResolveXMLIncludes)
		pop		eax
		test	ebx, ebx
		jz		done
		mov		edi, [eax]
		test	edi, edi
		jz		done
		mov		edx, [eax+4]
		cmp		dword ptr [edx], 'nem<'
		jz		done
		mov		esi, eax
		mov		[ebx], edi
		mov		ecx, offset s_baseFilesCache
		mov		eax, [ecx+8]
		mov		[ebx+4], eax
		push	edi
		call	XMLCache::Write
		mov		eax, esi
		ALIGN 16
	done:
		pop		edi
		pop		esi
		pop		ebx
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
		ALIGN 16
	doFree:
		push	dword ptr [ecx+8]
		push	ecx
		GAME_HEAP_FREE
		GAME_HEAP_FREE
		retn
	}
}

__declspec(naked) void __fastcall ResolveTrait(XMLtoTileData *data, int EDX, UInt32 valueType, const char *valueStr, UInt8 dataType)
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		mov		eax, [ecx+0xC]
		mov		edx, [ecx]
		test	eax, eax
		cmovz	eax, edx
		push	eax
		sub		esp, 0x18
		mov		eax, 0x80000000
		mov		edx, [ebp+0xC]
		cmp		[ebp+0x10], 3
		cmovz	eax, edx
		mov		[ebp-0x1C], eax
		movd	xmm0, eax
		cvtdq2ps	xmm0, xmm0
		movd	[ebp-8], xmm0
		mov		eax, 0x11F32DC
		mov		edx, [eax]
		test	edx, edx
		jz		arrEmpty
		dec		edx
		mov		[eax], edx
		mov		ecx, [eax+edx*4-0x40]
		jmp		fillValues
		ALIGN 16
	arrEmpty:
		push	0x14
		GAME_HEAP_ALLOC
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
		GAME_HEAP_FREE
		jmp		doneStr
		ALIGN 16
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
		CALL_EAX(0xA0AF10)
		mov		ecx, [ebp-0x1C]
		mov		[ecx+0xC], eax
		test	eax, eax
		jnz		jmpLbl_18
		push	dword ptr [ebp+0xC]
		mov		ecx, [ebp-0x1C]
		CALL_EAX(0xA0AE70)
		mov		ecx, [ebp-0x1C]
		mov		[ecx+0xC], eax
		jmp		jmpLbl_18
		ALIGN 16
	jmpLbl_1:
		cvttss2si	edx, [eax+4]
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
		ALIGN 16
	jmpLbl_2:
		mov		eax, [ebp-0xC]
		mov		dword ptr [eax], 8
		mov		dword ptr [eax+0x10], 0xBBA
		jmp		jmpLbl_17
		ALIGN 16
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
		ALIGN 16
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
		cvttss2si	edx, [eax+4]
		cmp		edx, 0xFA1
		jl		jmpLbl_7
		cmp		edx, 0x101D
		jg		jmpLbl_8
		nop
	jmpLbl_6:
		mov     dword ptr [ecx], 8
		jmp		jmpLbl_9
		ALIGN 16
	jmpLbl_7:
		cmp		edx, kAction_copy
		jl		jmpLbl_8
		cmp		edx, kAction_MAX
		jge		jmpLbl_8
		mov     dword ptr [ecx], 9
		jmp		jmpLbl_9
		ALIGN 16
	jmpLbl_8:
		mov     dword ptr [ecx], 0xFFFFFFFF
	jmpLbl_9:
		cvttss2si	edx, [ecx+4]
		mov		[ecx+0x10], edx
		mov		eax, [ebp-0x10]
		mov		edx, [eax+4]
		mov		[ecx+4], edx
		push	dword ptr [eax+8]
		add		ecx, 8
		call	String::Set
		jmp		jmpLbl_18
		ALIGN 16
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
		cvttss2si	edx, [ecx+4]
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
		CALL_EAX(0xAE8010)
		mov		ecx, eax
		call	FreeExtraString
		jmp		jmpLbl_18
		ALIGN 16
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
		ALIGN 16
	jmpLbl_13:
		mov		dword ptr [ecx], 5
		jmp		jmpLbl_17
		ALIGN 16
	jmpLbl_14:
		cmp		edx, kAction_copy
		jl		jmpLbl_16
		cmp		edx, kAction_MAX
		jge		jmpLbl_16
		test	eax, eax
		jnz		jmpLbl_15
		mov		dword ptr [ecx], 2
		jmp		jmpLbl_17
		ALIGN 16
	jmpLbl_15:
		cmp		dword ptr [ebp-0x10], 0
		jz		jmpLbl_17
		mov		dword ptr [ecx], 3
		jmp		jmpLbl_17
		ALIGN 16
	jmpLbl_16:
		test	eax, eax
		jz		jmpLbl_17
		cvttss2si	edx, [ecx+4]
		cmp		edx, 0x385
		jl		jmpLbl_17
		cmp		edx, 0x38C
		jg		jmpLbl_17
		mov		dword ptr [ecx], 7
		nop
	jmpLbl_17:
		mov		ecx, [ebp-4]
		add		ecx, 8
		mov		[ebp-0x1C], ecx
		CALL_EAX(ADDR_GetAvailableLinkedNode)
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
		ALIGN 16
	jmpLbl_18:
		mov		ecx, [ebp-4]
		add		ecx, 8
		CALL_EAX(0xAE8010)
		mov		ecx, eax
		call	FreeExtraString
		mov		ecx, [ebp-0xC]
		call	FreeExtraString
		ALIGN 16
	done:
		test	edi, edi
		jnz		printLog
		leave
		retn	0xC
		ALIGN 16
	printLog:
		lea		ecx, [ebp+8]
		call	PrintTraitEntry
		leave
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
		ALIGN 16
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
		leave
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
		ALIGN 16
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
		ALIGN 16
	notStr:
		cmp		dl, 'f'
		jnz		notFlt
		mov		eax, [eax+4]
		mov		[ecx], eax
		mov		al, 1
		jmp		done
		ALIGN 16
	notFlt:
		cmp		dl, 'i'
		setz	al
		jnz		done
		movd	xmm0, [eax+4]
		cvtdq2ps	xmm0, xmm0
		movss	[ecx], xmm0
	done:
		pop		esi
		retn	4
	}
}

__declspec(naked) XMLtoTileData* __cdecl ResolveXMLFileHook(const char *filePath, char *inData, UInt32 inLength)
{
	__asm
	{
		push	edx
		push	esp
		push	dword ptr [esp+0xC]
		mov		ecx, offset s_cachedParsedData
		call	UnorderedMap<const char*, ParsedData>::Insert
		pop		edx
		test	al, al
		jnz		notCached
		mov		eax, [edx]
		test	eax, eax
		jz		notCached
		mov		[ebp-0x11], 1
		retn
		ALIGN 16
	notCached:
		push	ebp
		mov		ebp, esp
		sub		esp, 0x14
		push	ebx
		push	esi
		push	edi
		mov		esi, edx
		xor		edi, edi
		mov		eax, s_externFileData
		mov		s_externFileData, edi
		test	eax, eax
		jnz		hasData
		push	dword ptr [ebp+8]
		call	GetXMLFileDataHook
		pop		ecx
	hasData:
		mov		[ebp-4], eax
		cmp		dword ptr [eax+4], 0
		jz		skipClose
		mov		ebx, eax
		cmp		[esi+5], 0
		jz		notDebug
		mov		[esi+5], 0
		push	ebx
		push	dword ptr [ebp+8]
		call	InitDebugLog
		mov		edi, eax
	notDebug:
		push	0x14
		GAME_HEAP_ALLOC
		mov		ecx, eax
		CALL_EAX(ADDR_InitXMLtoTileData)
		cmp		[esi+4], 0
		jz		skipCache
		mov		[esi], eax
		mov		ecx, [ebp]
		mov		[ecx-0x11], 1
		ALIGN 16
	skipCache:
		mov		esi, eax
		pxor	xmm0, xmm0
		movups	[ebp-0x14], xmm0
		mov		ebx, [ebx+4]
		dec		ebx
		ALIGN 16
	mainIter:
		inc		ebx
		mov		al, [ebx]
		test	al, al
		jz		mainEnd
		cmp		al, '<'
		jnz		notOpen
		cmp		[ebp-5], 0
		setnz	dl
		mov		[ebp-8], dl
		jnz		doneOpen
		mov		[ebp-5], 1
		mov		ecx, [ebp-0xC]
		test	ecx, ecx
		jz		doneOpen
		mov		[ebx], 0
		mov		[ebp-0xF], 2
		cmp		[ecx], '&'
		jnz		notTag
		cmp		[ecx+1], '-'
		jnz		numTag
		lea		eax, [ebp-0xC]
		push	eax
		add		ecx, 2
		call	GetGMSTValue
		test	al, al
		jz		doneOpen
		mov		[ebp-0xF], al
		jmp		doneValue
		ALIGN 16
	numTag:
		push	ecx
		mov		ecx, 0x11F32F4
		call	NiTMap::Lookup
		test	eax, eax
		jz		doneValue
		movd	xmm0, [eax+8]
		cvtdq2ps	xmm0, xmm0
		jmp		isNum
		ALIGN 16
	notTag:
		movzx	edx, byte ptr [ecx]
		cmp		dl, ' '
		jb		doneOpen
		mov		eax, ecx
		ALIGN 16
	charIter:
		cmp		kValidForNumeric[edx], 0
		jz		doneValue
		inc		eax
		mov		dl, [eax]
		test	dl, dl
		jnz		charIter
		call	StrToDbl
		cvtsd2ss	xmm0, xmm0
	isNum:
		mov		[ebp-0xF], 1
		movss	[ebp-0xC], xmm0
	doneValue:
		movzx	eax, byte ptr [ebp-0xF]
		push	eax
		mov		edx, [ebp-0xC]
		push	edx
		push	0xBB9
		mov		ecx, esi
		call	ResolveTrait
	doneOpen:
		mov		dword ptr [ebp-0xC], 0
		jmp		mainIter
		ALIGN 16
	notOpen:
		cmp		al, '>'
		jnz		notClose
		cmp		[ebp-5], 0
		jz		doneOpen
		cmp		[ebp-7], 0
		jnz		doneClose
		mov		ecx, [ebp-0xC]
		test	ecx, ecx
		jz		doneClose
		mov		[ebx], 0
		push	ecx
		call	AddTraitName
		test	eax, eax
		jz		invID0
		mov		eax, [eax+8]
		movzx	ecx, byte ptr [ebp-6]
		push	3
		push	eax
		push	ecx
		mov		ecx, esi
		call	ResolveTrait
		jmp		doneClose
		ALIGN 16
	invID0:
		test	edi, edi
		jz		doneClose
		mov		[ebp-0xE], 1
		push	dword ptr [ebp-0xC]
		push	offset kErrorInvID
		push	edi
		call	fprintf
		add		esp, 0xC
	doneClose:
		mov		dword ptr [ebp-8], 0
		mov		dword ptr [ebp-0xC], 0
		mov		[ebp-0xD], 0
		jmp		mainIter
		ALIGN 16
	notClose:
		cmp		[ebp-5], 0
		jz		isClosed
		cmp		al, '/'
		jnz		notSlash
		mov		[ebx], 0
		mov		[ebp-6], 1
		mov		[ebp-7], 0
		mov		[ebp-0xD], 0
		jmp		mainIter
		ALIGN 16
	notSlash:
		cmp		[ebp-8], 0
		jnz		mainIter
		cmp		al, ' '
		jnz		notSpace
		mov		[ebp-0xD], 1
		mov		[ebx], 0
		jmp		mainIter
		ALIGN 16
	notSpace:
		cmp		dword ptr [ebp-0xC], 0
		jnz		procAction
		mov		[ebp-0xC], ebx
		mov		[ebp-0xD], 0
		jmp		mainIter
		ALIGN 16
	procAction:
		cmp		[ebp-0xD], 0
		jz		mainIter
		mov		[ebp-0xD], 0
		mov		[ebp-7], 1
		push	dword ptr [ebp-0xC]
		mov		ecx, 0x11F32F4
		call	NiTMap::Lookup
		test	eax, eax
		jnz		validID0
		mov		ecx, [ebp-0xC]
		mov		dword ptr [ebp-0xC], 0
		jmp		logErrorID
		ALIGN 16
	validID0:
		mov		eax, [eax+8]
		push	3
		push	eax
		push	0
		mov		ecx, esi
		call	ResolveTrait
		mov		ecx, [ebx]
		or		ecx, 0x20202020
		cmp		ecx, '=crs'
		jnz		notSrc
		mov		ecx, 0xBBB
		add		ebx, 3
		jmp		findDQ0
		ALIGN 16
	notSrc:
		cmp		ecx, 'eman'
		jnz		onError0
		mov		ecx, 0xBBA
		add		ebx, 4
		jmp		verifyEQ
	needTrait:
		mov		ecx, [ebx+2]
		and		ecx, 'IART'
		cmp		ecx, 'IART'
		jnz		onError1
		mov		ecx, 0xBBC
		add		ebx, 7
	verifyEQ:
		cmp		[ebx], '='
		jnz		onError2
		ALIGN 16
	findDQ0:
		inc		ebx
		mov		dl, [ebx]
		test	dl, dl
		jz		mainEnd
		cmp		dl, '\"'
		jnz		findDQ0
		lea		eax, [ebx+1]
	findDQ1:
		inc		ebx
		mov		dl, [ebx]
		test	dl, dl
		jz		mainEnd
		cmp		dl, '\"'
		jnz		findDQ1
		cmp		eax, ebx
		jz		onError3
		mov		[ebx], 0
		mov		[ebp-0x14], ecx
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
		mov		[ebp-8], 1
		test	edi, edi
		jz		mainIter
		mov		[ebp-0xE], 1
		push	ecx
		push	offset kErrorInvID
		push	edi
		call	fprintf
		add		esp, 0xC
		jmp		mainIter
		ALIGN 16
	validID1:
		mov		eax, [eax+8]
		mov		ecx, 0xBBC
		mov		edx, 3
		ALIGN 16
	notTrait:
		push	edx
		push	eax
		push	ecx
		mov		ecx, esi
		call	ResolveTrait
		cmp		dword ptr [ebp-0x14], 0xBBB
		jnz		mainIter
		cmp		[ebx+1], ' '
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
		mov		[ebp-8], 1
		test	edi, edi
		jz		mainIter
		mov		[ebp-0xE], 1
		push	edi
		movzx	eax, dl
		push	kErrorStrings[eax*4]
		call	fputs
		add		esp, 8
		jmp		mainIter
		ALIGN 16
	isClosed:
		cmp		dword ptr [ebp-0xC], 0
		jnz		mainIter
		mov		[ebp-0xC], ebx
		jmp		mainIter
		ALIGN 16
	mainEnd:
		test	edi, edi
		jz		skipClose
		mov		s_debugLog.theFile, 0
		push	edi
		call	fclose
		pop		ecx
		cmp		[ebp-0xE], 0
		jz		skipClose
		push	dword ptr [ebp+8]
		push	offset kResolveError
		push	s_log.theFile
		call	fprintf
		call	fflush
		add		esp, 0xC
		ALIGN 16
	skipClose:
		mov		eax, [ebp-4]
		push	eax
		push	dword ptr [eax+4]
		GAME_HEAP_FREE
		GAME_HEAP_FREE
		mov		eax, esi
		pop		edi
		pop		esi
		pop		ebx
		leave
		retn
	}
}

static const float kFlt1d100 = 0.01F;
__declspec(naked) void TileTextApplyScaleHook()
{
	__asm
	{
		push	dword ptr [ebp-0x1C]
		push	dword ptr [ebp-0x38]
		push	0
		lea		edx, [ebp-0x54]
		push	edx
		sub		edx, 4
		push	edx
		add		edx, 0x44
		push	edx
		mov		[ebp-0x29], 0
		mov		edx, 0xFB8	// kTileValue_zoom
		mov		ecx, [ebp-0x1B4]
		call	GetTileValue
		test	eax, eax
		jz		noScale
		mov		edx, [eax+8]
		test	edx, edx
		jle		noScale
		cmp		edx, 0x42C80000
		jz		noScale
		movd	xmm0, edx
		mulss	xmm0, kFlt1d100
		movss	[ebp-0x1C], xmm0
		mov		[ebp-0x29], 1
		cmp		dword ptr [ebp-0x58], 0x800
		jnb		noScale
		unpcklps	xmm0, xmm0
		movq	xmm1, qword ptr [ebp-0x58]
		cvtdq2ps	xmm1, xmm1
		divps	xmm1, xmm0
		cvtps2dq	xmm1, xmm1
		movq	qword ptr [ebp-0x58], xmm1
	noScale:
		mov		ecx, [ebp-0x64]
		CALL_EAX(0xA12880)
		mov		edx, [ebp-0xB0]
		cmp		[ebp-0x29], 0
		jz		done
		movss	xmm0, [ebp-0x1C]
		movss	[edx+0x64], xmm0
		unpcklps	xmm0, xmm0
		movq	xmm1, qword ptr [ebp-0x58]
		cvtdq2ps	xmm1, xmm1
		mulps	xmm0, xmm1
		cvtps2dq	xmm0, xmm0
		movq	qword ptr [ebp-0x58], xmm0
	done:
		JMP_EAX(0xA2221C)
	}
}

#define PATH_Globals			(const char*)0x106EFF8
#define PATH_MessageMenu		(const char*)0x1075724
#define PATH_InventoryMenu		(const char*)0x1073A70
#define PATH_StatsMenu			(const char*)0x107730C
#define PATH_HUDMainMenu		(const char*)0x107343C
#define PATH_LoadingMenu		(const char*)0x1073F04
#define PATH_ContainerMenu		(const char*)0x1072258
#define PATH_DialogMenu			(const char*)0x107266C
#define PATH_SleepWaitMenu		(const char*)0x1076468
#define PATH_StartMenu			(const char*)0x1076D8C
#define PATH_LockPickMenu		(const char*)0x10744A0
#define PATH_QuantityMenu		(const char*)0x10758C4
#define PATH_MapMenu			(const char*)0x1074DF8
#define PATH_LevelUpMenu		(const char*)0x1073D9C
#define PATH_RepairMenu			(const char*)0x1075D14
#define PATH_RaceSexMenu		(const char*)0x10759BC
#define PATH_CharGenMenu		(const char*)0x1071C5C
#define PATH_BarterMenu			(const char*)0x10707B4
#define PATH_HackingMenu		(const char*)0x1072990
#define PATH_VATSMenu			(const char*)0x1077D50
#define PATH_ComputersMenu		(const char*)0x10720E8
#define PATH_RepairServicesMenu	(const char*)0x1075E9C
#define PATH_TutorialMenu		(const char*)0x1077BA4
#define PATH_SpecialBookMenu	(const char*)0x1076874
#define PATH_ItemModMenu		(const char*)0x1073C34
#define PATH_LoveTesterMenu		(const char*)0x10746DC
#define PATH_CompanionWheelMenu	(const char*)0x1071DFC
#define PATH_TraitSelectMenu	(const char*)0x1077B04
#define PATH_RecipeMenu			(const char*)0x107054C
#define PATH_SlotMachineMenu	(const char*)0x1076670
#define PATH_BlackJackMenu		(const char*)0x1070C38
#define PATH_RouletteMenu		(const char*)0x1076254
#define PATH_CaravanMenu		(const char*)0x107170C
#define PATH_TraitMenu			(const char*)0x1077A5C

const MappedPair<const char*, const char*> kMenuNameToFile[] =
{
	{"BarterMenu", PATH_BarterMenu}, {"BlackJackMenu", PATH_BlackJackMenu}, {"CaravanMenu", PATH_CaravanMenu}, {"CharGenMenu", PATH_CharGenMenu},
	{"CompanionWheelMenu", PATH_CompanionWheelMenu}, {"ComputersMenu", PATH_ComputersMenu}, {"ContainerMenu", PATH_ContainerMenu},
	{"DialogMenu", PATH_DialogMenu}, {"HackingMenu", PATH_HackingMenu}, {"LevelUpMenu", PATH_LevelUpMenu}, {"LoadingMenu", PATH_LoadingMenu},
	{"LockPickMenu", PATH_LockPickMenu}, {"LoveTesterMenu", PATH_LoveTesterMenu}, {"MessageMenu", PATH_MessageMenu}, {"QuantityMenu", PATH_QuantityMenu},
	{"RaceSexMenu", PATH_RaceSexMenu}, {"RecipeMenu", PATH_RecipeMenu}, {"RepairServicesMenu", PATH_RepairServicesMenu}, {"RouletteMenu", PATH_RouletteMenu},
	{"SleepWaitMenu", PATH_SleepWaitMenu}, {"SlotMachineMenu", PATH_SlotMachineMenu}, {"SpecialBookMenu", PATH_SpecialBookMenu}, {"StartMenu", PATH_StartMenu},
	{"TraitMenu", PATH_TraitMenu}, {"TraitSelectMenu", PATH_TraitSelectMenu}, {"TutorialMenu", PATH_TutorialMenu}, {"VATSMenu", PATH_VATSMenu},
	{"HUDMainMenu", PATH_HUDMainMenu}, {"InventoryMenu", PATH_InventoryMenu}, {"ItemModMenu", PATH_ItemModMenu}, {"MapMenu", PATH_MapMenu},
	{"RepairMenu", PATH_RepairMenu}, {"StatsMenu", PATH_StatsMenu}
};

const char *kTileTypeNames[] = {"menu", "rect", "image", "text", "hotrect", "nif", "radial", "template"};

typedef UnorderedSet<char*> IncludePaths;
typedef UnorderedMap<char*, IncludePaths> IncludeRefs;

class TileName
{
	char		*name;

public:
	TileName(const char *inName) : name(CopyString(inName)) {}
	~TileName() {free(name);}

	operator char*() const {return name;}
};

void __fastcall UIOLoad(InterfaceManager *interfaceMgr)
{
	*(InterfaceManager**)0x11D8A80 = interfaceMgr;

	void (*RegTraitID)(const char*, UInt32) = (void (*)(const char*, UInt32))0x9FF8A0;

	RegTraitID("ge", kAction_gte);
	RegTraitID("ne", kAction_neq);
	RegTraitID("le", kAction_lte);

	//	Register new operators
	RegTraitID("neg", kAction_neg);
	RegTraitID("recipr", kAction_recipr);
	RegTraitID("land", kAction_land);
	RegTraitID("lor", kAction_lor);
	RegTraitID("shl", kAction_shl);
	RegTraitID("shr", kAction_shr);
	RegTraitID("bt", kAction_bt);
	RegTraitID("sqrt", kAction_sqrt);
	RegTraitID("pow", kAction_pow);
	RegTraitID("sin", kAction_sin);
	RegTraitID("cos", kAction_cos);
	RegTraitID("tan", kAction_tan);
	RegTraitID("asin", kAction_asin);
	RegTraitID("acos", kAction_acos);
	RegTraitID("atan", kAction_atan);
	RegTraitID("log", kAction_log);
	RegTraitID("rand", kAction_rand);

	SAFE_WRITE_BUF(0xA03923, "\x8B\x45\x08\x3D\xB7\x0F\x00\x00\x72\x50\x3D\xF7\x0F\x00\x00\x74\x07\x3D\xBE\x0F\x00\x00\x77\x42\x8B\x45\xCC\xF6\x40\x30\x02\x75\x0B\x80\x48\x30\x02\x50\xE8\x42\x3D\x00\x00\x58\xC9\xC2\x0C\x00");
	SafeWrite8(0xA21B83, 0xAC);
	SafeWrite8(0xA22041, 0xAC);
	SafeWrite8(0xA22047, 0xAC);
	SafeWrite8(0xA22125, 0xAC);
	SafeWrite8(0xA223D2, 0xAC);
	SafeWrite8(0xA21BCB, 0xE4);
	SafeWrite8(0xA22014, 0xE4);
	WriteRelJump(0xA221F8, (UInt32)TileTextApplyScaleHook);
	if (*(UInt32*)0xA22244 == 0xA222E768)
		SAFE_WRITE_BUF(0xA22244, "\x68\xAD\x0F\x00\x00\x8B\x8D\x4C\xFE\xFF");

	//	Nuke HUDMainMenu/HardcoreMode/ set texts <zoom>
	SafeWrite16(0x76F688, 0x20EB);
	SafeWrite16(0x76F742, 0x1FEB);
	SafeWrite16(0x76F824, 0x1FEB);
	SafeWrite16(0x76F92D, 0x20EB);
	SafeWrite16(0x76FA5E, 0x1FEB);

	CdeclCall(0x476C00);

	g_gameSettingsMap = (NiTMap*)(*(UInt8**)0x11C8048 + 0x10C);

	PrintLog("--------------------------------\n*** Checking supported files ***\n--------------------------------\n\n");

	UnorderedMap<const char*, const char*> menuNameToFile(kMenuNameToFile, 33);
	UnorderedMap<char*, IncludeRefs> injectLists;
	char menusPath[0x80], prefabsPath[0x80], publicPath[0x50], *bufferPtr, *menuFile, *tileName, *pathStr, *delim;
	bool bDarnUI = false, bVUIplus = false, bOneHUD = false, doSupported = true, fixSpaces = false, condLine, skipCond, cndEvalStack[0x10], expectANDOR, cndNeg, tempRes;
	UInt32 cndStackLevel, cndSkipLevel, cndToken;

	s_cachedBaseFiles[kTempXMLFile] = {};
	for (auto menuPath : {PATH_Globals, PATH_HUDMainMenu, PATH_InventoryMenu, PATH_ItemModMenu, PATH_MapMenu, PATH_RepairMenu, PATH_StatsMenu})
		s_cachedParsedData.Emplace(menuPath, false);
	s_cachedParsedData.Emplace(kTempXMLFile, false, false);

	bufferPtr = s_baseFilesCache.cache;
	if (GetPrivateProfileString("General", "sDoNotCache", nullptr, bufferPtr, 0x4000, "Data\\uio\\settings.ini"))
	{
		for (; *bufferPtr; bufferPtr = delim)
		{
			delim = GetNextToken(bufferPtr, '\t');
			s_cachedBaseFiles[bufferPtr] = {};
			s_cachedParsedData.Emplace(bufferPtr, false);
		}
	}

	UnorderedSet<const char*> loadedMods(0x100);
	memcpy(StrCopy(menusPath, (const char*)0x1202E98), "plugins.txt", 12);
	for (LineIterator modIter(menusPath, s_baseFilesCache.cache); modIter; ++modIter)
		loadedMods.Insert(*modIter);

	if (FileExists(PATH_HUDMainMenu))
	{
		bOneHUD = loadedMods.HasKey("oHUD.esm");
		if (FileExists(PATH_InventoryMenu) && FileExists(PATH_MapMenu) && FileExists(PATH_StatsMenu) && FileExists(PATH_StartMenu))
		{
			bDarnUI = FileExists("Data\\menus\\prefabs\\duinvsettings.xml");
			bVUIplus = FileExists("Data\\menus\\prefabs\\VUI+\\settings.xml");
		}
	}
	PrintLog("[%c] DarNified UI\n[%c] Vanilla UI Plus (VUI+)\n[%c] One HUD (oHUD)\n", bDarnUI ? 'x' : ' ', bVUIplus ? 'x' : ' ', bOneHUD ? 'x' : ' ');

	memcpy(menusPath, "Data\\menus\\", 12);
	memcpy(prefabsPath, "Data\\menus\\prefabs\\", 20);
	LineIterator lineIter("Data\\uio\\supported.txt", s_baseFilesCache.cache);
	memcpy(publicPath, "Data\\uio\\public\\*.txt", 22);
	DirectoryIterator pubIter(publicPath);

	while (true)
	{
		if (!doSupported)
		{
			if (!pubIter) break;
			StrCopy(publicPath + 16, *pubIter);
			++pubIter;
			lineIter.Init(publicPath);
			fixSpaces = true;
			PrintLog("\n>>>>> Processing file '%s'\n", publicPath + 9);
		}
		else
		{
			doSupported = false;
			PrintLog("\n>>>>> Processing file 'supported.txt'\n");
		}

		if (!lineIter)
		{
			PrintLog("\tERROR: Could not open file > Skipping.\n");
			continue;
		}

		condLine = false;
		do
		{
			bufferPtr = *lineIter;
			++lineIter;
			if (*bufferPtr == ';') continue;
			if (!condLine)
			{
				menuFile = GetNextToken(bufferPtr, ':');
				if (!*menuFile) continue;
				skipCond = condLine = true;
				PrintLog("\t> Checking '%s'\n", bufferPtr);
				StrCopy(prefabsPath + 19, bufferPtr);
				if (!FileExists(prefabsPath))
				{
					PrintLog("\t\t! File not found > Skipping.\n");
					continue;
				}
				tileName = GetNextToken(menuFile, ':');
				if (*menuFile == '@')
				{
					if (!*tileName && !StrBeginsCI(menuFile + 1, "prefabs\\"))
					{
						PrintLog("\t\t! Injecting @ EoF is only allowed with prefab files > Skipping.\n");
						continue;
					}
					StrCopy(menusPath + 11, menuFile + 1);
					menuFile = menusPath;
				}
				else
				{
					if (!*tileName) tileName = menuFile;
					menuFile = (char*)menuNameToFile.Get(menuFile);
					if (!menuFile)
					{
						PrintLog("\t\t! Invalid target menu > Skipping.\n");
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

				if (fixSpaces)
					FixSpaces(bufferPtr);
				
				cndEvalStack[0] = false;
				cndStackLevel = 0;
				cndSkipLevel = 0;
				expectANDOR = false;
				for (; *bufferPtr; bufferPtr = delim)
				{
					delim = GetNextToken(bufferPtr, '\t');
					if (*bufferPtr == '(')
					{
						expectANDOR = false;
						if (cndSkipLevel)
							cndSkipLevel++;
						else
							cndEvalStack[++cndStackLevel] = false;
						continue;
					}
					if (*bufferPtr == ')')
					{
						expectANDOR = true;
						if (cndSkipLevel)
							cndSkipLevel--;
						else if (cndStackLevel)
						{
							tempRes = cndEvalStack[cndStackLevel];
							cndEvalStack[--cndStackLevel] = tempRes;
						}
						continue;
					}
					if (cndSkipLevel) continue;
					if (expectANDOR)
					{
						if (*(UInt16*)bufferPtr == '&&')
							cndSkipLevel = !cndEvalStack[cndStackLevel];
						else if (*(UInt16*)bufferPtr == '||')
							cndSkipLevel = cndEvalStack[cndStackLevel];
						else continue;
					}
					else
					{
						if (cndNeg = *bufferPtr == '!')
							bufferPtr++;
						cndToken = *(UInt32*)bufferPtr | 0x20202020;
						if (cndToken == 'eurt')
							tempRes = true;
						else if (cndToken == 'nrad')
							tempRes = bDarnUI;
						else if (cndToken == '+iuv')
							tempRes = bVUIplus;
						else if (cndToken == 'duho')
							tempRes = bOneHUD;
						else if (*bufferPtr == '#')
						{
							StrCopy(prefabsPath + 19, bufferPtr + 1);
							tempRes = FileExists(prefabsPath);
						}
						else if (*bufferPtr == '?')
							tempRes = loadedMods.HasKey(bufferPtr + 1);
						else tempRes = false;

						cndEvalStack[cndStackLevel] = tempRes ^ cndNeg;
					}
					expectANDOR = !expectANDOR;
				}

				if (!cndEvalStack[0])
					PrintLog("\t\t! Conditions evaluated as FALSE > Skipping.\n");
				else if (injectLists[menuFile][tileName].Insert(pathStr))
					PrintLog("\t\t+ Adding to '%s' @ %s\n", menuFile + 11, *tileName ? tileName : "EoF");
				else PrintLog("\t\t! File already added > Skipping.\n");
			}
		}
		while (lineIter);
	}

	PrintLog("\n---------------------------\n*** Patching menu files ***\n---------------------------\n");

	if (injectLists.Empty())
	{
		PrintLog("\nNo files requiring patching > Aborting.\n");
		return;
	}

	UnorderedSet<const char*> tileTypes(kTileTypeNames, 8);
	UInt32 length;
	XMLFileData *fileData;
	char *bgnPtr, *endPtr, srchType;
	Vector<TileName> openTiles(0x10);

	for (auto menuIter = injectLists.Begin(); menuIter; ++menuIter)
	{
		PrintLog("\n>>>>> Processing file '%s'\n", menuIter.Key() + 11);

		fileData = XMLFileData::Get(menuIter.Key());
		if (!fileData)
		{
			PrintLog("\tERROR: Could not open file > Skipping.\n");
			continue;
		}

		s_baseFilesCache.length = 0;

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
						tileName = openTiles.Top();
						auto findTile = menuIter().Find(tileName);
						if (findTile)
						{
							for (auto inclIter = findTile().Begin(); inclIter; ++inclIter)
							{
								s_baseFilesCache.WriteInclude(*inclIter);
								PrintLog("\t+ Injecting '%s' @ %s\n", *inclIter, tileName);
							}
							findTile.Remove();
						}
						openTiles.Pop();
					}
					*endPtr = '>';

					if (menuIter().Empty())
					{
						s_baseFilesCache.Write(bufferPtr, fileData->length - (bufferPtr - fileData->data));
						break;
					}
				}
				else if (length >= 12)
				{
					if (delim = SubStrCI(bgnPtr + 4, "name=", length - 4))
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
							auto findTile = menuIter().Find(openTiles.Top());
							if ((findTile && findTile().HasKey(bgnPtr)) || StrBeginsCI(bgnPtr, "includes_"))
							{
								PrintLog("\t- Removing duplicate reference '%s'\n", bgnPtr);
								bufferPtr = endPtr + 1;
								continue;
							}
						}
						*delim = '\"';
					}
				}
			}

			endPtr++;
			s_baseFilesCache.Write(bufferPtr, endPtr - bufferPtr);
			bufferPtr = endPtr;
		}

		fileData->Destroy();
		openTiles.Clear();

		for (auto notFound = menuIter().Begin(); notFound; ++notFound)
		{
			if (*notFound.Key())
				PrintLog("\t! Menu element '%s' not found > Skipping.\n", notFound.Key());
			else
			{
				for (auto inclIter = notFound().Begin(); inclIter; ++inclIter)
				{
					s_baseFilesCache.WriteInclude(*inclIter);
					PrintLog("\t+ Injecting '%s' @ EoF\n", *inclIter);
				}
			}
		}

		s_cachedBaseFiles.Emplace(menuIter.Key(), StrBeginsCI(menuIter.Key() + 11, "prefabs\\"));
	}
}

__declspec(naked) void DoTileOperatorCOPY()
{
	__asm
	{
		mov		edx, [ebp-0x34]
		test	edx, edx
		jz		setValue
		mov		ecx, [eax+0xC]
		test	ecx, ecx
		jz		copyStr
		push	ecx
		push	edx
		CALL_EAX(ADDR_strcmp)
		pop		ecx
		test	eax, eax
		jnz		freeStr
		pop		ecx
		mov		[ebp-0x25], 0
		JMP_EAX(0xA09506)
		ALIGN 16
	setValue:
		mov		edx, [ebp-0x38]
		mov		[eax+8], edx
		JMP_EAX(0xA09506)
		ALIGN 16
	freeStr:
		GAME_HEAP_FREE
	copyStr:
		mov		[ebp-0x25], 1
		mov		ecx, [ebp-0x34]
		call	StrLen
		inc		eax
		push	eax
		push	ecx
		push	eax
		GAME_HEAP_ALLOC
		push	eax
		call	_memcpy
		add		esp, 0xC
		mov		ecx, [ebp-0x6C]
		mov		[ecx+0xC], eax
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorADD()
{
	__asm
	{
		movss	xmm0, [eax+8]
		addss	xmm0, [ebp-0x38]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorSUB()
{
	__asm
	{
		movss	xmm0, [eax+8]
		subss	xmm0, [ebp-0x38]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorMUL()
{
	__asm
	{
		movss	xmm0, [eax+8]
		mulss	xmm0, [ebp-0x38]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorDIV()
{
	__asm
	{
		cmp		dword ptr [ebp-0x38], 0
		jz		done
		movss	xmm0, [eax+8]
		divss	xmm0, [ebp-0x38]
		movss	[eax+8], xmm0
	done:
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorMIN()
{
	__asm
	{
		movss	xmm0, [eax+8]
		minss	xmm0, [ebp-0x38]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorMAX()
{
	__asm
	{
		movss	xmm0, [eax+8]
		maxss	xmm0, [ebp-0x38]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorMOD()
{
	__asm
	{
		cvtss2si	ecx, [ebp-0x38]
		test	ecx, ecx
		jz		done
		cvtss2si	eax, [eax+8]
		cdq
		idiv	ecx
		movd	xmm0, edx
		cvtdq2ps	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
	done:
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorFLOOR()
{
	__asm
	{
		movss	xmm0, [eax+8]
		addss	xmm0, [ebp-0x38]
		lea		ecx, [ebp-0x78]
		mov		dword ptr [ecx], 0x3FA0
		ldmxcsr	[ecx]
		cvtps2dq	xmm0, xmm0
		mov		byte ptr [ecx+1], 0x1F
		ldmxcsr	[ecx]
		cvtdq2ps	xmm0, xmm0
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorCEIL()
{
	__asm
	{
		movss	xmm0, [eax+8]
		addss	xmm0, [ebp-0x38]
		lea		ecx, [ebp-0x78]
		mov		dword ptr [ecx], 0x5FA0
		ldmxcsr	[ecx]
		cvtps2dq	xmm0, xmm0
		mov		byte ptr [ecx+1], 0x1F
		ldmxcsr	[ecx]
		cvtdq2ps	xmm0, xmm0
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorABS()
{
	__asm
	{
		movss	xmm0, [eax+8]
		addss	xmm0, [ebp-0x38]
		movd	ecx, xmm0
		and		ecx, 0x7FFFFFFF
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorROUND()
{
	__asm
	{
		mov		ecx, [eax+8]
		test	ecx, ecx
		jz		done
		mov		edx, [ebp-0x38]
		test	edx, edx
		jle		done
		movd	xmm0, ecx
		movd	xmm1, edx
		divss	xmm0, xmm1
		addss	xmm0, kFlt1d100
		cvtps2dq	xmm0, xmm0
		cvtdq2ps	xmm0, xmm0
		mulss	xmm0, xmm1
		movss	[eax+8], xmm0
	done:
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorGT()
{
	__asm
	{
		xor		ecx, ecx
		mov		edx, 0x3F800000
		movss	xmm0, [eax+8]
		comiss	xmm0, [ebp-0x38]
		cmova	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorGTE()
{
	__asm
	{
		xor		ecx, ecx
		mov		edx, 0x3F800000
		movss	xmm0, [eax+8]
		comiss	xmm0, [ebp-0x38]
		cmovnb	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorEQ()
{
	__asm
	{
		xor		ecx, ecx
		mov		edx, 0x3F800000
		movss	xmm0, [eax+8]
		comiss	xmm0, [ebp-0x38]
		cmovz	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorNEQ()
{
	__asm
	{
		xor		ecx, ecx
		mov		edx, 0x3F800000
		movss	xmm0, [eax+8]
		comiss	xmm0, [ebp-0x38]
		cmovnz	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorLT()
{
	__asm
	{
		xor		ecx, ecx
		mov		edx, 0x3F800000
		movss	xmm0, [eax+8]
		comiss	xmm0, [ebp-0x38]
		cmovb	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorLTE()
{
	__asm
	{
		xor		ecx, ecx
		mov		edx, 0x3F800000
		movss	xmm0, [eax+8]
		comiss	xmm0, [ebp-0x38]
		cmovbe	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorAND()
{
	__asm
	{
		cmp		dword ptr [eax+8], 0
		jz		done
		mov		edx, 0x3F800000
		cmp		dword ptr [ebp-0x34], 0
		jz		value
		mov		[eax+8], edx
	done:
		JMP_EAX(0xA09506)
		ALIGN 16
	value:
		xor		ecx, ecx
		cmp		dword ptr [ebp-0x38], 0
		cmovnz	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorOR()
{
	__asm
	{
		mov		ecx, [eax+8]
		cmp		dword ptr [ebp-0x34], 0
		jnz		done
		or		ecx, [ebp-0x38]
	done:
		mov		edx, 0x3F800000
		test	ecx, ecx
		cmovnz	ecx, edx
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorNOT()
{
	__asm
	{
		xor		ecx, ecx
		cmp		dword ptr [ebp-0x34], 0
		jnz		done
		mov		edx, 0x3F800000
		cmp		dword ptr [ebp-0x38], 0
		cmovz	ecx, edx
	done:
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorONLYIF()
{
	__asm
	{
		cmp		dword ptr [ebp-0x38], 0
		jnz		done
		cmp		dword ptr [ebp-0x34], 0
		jnz		done
		mov		dword ptr [eax+8], 0
	done:
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorONLYIFNOT()
{
	__asm
	{
		mov		ecx, [ebp-0x38]
		or		ecx, [ebp-0x34]
		jz		done
		mov		dword ptr [eax+8], 0
	done:
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorNEG()
{
	__asm
	{
		mov		ecx, [ebp-0x38]
		test	ecx, ecx
		jz		done
		xor		ecx, 0x80000000
	done:
		mov		[eax+8], ecx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorRECIPR()
{
	__asm
	{
		mov		edx, 0xA09506
		mov		ecx, [ebp-0x38]
		test	ecx, ecx
		jz		isZero
		movd	xmm0, ecx
		rcpss	xmm0, xmm0
		movss	[eax+8], xmm0
		jmp		edx
		ALIGN 16
	isZero:
		mov		[eax+8], ecx
		jmp		edx
	}
}
__declspec(naked) void DoTileOperatorLAND()
{
	__asm
	{
		movss	xmm0, [eax+8]
		cvtps2dq	xmm0, xmm0
		movss	xmm1, [ebp-0x38]
		cvtps2dq	xmm1, xmm1
		andps	xmm0, xmm1
		cvtdq2ps	xmm0, xmm0
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorLOR()
{
	__asm
	{
		movss	xmm0, [eax+8]
		cvtps2dq	xmm0, xmm0
		movss	xmm1, [ebp-0x38]
		cvtps2dq	xmm1, xmm1
		orps	xmm0, xmm1
		cvtdq2ps	xmm0, xmm0
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorSHL()
{
	__asm
	{
		movss	xmm0, [eax+8]
		cvtps2dq	xmm0, xmm0
		movss	xmm1, [ebp-0x38]
		cvtps2dq	xmm1, xmm1
		pslld	xmm0, xmm1
		cvtdq2ps	xmm0, xmm0
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorSHR()
{
	__asm
	{
		movss	xmm0, [eax+8]
		cvtps2dq	xmm0, xmm0
		movss	xmm1, [ebp-0x38]
		cvtps2dq	xmm1, xmm1
		psrad	xmm0, xmm1
		cvtdq2ps	xmm0, xmm0
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorBT()
{
	__asm
	{
		cvttss2si	ecx, [eax+8]
		cvttss2si	edx, [ebp-0x38]
		and		edx, 0x1F
		bt		ecx, edx
		setc	dl
		neg		edx
		and		edx, 0x3F800000
		mov		[eax+8], edx
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorSQRT()
{
	__asm
	{
		mov		ecx, [ebp-0x38]
		test	ecx, ecx
		js		done
		movd	xmm0, ecx
		sqrtss	xmm0, xmm0
		movss	[eax+8], xmm0
	done:
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorPOW()
{
	__asm
	{
		movss	xmm0, [eax+8]
		cvtps2pd	xmm0, xmm0
		movss	xmm1, [ebp-0x38]
		cvtps2pd	xmm1, xmm1
		call	dPow
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorSIN()
{
	__asm
	{
		movss	xmm0, [ebp-0x38]
		cvtps2pd	xmm0, xmm0
		mulsd	xmm0, kDblPId180
		call	dSin
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorCOS()
{
	__asm
	{
		movss	xmm0, [ebp-0x38]
		cvtps2pd	xmm0, xmm0
		mulsd	xmm0, kDblPId180
		call	dCos
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorTAN()
{
	__asm
	{
		movss	xmm0, [ebp-0x38]
		cvtps2pd	xmm0, xmm0
		mulsd	xmm0, kDblPId180
		call	dTan
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorASIN()
{
	__asm
	{
		movss	xmm0, [ebp-0x38]
		cvtps2pd	xmm0, xmm0
		call	dASin
		mulsd	xmm0, kDbl180dPI
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorACOS()
{
	__asm
	{
		movss	xmm0, [ebp-0x38]
		cvtps2pd	xmm0, xmm0
		call	dACos
		mulsd	xmm0, kDbl180dPI
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorATAN()
{
	__asm
	{
		movss	xmm0, [ebp-0x38]
		cvtps2pd	xmm0, xmm0
		call	dATan
		mulsd	xmm0, kDbl180dPI
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorLOG()
{
	__asm
	{
		mov		ecx, [ebp-0x38]
		test	ecx, ecx
		jle		done
		movd	xmm0, ecx
		cvtps2pd	xmm0, xmm0
		call	dLog
		cvtsd2ss	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
	done:
		JMP_EAX(0xA09506)
	}
}
__declspec(naked) void DoTileOperatorRAND()
{
	__asm
	{
		cvttss2si	edx, [ebp-0x38]
		test	edx, edx
		jle		done
		push	edx
		mov		ecx, 0x11C4180
		CALL_EAX(ADDR_GetRandomInt)
		movd	xmm0, eax
		cvtdq2ps	xmm0, xmm0
		mov		eax, [ebp-0x6C]
		movss	[eax+8], xmm0
		JMP_EAX(0xA09506)
		ALIGN 16
	done:
		mov		dword ptr [eax+8], 0
		JMP_EAX(0xA09506)
	}
}

const void *kDoOperatorJumpTable[] =
{
	DoTileOperatorCOPY, DoTileOperatorADD, DoTileOperatorSUB, DoTileOperatorMUL, DoTileOperatorDIV, DoTileOperatorMIN, DoTileOperatorMAX, DoTileOperatorMOD,
	DoTileOperatorFLOOR, DoTileOperatorCEIL, DoTileOperatorABS, DoTileOperatorROUND, DoTileOperatorGT, DoTileOperatorGTE, DoTileOperatorEQ, DoTileOperatorNEQ,
	DoTileOperatorLT, DoTileOperatorLTE, DoTileOperatorAND, DoTileOperatorOR, DoTileOperatorNOT, DoTileOperatorONLYIF, DoTileOperatorONLYIFNOT,
	(void*)0xA09506, (void*)0xA09506, (void*)0xA09506,
	//	Extra operators:
	DoTileOperatorNEG, DoTileOperatorRECIPR, DoTileOperatorLAND, DoTileOperatorLOR, DoTileOperatorSHL, DoTileOperatorSHR, DoTileOperatorBT, DoTileOperatorSQRT,
	DoTileOperatorPOW, DoTileOperatorSIN, DoTileOperatorCOS, DoTileOperatorTAN, DoTileOperatorASIN, DoTileOperatorACOS, DoTileOperatorATAN, DoTileOperatorLOG,
	DoTileOperatorRAND
};

#define NUM_OPERATORS sizeof(kDoOperatorJumpTable) / 4

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
	info->version = 226;
	if (nvse->isEditor)
		return false;
	s_log.OpenWrite(s_logPath);
	if (nvse->runtimeVersion != 0x040020D0)
	{
		PrintLog("ERROR: Unsupported runtime version (%08X).\n", nvse->runtimeVersion);
		return false;
	}
	return true;
}

bool NVSEPlugin_Load(const NVSEInterface *nvse)
{
	_memcpy = memcpy;
	_memmove = memmove;

	if (GetPrivateProfileInt("General", "bDebugMode", 0, "Data\\uio\\settings.ini"))
	{
		s_debugMode = true;
		s_logPath = "uio_debug\\uio_debug.log";
		PrintLog("Debug mode enabled > Log moved to %s", s_logPath);
		s_log.Close();
		CreateDirectory(s_debugPath, NULL);
		s_log.OpenWrite(s_logPath);
	}
	PrintLog("UI Organizer v2.26\nRuntime version = %08X\nNVSE version = %08X\n\n", nvse->runtimeVersion, nvse->nvseVersion);

	SAFE_WRITE_BUF(0x70A049, "\xC7\x45\xFC\xFF\xFF\xFF\xFF\x68\x5C\xA0\x70\x00");
	WriteRelJump(0x70A055, (UInt32)UIOLoad);
	WriteRelCall(0xA01B87, (UInt32)ResolveXMLFileHook);
	WriteRelCall(0xA02F44, (UInt32)GetXMLFileDataHook);

	UInt8 instrBuffer[43] =
	{
		0x8B, 0x45, 0xD4, 0x8B, 0x48, 0x08, 0x89, 0x4D, 0xD4, 0x8B, 0x55, 0xD0, 0x81, 0xEA, 0xD0, 0x07, 0x00, 0x00, 0x83, 0xFA,
		NUM_OPERATORS, 0x0F, 0x83, 0x5F, 0xFE, 0xFF, 0xFF, 0x8B, 0x85, 0x20, 0xFF, 0xFF, 0xFF, 0x89, 0x45, 0x94, 0xFF, 0x24, 0x95
	};
	/*
		mov		eax, [ebp-0x2C]
		mov		ecx, [eax+8]
		mov		[ebp-0x2C], ecx
		mov		edx, [ebp-0x30]
		sub		edx, 0x7D0
		cmp		edx, NUM_OPERATORS
		jnb		-0x1A1 (0xA09506)
		mov		eax, [ebp-0xE0]		// Tile::Value*
		mov		[ebp-0x6C], eax
		jmp		ds:kDoOperatorJumpTable[edx*4]
	*/
	*(UInt32*)(instrBuffer + sizeof(instrBuffer) - 4) = (UInt32)&kDoOperatorJumpTable;
	SafeWriteBuf(0xA0968C, instrBuffer, sizeof(instrBuffer));
	return true;
}