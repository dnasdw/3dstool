#ifndef YAZ0_H_
#define YAZ0_H_

#include "utility.h"

class CYaz0
{
public:
	static bool GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize);
	static u32 GetCompressBoundSize(u32 a_uUncompressedSize, n32 a_nCompressAlign);
	static bool Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize);
	static bool Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, n32 a_nYaz0Align);
private:
	struct SCompressInfo
	{
		u16 WindowPos;
		u16 WindowLen;
		n16* OffsetTable;
		n16* ReversedOffsetTable;
		n16* ByteTable;
		n16* EndTable;
	};
	CYaz0();
	static void initTable(SCompressInfo* a_pInfo, void* a_pWork);
	static int search(SCompressInfo* a_pInfo, const u8* a_pSrc, int& a_nOffset, int a_nMaxSize);
	static inline void slide(SCompressInfo* a_pInfo, const u8* a_pSrc, int a_nSize);
	static void slideByte(SCompressInfo* a_pInfo, const u8* a_pSrc);
	static const int s_nCompressWorkSize;
};

#endif	// YAZ0_H_
