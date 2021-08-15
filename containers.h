#pragma once

#include "utility.h"

#define MAP_MIN_BUCKET_COUNT		4UL
#define MAP_MAX_BUCKET_COUNT		0x40000UL
#define MAP_DEFAULT_BUCKET_COUNT	8UL
#define VECTOR_DEFAULT_ALLOC		8UL

void* __fastcall Pool_Alloc(UInt32 size);
void __fastcall Pool_Free(void *pBlock, UInt32 size);
void* __fastcall Pool_Realloc(void *pBlock, UInt32 curSize, UInt32 reqSize);
void* __fastcall Pool_Alloc_Buckets(UInt32 numBuckets);
UInt32 __fastcall AlignBucketCount(UInt32 count);

#define POOL_ALLOC(count, type) (type*)Pool_Alloc(count * sizeof(type))
#define POOL_FREE(block, count, type) Pool_Free(block, count * sizeof(type))
#define POOL_REALLOC(block, curCount, newCount, type) (type*)Pool_Realloc(block, curCount * sizeof(type), newCount * sizeof(type))

template <typename T_Data> __forceinline UInt32 AlignNumAlloc(UInt32 numAlloc)
{
	switch (sizeof(T_Data) & 0xF)
	{
		case 0:
			return numAlloc;
		case 2:
		case 6:
		case 0xA:
		case 0xE:
			if (numAlloc & 7)
			{
				numAlloc &= 0xFFFFFFF8;
				numAlloc += 8;
			}
			return numAlloc;
		case 4:
		case 0xC:
			if (numAlloc & 3)
			{
				numAlloc &= 0xFFFFFFFC;
				numAlloc += 4;
			}
			return numAlloc;
		case 8:
			if (numAlloc & 1)
				numAlloc++;
			return numAlloc;
		default:
			if (numAlloc & 0xF)
			{
				numAlloc &= 0xFFFFFFF0;
				numAlloc += 0x10;
			}
			return numAlloc;
	}
}

template <typename T_Key, typename T_Data> struct MappedPair
{
	T_Key		key;
	T_Data		value;
};

template <typename T_Key> __forceinline UInt32 HashKey(T_Key inKey)
{
	if (std::is_same_v<T_Key, char*> || std::is_same_v<T_Key, const char*>)
		return StrHash(*(const char**)&inKey);
	UInt32 uKey;
	if (sizeof(T_Key) == 1)
		uKey = *(UInt8*)&inKey;
	else if (sizeof(T_Key) == 2)
		uKey = *(UInt16*)&inKey;
	else
	{
		uKey = *(UInt32*)&inKey;
		if (sizeof(T_Key) > 4)
			uKey += uKey ^ ((UInt32*)&inKey)[1];
	}
	return (uKey * 0xD) ^ (uKey >> 0xF);
}

template <typename T_Key> class HashedKey
{
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;

	T_Key		key;

public:
	__forceinline bool Match(Key_Arg inKey, UInt32) const {return key == inKey;}
	__forceinline Key_Arg Get() const {return key;}
	__forceinline void Set(Key_Arg inKey, UInt32) {key = inKey;}
	__forceinline UInt32 GetHash() const {return HashKey<T_Key>(key);}
	__forceinline void Clear() {key.~T_Key();}
};

template <> class HashedKey<const char*>
{
	UInt32		hashVal;

public:
	__forceinline bool Match(const char*, UInt32 inHash) const {return hashVal == inHash;}
	__forceinline const char *Get() const {return "";}
	__forceinline void Set(const char*, UInt32 inHash) {hashVal = inHash;}
	__forceinline UInt32 GetHash() const {return hashVal;}
	__forceinline void Clear() {}
};

template <> class HashedKey<char*>
{
	UInt32		hashVal;
	char		*key;

public:
	__forceinline bool Match(char*, UInt32 inHash) const {return hashVal == inHash;}
	__forceinline char *Get() const {return key;}
	__forceinline void Set(char *inKey, UInt32 inHash)
	{
		hashVal = inHash;
		key = CopyString(inKey);
	}
	__forceinline UInt32 GetHash() const {return hashVal;}
	__forceinline void Clear() {free(key);}
};

template <typename T_Key, typename T_Data, const UInt32 _default_bucket_count = MAP_DEFAULT_BUCKET_COUNT> class UnorderedMap
{
	using H_Key = HashedKey<T_Key>;
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;
	using M_Pair = const MappedPair<T_Key, T_Data>;

	struct Entry
	{
		Entry		*next;
		H_Key		key;
		T_Data		value;

		void Clear()
		{
			key.Clear();
			value.~T_Data();
		}
	};

	struct Bucket
	{
		Entry	*entries;

		void Insert(Entry *entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Remove(Entry *entry, Entry *prev)
		{
			if (prev) prev->next = entry->next;
			else entries = entry->next;
			entry->Clear();
			POOL_FREE(entry, 1, Entry);
		}

		void Clear()
		{
			if (!entries) return;
			Entry *pEntry;
			do
			{
				pEntry = entries;
				entries = entries->next;
				pEntry->Clear();
				POOL_FREE(pEntry, 1, Entry);
			}
			while (entries);
		}

		UInt32 Size() const
		{
			if (!entries) return 0;
			UInt32 size = 1;
			Entry *pEntry = entries;
			while (pEntry = pEntry->next)
				size++;
			return size;
		}
	};

	Bucket		*buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numEntries;		// 08

	Bucket *End() const {return buckets + numBuckets;}

	__declspec(noinline) void ResizeTable(UInt32 newCount)
	{
		Bucket *pBucket = buckets, *pEnd = End(), *newBuckets = (Bucket*)Pool_Alloc_Buckets(newCount);
		Entry *pEntry, *pTemp;
		newCount--;
		do
		{
			pEntry = pBucket->entries;
			while (pEntry)
			{
				pTemp = pEntry;
				pEntry = pEntry->next;
				newBuckets[pTemp->key.GetHash() & newCount].Insert(pTemp);
			}
			pBucket++;
		}
		while (pBucket != pEnd);
		POOL_FREE(buckets, numBuckets, Bucket);
		buckets = newBuckets;
		numBuckets = newCount + 1;
	}

	Entry *FindEntry(Key_Arg key) const
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			for (Entry *pEntry = buckets[hashVal & (numBuckets - 1)].entries; pEntry; pEntry = pEntry->next)
				if (pEntry->key.Match(key, hashVal)) return pEntry;
		}
		return nullptr;
	}

	bool InsertKey(Key_Arg key, T_Data **outData)
	{
		if (!buckets)
		{
			numBuckets = AlignBucketCount(numBuckets);
			buckets = (Bucket*)Pool_Alloc_Buckets(numBuckets);
		}
		else if ((numEntries > numBuckets) && (numBuckets < MAP_MAX_BUCKET_COUNT))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = HashKey<T_Key>(key);
		Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry *pEntry = pBucket->entries; pEntry; pEntry = pEntry->next)
		{
			if (!pEntry->key.Match(key, hashVal)) continue;
			*outData = &pEntry->value;
			return false;
		}
		numEntries++;
		Entry *newEntry = POOL_ALLOC(1, Entry);
		newEntry->key.Set(key, hashVal);
		pBucket->Insert(newEntry);
		*outData = &newEntry->value;
		return true;
	}

public:
	UnorderedMap(UInt32 _numBuckets = _default_bucket_count) : buckets(nullptr), numBuckets(_numBuckets), numEntries(0) {}
	UnorderedMap(M_Pair *inList, UInt32 size) : buckets(nullptr), numBuckets(size), numEntries(0) {InsertList(inList, size);}
	~UnorderedMap()
	{
		if (!buckets) return;
		Clear();
		POOL_FREE(buckets, numBuckets, Bucket);
		buckets = nullptr;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	UInt32 BucketCount() const {return numBuckets;}

	void SetBucketCount(UInt32 newCount)
	{
		if (buckets)
		{
			newCount = AlignBucketCount(newCount);
			if ((numBuckets != newCount) && (numEntries <= newCount))
				ResizeTable(newCount);
		}
		else numBuckets = newCount;
	}

	float LoadFactor() const {return (float)numEntries / (float)numBuckets;}

	bool Insert(Key_Arg key, T_Data **outData)
	{
		if (InsertKey(key, outData))
		{
			new (*outData) T_Data();
			return true;
		}
		return false;
	}

	T_Data& operator[](Key_Arg key)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data();
		return *outData;
	}

	template <typename ...Args>
	T_Data* Emplace(Key_Arg key, Args&& ...args)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data(std::forward<Args>(args)...);
		return outData;
	}

	Data_Arg InsertNotIn(Key_Arg key, Data_Arg value)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			*outData = value;
		return value;
	}

	void InsertList(M_Pair *inList, UInt32 size)
	{
		T_Data *outData;
		for (; size; size--, inList++)
		{
			InsertKey(inList->key, &outData);
			*outData = inList->value;
		}
	}

	bool HasKey(Key_Arg key) const {return FindEntry(key) ? true : false;}

	T_Data Get(Key_Arg key)
	{
		Entry *pEntry = FindEntry(key);
		return pEntry ? pEntry->value : NULL;
	}

	T_Data* GetPtr(Key_Arg key)
	{
		Entry *pEntry = FindEntry(key);
		return pEntry ? &pEntry->value : nullptr;
	}

	bool Erase(Key_Arg key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = nullptr;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					numEntries--;
					pBucket->Remove(pEntry, prev);
					return true;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return false;
	}

	T_Data GetErase(Key_Arg key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = nullptr;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					T_Data outVal = pEntry->value;
					numEntries--;
					pBucket->Remove(pEntry, prev);
					return outVal;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return NULL;
	}

	bool Clear()
	{
		if (!numEntries) return false;
		Bucket *pBucket = buckets, *pEnd = End();
		do
		{
			pBucket->Clear();
			pBucket++;
		}
		while (pBucket != pEnd);
		numEntries = 0;
		return true;
	}

	class Iterator
	{
		friend UnorderedMap;

		UnorderedMap	*table;
		Bucket			*bucket;
		Entry			*entry;

		void FindNonEmpty()
		{
			for (Bucket *pEnd = table->End(); bucket != pEnd; bucket++)
				if (entry = bucket->entries) return;
		}

	public:
		void Init(UnorderedMap &_table)
		{
			table = &_table;
			entry = nullptr;
			if (table->numEntries)
			{
				bucket = table->buckets;
				FindNonEmpty();
			}
		}

		void Find(Key_Arg key)
		{
			if (!table->numEntries)
			{
				entry = nullptr;
				return;
			}
			UInt32 hashVal = HashKey<T_Key>(key);
			bucket = &table->buckets[hashVal & (table->numBuckets - 1)];
			entry = bucket->entries;
			while (entry)
			{
				if (entry->key.Match(key, hashVal))
					break;
				entry = entry->next;
			}
		}

		UnorderedMap* Table() const {return table;}
		Key_Arg Key() const {return entry->key.Get();}
		Data_Arg operator()() const {return entry->value;}
		T_Data& Ref() {return entry->value;}
		Data_Arg operator*() const {return entry->value;}
		Data_Arg operator->() const {return entry->value;}

		explicit operator bool() const {return entry != nullptr;}
		void operator++()
		{
			if (entry)
				entry = entry->next;
			else entry = bucket->entries;
			if (!entry && table->numEntries)
			{
				bucket++;
				FindNonEmpty();
			}
		}

		bool IsValid()
		{
			if (entry)
			{
				for (Entry *temp = bucket->entries; temp; temp = temp->next)
					if (temp == entry) return true;
				entry = nullptr;
			}
			return false;
		}

		void Remove()
		{
			Entry *curr = bucket->entries, *prev = nullptr;
			do
			{
				if (curr == entry) break;
				prev = curr;
			}
			while (curr = curr->next);
			table->numEntries--;
			bucket->Remove(entry, prev);
			entry = prev;
		}

		Iterator(UnorderedMap *_table = nullptr) : table(_table), entry(nullptr) {}
		Iterator(UnorderedMap &_table) {Init(_table);}
		Iterator(UnorderedMap &_table, Key_Arg key) : table(&_table) {Find(key);}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator Find(Key_Arg key) {return Iterator(*this, key);}
};

template <typename T_Key, const UInt32 _default_bucket_count = MAP_DEFAULT_BUCKET_COUNT> class UnorderedSet
{
	using H_Key = HashedKey<T_Key>;
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;

	struct Entry
	{
		Entry		*next;
		H_Key		key;

		void Clear() {key.Clear();}
	};

	struct Bucket
	{
		Entry	*entries;

		void Insert(Entry *entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Remove(Entry *entry, Entry *prev)
		{
			if (prev) prev->next = entry->next;
			else entries = entry->next;
			entry->Clear();
			POOL_FREE(entry, 1, Entry);
		}

		void Clear()
		{
			if (!entries) return;
			Entry *pEntry;
			do
			{
				pEntry = entries;
				entries = entries->next;
				pEntry->Clear();
				POOL_FREE(pEntry, 1, Entry);
			}
			while (entries);
		}

		UInt32 Size() const
		{
			if (!entries) return 0;
			UInt32 size = 1;
			Entry *pEntry = entries;
			while (pEntry = pEntry->next)
				size++;
			return size;
		}
	};

	Bucket		*buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numEntries;		// 08

	Bucket *End() const {return buckets + numBuckets;}

	__declspec(noinline) void ResizeTable(UInt32 newCount)
	{
		Bucket *pBucket = buckets, *pEnd = End(), *newBuckets = (Bucket*)Pool_Alloc_Buckets(newCount);
		Entry *pEntry, *pTemp;
		newCount--;
		do
		{
			pEntry = pBucket->entries;
			while (pEntry)
			{
				pTemp = pEntry;
				pEntry = pEntry->next;
				newBuckets[pTemp->key.GetHash() & newCount].Insert(pTemp);
			}
			pBucket++;
		}
		while (pBucket != pEnd);
		POOL_FREE(buckets, numBuckets, Bucket);
		buckets = newBuckets;
		numBuckets = newCount + 1;
	}

public:
	UnorderedSet(UInt32 _numBuckets = _default_bucket_count) : buckets(nullptr), numBuckets(_numBuckets), numEntries(0) {}
	UnorderedSet(T_Key *inList, UInt32 size) : buckets(nullptr), numBuckets(size), numEntries(0) {InsertList(inList, size);}
	~UnorderedSet()
	{
		if (!buckets) return;
		Clear();
		POOL_FREE(buckets, numBuckets, Bucket);
		buckets = nullptr;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	UInt32 BucketCount() const {return numBuckets;}

	void SetBucketCount(UInt32 newCount)
	{
		if (buckets)
		{
			newCount = AlignBucketCount(newCount);
			if ((numBuckets != newCount) && (numEntries <= newCount))
				ResizeTable(newCount);
		}
		else numBuckets = newCount;
	}

	float LoadFactor() const {return (float)numEntries / (float)numBuckets;}

	bool Insert(Key_Arg key)
	{
		if (!buckets)
		{
			numBuckets = AlignBucketCount(numBuckets);
			buckets = (Bucket*)Pool_Alloc_Buckets(numBuckets);
		}
		else if ((numEntries > numBuckets) && (numBuckets < MAP_MAX_BUCKET_COUNT))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = HashKey<T_Key>(key);
		Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry *pEntry = pBucket->entries; pEntry; pEntry = pEntry->next)
			if (pEntry->key.Match(key, hashVal)) return false;
		numEntries++;
		Entry *newEntry = POOL_ALLOC(1, Entry);
		newEntry->key.Set(key, hashVal);
		pBucket->Insert(newEntry);
		return true;
	}

	void InsertList(T_Key *inList, UInt32 size)
	{
		for (; size; size--, inList++)
			Insert(*inList);
	}

	bool HasKey(Key_Arg key) const
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			for (Entry *pEntry = buckets[hashVal & (numBuckets - 1)].entries; pEntry; pEntry = pEntry->next)
				if (pEntry->key.Match(key, hashVal)) return true;
		}
		return false;
	}

	bool Erase(Key_Arg key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = nullptr;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					numEntries--;
					pBucket->Remove(pEntry, prev);
					return true;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return false;
	}

	bool Clear()
	{
		if (!numEntries) return false;
		Bucket *pBucket = buckets, *pEnd = End();
		do
		{
			pBucket->Clear();
			pBucket++;
		}
		while (pBucket != pEnd);
		numEntries = 0;
		return true;
	}

	class Iterator
	{
		friend UnorderedSet;

		UnorderedSet	*table;
		Bucket			*bucket;
		Entry			*entry;

		void FindNonEmpty()
		{
			for (Bucket *pEnd = table->End(); bucket != pEnd; bucket++)
				if (entry = bucket->entries) return;
		}

	public:
		void Init(UnorderedSet &_table)
		{
			table = &_table;
			entry = nullptr;
			if (table->numEntries)
			{
				bucket = table->buckets;
				FindNonEmpty();
			}
		}

		Key_Arg operator*() const {return entry->key.Get();}
		Key_Arg operator->() const {return entry->key.Get();}

		explicit operator bool() const {return entry != nullptr;}
		void operator++()
		{
			if ((entry = entry->next) || !table->numEntries)
				return;
			bucket++;
			FindNonEmpty();
		}

		Iterator() : table(nullptr), entry(nullptr) {}
		Iterator(UnorderedSet &_table) {Init(_table);}
	};

	Iterator Begin() {return Iterator(*this);}
};

template <typename T_Data> class Vector
{
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	T_Data		*data;		// 00
	UInt32		numItems;	// 04
	UInt32		numAlloc;	// 08

	__declspec(noinline) T_Data *AllocateData()
	{
		if (!data)
		{
			numAlloc = AlignNumAlloc<T_Data>(numAlloc);
			data = POOL_ALLOC(numAlloc, T_Data);
		}
		else if (numAlloc <= numItems)
		{
			UInt32 newAlloc = numAlloc << 1;
			data = POOL_REALLOC(data, numAlloc, newAlloc, T_Data);
			numAlloc = newAlloc;
		}
		return data + numItems++;
	}

	T_Data *End() const {return data + numItems;}

public:
	Vector(UInt32 _alloc = VECTOR_DEFAULT_ALLOC) : data(nullptr), numItems(0), numAlloc(_alloc) {}
	Vector(std::initializer_list<T_Data> inList) : data(nullptr), numItems(0), numAlloc(inList.size()) {AppendList(inList);}
	~Vector()
	{
		if (!data) return;
		Clear();
		POOL_FREE(data, numAlloc, T_Data);
		data = nullptr;
	}

	UInt32 Size() const {return numItems;}
	bool Empty() const {return !numItems;}

	T_Data* Data() const {return data;}

	T_Data& operator[](UInt32 index) const {return data[index];}

	T_Data *GetPtr(UInt32 index) const {return (index < numItems) ? (data + index) : nullptr;}

	Data_Arg Top() const {return data[numItems - 1];}

	T_Data* Append(Data_Arg item)
	{
		T_Data *pData = AllocateData();
		memcpy(pData, &item, sizeof(T_Data));
		return pData;
	}

	template <typename ...Args>
	T_Data* Append(Args&& ...args)
	{
		T_Data *pData = AllocateData();
		new (pData) T_Data(std::forward<Args>(args)...);
		return pData;
	}

	void AppendList(std::initializer_list<T_Data> inList)
	{
		for (auto iter = inList.begin(); iter != inList.end(); ++iter)
			Append(*iter);
	}

	T_Data* Insert(UInt32 index, Data_Arg item)
	{
		if (index > numItems)
			index = numItems;
		UInt32 size = numItems - index;
		T_Data *pData = AllocateData();
		if (size)
		{
			pData = data + index;
			memmove(pData + 1, pData, sizeof(T_Data) * size);
		}
		memcpy(pData, &item, sizeof(T_Data));
		return pData;
	}

	template <typename ...Args>
	T_Data* Insert(UInt32 index, Args&& ...args)
	{
		if (index > numItems)
			index = numItems;
		UInt32 size = numItems - index;
		T_Data *pData = AllocateData();
		if (size)
		{
			pData = data + index;
			memmove(pData + 1, pData, sizeof(T_Data) * size);
		}
		new (pData) T_Data(std::forward<Args>(args)...);
		return pData;
	}

	SInt32 GetIndexOf(Data_Arg item) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				if (*pData == item)
					return pData - data;
				pData++;
			}
			while (pData != pEnd);
		}
		return -1;
	}

	template <class Matcher>
	SInt32 GetIndexOf(Matcher &matcher) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				if (matcher(*pData))
					return pData - data;
				pData++;
			}
			while (pData != pEnd);
		}
		return -1;
	}

	template <class Matcher>
	T_Data* Find(Matcher &matcher) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				if (matcher(*pData))
					return pData;
				pData++;
			}
			while (pData != pEnd);
		}
		return nullptr;
	}

	bool RemoveNth(UInt32 index)
	{
		if (index >= numItems) return false;
		T_Data *pData = data + index;
		pData->~T_Data();
		numItems--;
		index = numItems - index;
		if (index) memmove(pData, pData + 1, sizeof(T_Data) * index);
		return true;
	}

	bool Remove(Data_Arg item)
	{
		if (numItems)
		{
			T_Data *pData = End(), *pEnd = data;
			do
			{
				pData--;
				if (*pData != item) continue;
				numItems--;
				pData->~T_Data();
				UInt32 size = (UInt32)End() - (UInt32)pData;
				if (size) memmove(pData, pData + 1, size);
				return true;
			}
			while (pData != pEnd);
		}
		return false;
	}

	template <class Matcher>
	UInt32 Remove(Matcher &matcher)
	{
		if (numItems)
		{
			T_Data *pData = End(), *pEnd = data;
			UInt32 removed = 0, size;
			do
			{
				pData--;
				if (!matcher(*pData)) continue;
				numItems--;
				pData->~T_Data();
				size = (UInt32)End() - (UInt32)pData;
				if (size) memmove(pData, pData + 1, size);
				removed++;
			}
			while (pData != pEnd);
			return removed;
		}
		return 0;
	}

	void Pop()
	{
		if (numItems)
			data[--numItems].~T_Data();
	}

	void Clear()
	{
		if (!numItems) return;
		T_Data *pData = data, *pEnd = End();
		do
		{
			pData->~T_Data();
			pData++;
		}
		while (pData != pEnd);
		numItems = 0;
	}

	class Iterator
	{
		friend Vector;

		T_Data		*pData;
		UInt32		count;

		Iterator() {}

	public:
		explicit operator bool() const {return count != 0;}
		void operator++()
		{
			pData++;
			count--;
		}
		void operator+=(UInt32 _count)
		{
			if (_count >= count)
				count = 0;
			else
			{
				pData += _count;
				count -= _count;
			}
		}

		Data_Arg operator*() const {return *pData;}
		Data_Arg operator->() const {return *pData;}
		Data_Arg operator()() const {return *pData;}
		T_Data& Ref() {return *pData;}

		Iterator(Vector &source) : pData(source.data), count(source.numItems) {}
		Iterator(Vector &source, UInt32 index) : count(source.numItems)
		{
			if (count <= index)
				count = 0;
			else
			{
				pData = source.data + index;
				count -= index;
			}
		}
	};

	class RvIterator : public Iterator
	{
	public:
		void operator--()
		{
			pData--;
			count--;
		}
		void operator-=(UInt32 _count)
		{
			if (_count >= count)
				count = 0;
			else
			{
				pData -= _count;
				count -= _count;
			}
		}

		void Remove(Vector &source)
		{
			pData->~T_Data();
			UInt32 size = source.numItems - count;
			source.numItems--;
			if (size) memmove(pData, pData + 1, size * sizeof(T_Data));
		}

		RvIterator(Vector &source)
		{
			if (count = source.numItems)
				pData = source.data + (count - 1);
		}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator BeginAt(UInt32 index) {return Iterator(*this, index);}

	RvIterator BeginRv() {return RvIterator(*this);}
};