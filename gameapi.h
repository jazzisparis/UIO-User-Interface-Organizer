#pragma once

#include "utility.h"

struct String
{
	char		*m_data;
	UInt16		m_dataLen;
	UInt16		m_bufLen;

	void Set(const char *src);
};

template <typename T_Data> struct ListNode
{
	T_Data		*data;
	ListNode	*next;
};

template <typename T_Data> struct DListNode
{
	DListNode	*next;
	DListNode	*prev;
	T_Data		*data;
};

template <class Item> class DList
{
public:
	typedef DListNode<Item> Node;

private:
	Node		*first;
	Node		*last;
	UInt32		count;

public:
	bool Empty() const {return !first;}
	Node *Head() {return first;}
	Node *Tail() {return last;}
	UInt32 Size() const {return count;}
};

class NiTMap
{
public:
	struct Entry
	{
		Entry		*next;
		UInt32		key;
		UInt32		data;
	};

	virtual NiTMap	*Destructor(bool doFree);
	virtual UInt32	CalculateBucket(UInt32 key);
	virtual bool	Equal(UInt32 key1, UInt32 key2);
	virtual void	FillEntry(Entry *entry, UInt32 key, UInt32 data);
	virtual	void	Unk_04(void *arg0);
	virtual	void	Unk_05();
	virtual	void	Unk_06();

	UInt32		numBuckets;	// 04
	Entry		**buckets;	// 08
	UInt32		numItems;	// 0C

	Entry *Lookup(UInt32 key);
	bool Insert(UInt32 key, Entry **outEntry);
};

template <typename T_Data> class NiTArray
{
public:
	virtual NiTArray	*Destroy(bool doFree);

	T_Data		*data;			// 04
	UInt16		capacity;		// 08
	UInt16		firstFreeEntry;	// 0A
	UInt16		numObjs;		// 0C
	UInt16		growSize;		// 0E
};

class InterfaceManager;
class Tile;
class TileExtra;

enum TileActionType
{
	kAction_copy = 0x7D0,
	kAction_add,
	kAction_sub,
	kAction_mul,
	kAction_div,
	kAction_min,
	kAction_max,
	kAction_mod,
	kAction_floor,
	kAction_ceil,
	kAction_abs,
	kAction_round,
	kAction_gt,
	kAction_gte,
	kAction_eq,
	kAction_neq,
	kAction_lt,
	kAction_lte,
	kAction_and,
	kAction_or,
	kAction_not,
	kAction_onlyif,
	kAction_onlyifnot,
	kAction_ref,
	kAction_begin,
	kAction_end,
	kAction_neg,
	kAction_recipr,
	kAction_land,
	kAction_lor,
	kAction_shl,
	kAction_shr,
	kAction_bt,
	kAction_sqrt,
	kAction_pow,
	kAction_sin,
	kAction_cos,
	kAction_tan,
	kAction_asin,
	kAction_acos,
	kAction_atan,
	kAction_log,
	kAction_rand,
	kAction_MAX
};

struct TileValue
{
	UInt32		id;			// 00
	Tile		*parent;	// 04
	float		num;		// 08
	char		*str;		// 0C
	void		*action;	// 10
};

TileValue* __fastcall GetTileValue(Tile *tile, UInt32 typeID);

struct NiString
{
	const char		*str;
};

struct XMLtoTileData
{
	struct ParsedXMLTag
	{
		UInt32		tagType;	// 00
		float		value;		// 04
		String		nameOrSrc;	// 08
		union					// 10
		{
			UInt32	valueID;
			Tile	*tmplTile;
		};
	};

	struct TemplateData
	{
		NiString				templateName;	// 00
		TileExtra				*tileExtra;		// 04
		DList<ParsedXMLTag>		parsedTags;		// 08
	};

	TemplateData			*data;		// 00
	ListNode<TemplateData>	list04;		// 04
	UInt32					unk0C;		// 0C
	bool					donotFree;	// 10
	UInt8					pad11[3];	// 11

	void Destroy()
	{
		ThisCall(0xA0AD40, this);
		GameHeapFree(this);
	}
};