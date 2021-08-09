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

class Tile;
class TileExtra;

struct TileValue
{
	UInt32		id;			// 00
	Tile		*parent;	// 04
	float		num;		// 08
	char		*str;		// 0C
	void		*action;	// 10
};

TileValue* __fastcall GetTileValue(Tile *tile, UInt32 typeID);

struct ModInfo
{
	struct ChunkHeader
	{
		UInt32	type : 4;
		UInt16	size : 2;
	};

	struct FormInfo
	{
		UInt32		recordType;
		UInt32		dataSize;
		UInt32		formFlags;
		UInt32		formID;
		UInt32		unk10;
		UInt16		formVersion;
		UInt16		unk16;
	};

	struct GroupInfo
	{
		UInt32		recordType;
		UInt32		groupSize;
		UInt32		groupLabel;
		UInt32		groupType;
		UInt32		unk10;
		UInt16		unk14;
		UInt16		unk16;
	};

	struct FileHeader
	{
		float	version;
		UInt32	recordCount;
		UInt32	nextObectID;
	};

	struct MasterSize
	{
		UInt32	low;
		UInt32	high;
	};

	ListNode<UInt32>					unkList;			// 000
	UInt32								*pointerMap;		// 008
	UInt32								unk00C;				// 00C
	void								*unkFile;			// 010
	UInt32								unk014;				// 014 
	void								*unk018;			// 018
	void								*unk01C;			// 01C
	char								name[0x104];		// 020
	char								filepath[0x104];	// 124
	UInt32								unk228;				// 228
	UInt32								unk22C;				// 22C
	UInt32								unk230;				// 230
	UInt32								unk234;				// 234
	UInt32								unk238;				// 238
	UInt32								unk23C;				// 23C
	FormInfo							formInfo;			// 240
	ChunkHeader							subRecordHeader;	// 258
	UInt32								unk260;				// 260
	UInt32								fileOffset;			// 264
	UInt32								dataOffset;			// 268
	UInt32								subrecordBytesRead;	// 26C
	FormInfo							writeInfo;			// 270
	UInt32								writeOffset;		// 288
	UInt32								bytesToWrite;		// 28C
	ListNode<UInt32>					list290;			// 290
	UInt8								unk298;				// 298
	UInt8								bIsBigEndian;		// 299
	UInt8								unk29A;				// 29A
	UInt8								pad29B;				// 29B
	WIN32_FIND_DATA						fileData;			// 29C
	FileHeader							header;				// 3DC
	UInt8								flags;				// 3E8
	UInt8								pad3E9[3];			// 3E9
	ListNode<char>						*refModNames;		// 3EC
	UInt32								unk3F0;				// 3F0
	ListNode<MasterSize>				*refModData;		// 3F4
	UInt32								unk3F8;				// 3F8
	UInt32								numRefMods;			// 3FC
	ModInfo								**refModInfo;		// 400
	UInt32								unk404;				// 404
	UInt32								unk408;				// 408
	UInt8								modIndex;			// 40C
	UInt8								pad40D[3];			// 40D
	String								author;				// 410
	String								description;		// 418
	void								*dataBuf;			// 420 
	UInt32								dataBufSize;		// 424
	UInt8								unk428;				// 428
	UInt8								pad429[3];			// 429
};

struct ModList
{
	ListNode<ModInfo>	modInfoList;		// 00
	UInt32				loadedModCount;		// 08
	ModInfo				*loadedMods[0xFF];	// 0C
};

class DataHandler
{
public:
	UInt32				unk00;					// 000
	void				*boundObjectList;		// 004
	ListNode<void>		packageList;			// 008
	ListNode<void>		worldSpaceList;			// 010
	ListNode<void>		climateList;			// 018
	ListNode<void>		imageSpaceList;			// 020
	ListNode<void>		imageSpaceModList;		// 028
	ListNode<void>		weatherList;			// 030
	ListNode<void>		enchantmentItemList;	// 038
	ListNode<void>		spellItemList;			// 040
	ListNode<void>		headPartList;			// 048
	ListNode<void>		hairList;				// 050
	ListNode<void>		eyeList;				// 058
	ListNode<void>		raceList;				// 060
	ListNode<void>		encounterZoneList;		// 068
	ListNode<void>		landTextureList;		// 070
	ListNode<void>		cameraShotList;			// 078
	ListNode<void>		classList;				// 080
	ListNode<void>		factionList;			// 088
	ListNode<void>		reputationList;			// 090
	ListNode<void>		challengeList;			// 098
	ListNode<void>		recipeList;				// 0A0
	ListNode<void>		recipeCategoryList;		// 0A8
	ListNode<void>		ammoEffectList;			// 0B0
	ListNode<void>		casinoList;				// 0B8
	ListNode<void>		caravanDeckList;		// 0C0
	ListNode<void>		scriptList;				// 0C8
	ListNode<void>		soundList;				// 0D0
	ListNode<void>		acousticSpaceList;		// 0D8
	ListNode<void>		ragdollList;			// 0E0
	ListNode<void>		globalList;				// 0E8
	ListNode<void>		voiceTypeList;			// 0F0
	ListNode<void>		impactDataList;			// 0F8
	ListNode<void>		impactDataSetList;		// 100
	ListNode<void>		topicList;				// 108
	ListNode<void>		topicInfoList;			// 110
	ListNode<void>		questList;				// 118
	ListNode<void>		combatStyleList;		// 120
	ListNode<void>		loadScreenList;			// 128
	ListNode<void>		waterFormList;			// 130
	ListNode<void>		effectShaderList;		// 138
	ListNode<void>		projectileList;			// 140
	ListNode<void>		explosionList;			// 148
	ListNode<void>		radiationStageList;		// 150
	ListNode<void>		dehydrationStageList;	// 158
	ListNode<void>		hungerStageList;		// 160
	ListNode<void>		sleepDepriveStageList;	// 168
	ListNode<void>		debrisList;				// 170
	ListNode<void>		perkList;				// 178
	ListNode<void>		bodyPartDataList;		// 180
	ListNode<void>		noteList;				// 188
	ListNode<void>		listFormList;			// 190
	ListNode<void>		menuIconList;			// 198
	ListNode<void>		anioList;				// 1A0
	ListNode<void>		messageList;			// 1A8
	ListNode<void>		lightingTemplateList;	// 1B0
	ListNode<void>		musicTypeList;			// 1B8
	ListNode<void>		loadScreenTypeList;		// 1C0
	ListNode<void>		mediaSetList;			// 1C8
	ListNode<void>		mediaLocControllerList;	// 1D0
	void				*regionList;			// 1D8
	NiTArray<void*>		cellArray;				// 1DC
	NiTArray<void*>		addonArray;				// 1EC

	UInt32				unk1FC[3];				// 1FC
	UInt32				nextCreatedRefID;		// 208
	UInt32				unk20C;					// 20C
	ModList				modList;				// 210
	UInt8				unk618;					// 618
	UInt8				unk619;					// 619
	UInt8				unk61A;					// 61A
	UInt8				unk61B;					// 61B
	UInt32				unk61C;					// 61C
	UInt8				unk620;					// 620
	UInt8				loading;				// 621
	UInt8				unk622;					// 622
	UInt8				unk623;					// 623
	void				*regionManager;			// 624
	void				*vendorContainer;		// 628
	UInt32				unk62C;					// 62C	
	UInt32				unk630;					// 630
	UInt32				unk634;					// 634
	UInt32				unk638;					// 638

	bool IsModLoaded(const char *modName);
};