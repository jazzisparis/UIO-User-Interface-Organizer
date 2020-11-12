#include "gameapi.h"

extern UInt32 kHeapAllocAddr, kHeapFreeAddr, kAddrSub43A010;

__declspec(naked) void* __stdcall GameHeapAlloc(UInt32 size)
{
	__asm
	{
		mov		ecx, 0x11F6238
		jmp		kHeapAllocAddr
	}
}

__declspec(naked) void __stdcall GameHeapFree(void *ptr)
{
	__asm
	{
		mov		ecx, 0x11F6238
		jmp		kHeapFreeAddr
	}
}

__declspec(naked) void String::Set(const char *src)
{
	__asm
	{
		push	esi
		mov		esi, ecx
		mov		ecx, [esp+8]
		call	StrLen
		mov		[esi+4], ax
		test	ax, ax
		jz		nullStr
		mov		ecx, [esi]
		cmp		word ptr [esi+6], ax
		jnb		doCopy
		mov		[esi+6], ax
		inc		ax
		push	eax
		test	ecx, ecx
		jz		doAlloc
		push	ecx
		call	GameHeapFree
	doAlloc:
		call	GameHeapAlloc
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
		jmp		done
	nullStr:
		mov		eax, [esi]
		test	ecx, ecx
		jz		nullSrc
		test	eax, eax
		jz		done
		mov		[eax], 0
		jmp		done
	nullSrc:
		mov		word ptr [esi+6], 0
		test	eax, eax
		jz		done
		mov		dword ptr [esi], 0
		push	eax
		call	GameHeapFree
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
		mov		eax, [esp+0xC]
		push	eax
		mov		eax, [ecx]
		call	dword ptr [eax+4]
		mov		ecx, [esi+8]
		mov		edi, [ecx+eax*4]
	findEntry:
		test	edi, edi
		jz		done
		mov		eax, [esp+0xC]
		push	dword ptr [edi+4]
		push	eax
		mov		ecx, esi
		mov		eax, [ecx]
		call	dword ptr [eax+8]
		test	al, al
		jnz		done
		mov		edi, [edi]
		jmp		findEntry
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
		call	kAddrSub43A010
		mov		edi, eax
		mov		dword ptr [eax+8], 0
		mov		ecx, [ebp+8]
		cmp		esi, 0x11F3308
		jz		notStr
		call	StrLen
		inc		eax
		push	eax
		push	eax
		call	GameHeapAlloc
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
		mov		esp, ebp
		pop		ebp
		retn	8
	}
}

bool DataHandler::IsModLoaded(const char *modName)
{
	ListNode<ModInfo> *traverse = &modList.modInfoList;
	ModInfo *modInfo;
	do
	{
		modInfo = traverse->data;
		if (modInfo && StrEqualCI(modInfo->name, modName))
			return true;
	}
	while (traverse = traverse->next);
	return false;
}