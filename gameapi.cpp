#include "gameapi.h"

__declspec(naked) void String::Set(const char *src)
{
	__asm
	{
		push	esi
		mov		esi, ecx
		mov		ecx, [esp+8]
		call	StrLen
		mov		[esi+4], ax
		test	eax, eax
		jz		nullStr
		mov		ecx, [esi]
		cmp		word ptr [esi+6], ax
		jnb		doCopy
		mov		[esi+6], ax
		inc		eax
		push	eax
		test	ecx, ecx
		jz		doAlloc
		push	ecx
		GAME_HEAP_FREE
	doAlloc:
		GAME_HEAP_ALLOC
		mov		[esi], eax
		mov		ecx, eax
		movzx	eax, word ptr [esi+4]
	doCopy:
		inc		eax
		push	eax
		push	dword ptr [esp+0xC]
		push	ecx
		call	_memcpy
		add		esp, 0xC
		pop		esi
		retn	4
	nullStr:
		mov		eax, [esi]
		test	ecx, ecx
		jz		nullSrc
		test	eax, eax
		jz		done
		mov		[eax], 0
		pop		esi
		retn	4
	nullSrc:
		mov		word ptr [esi+6], 0
		test	eax, eax
		jz		done
		mov		dword ptr [esi], 0
		push	eax
		GAME_HEAP_FREE
	done:
		pop		esi
		retn	4
	}
}

__declspec(naked) NiTMap::Entry *NiTMap::Lookup(UInt32 key)
{
	__asm
	{
		push	esi
		push	edi
		mov		esi, ecx
		push	dword ptr [esp+0xC]
		mov		eax, [ecx]
		call	dword ptr [eax+4]
		mov		ecx, [esi+8]
		mov		edi, [ecx+eax*4]
		ALIGN 16
	findEntry:
		test	edi, edi
		jz		done
		push	dword ptr [edi+4]
		push	dword ptr [esp+0x10]
		mov		ecx, esi
		mov		eax, [ecx]
		call	dword ptr [eax+8]
		test	al, al
		jnz		done
		mov		edi, [edi]
		jmp		findEntry
		ALIGN 16
	done:
		mov		eax, edi
		pop		edi
		pop		esi
		retn	4
	}
}

__declspec(naked) bool NiTMap::Insert(UInt32 key, Entry **outEntry)
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		push	ecx
		push	esi
		push	edi
		mov		esi, ecx
		push	dword ptr [ebp+8]
		mov		eax, [ecx]
		call	dword ptr [eax+4]
		mov		[ebp-4], eax
		mov		ecx, [esi+8]
		mov		edi, [ecx+eax*4]
		ALIGN 16
	findEntry:
		test	edi, edi
		jz		notFound
		push	dword ptr [edi+4]
		push	dword ptr [ebp+8]
		mov		ecx, esi
		mov		eax, [ecx]
		call	dword ptr [eax+8]
		test	al, al
		jnz		gotEntry
		mov		edi, [edi]
		jmp		findEntry
	gotEntry:
		xor		al, al
		jmp		done
	notFound:
		CALL_EAX(0x43A010)
		mov		edi, eax
		mov		dword ptr [eax+8], 0
		mov		ecx, [ebp+8]
		cmp		esi, 0x11F3308
		jz		notStr
		call	StrLen
		inc		eax
		push	eax
		push	eax
		GAME_HEAP_ALLOC
		push	dword ptr [ebp+8]
		push	eax
		call	_memcpy
		add		esp, 0xC
		mov		ecx, eax
	notStr:
		mov		[edi+4], ecx
		mov		eax, [ebp-4]
		mov		ecx, [esi+8]
		lea		eax, [ecx+eax*4]
		mov		edx, [eax]
		mov		[edi], edx
		mov		[eax], edi
		inc		dword ptr [esi+0xC]
		mov		al, 1
	done:
		mov		edx, [ebp+0xC]
		mov		[edx], edi
		pop		edi
		pop		esi
		leave
		retn	8
	}
}

__declspec(naked) TileValue* __fastcall GetTileValue(Tile *tile, UInt32 typeID)
{
	__asm
	{
		push	ebx
		push	esi
		push	edi
		mov		ebx, [ecx+0x14]
		xor		esi, esi
		mov		edi, [ecx+0x18]
		ALIGN 16
	iterHead:
		cmp		esi, edi
		jz		iterEnd
		lea		ecx, [esi+edi]
		shr		ecx, 1
		mov		eax, [ebx+ecx*4]
		cmp		[eax], edx
		jz		done
		jb		isLT
		mov		edi, ecx
		jmp		iterHead
		ALIGN 16
	isLT:
		lea		esi, [ecx+1]
		jmp		iterHead
	iterEnd:
		xor		eax, eax
	done:
		pop		edi
		pop		esi
		pop		ebx
		retn
	}
}

bool DataHandler::IsModLoaded(const char *modName)
{
	ListNode<ModInfo> *traverse = &modList.modInfoList;
	ModInfo *modInfo;
	do
	{
		modInfo = traverse->data;
		if (modInfo && !StrCompare(modInfo->name, modName))
			return true;
	}
	while (traverse = traverse->next);
	return false;
}