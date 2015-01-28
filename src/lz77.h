#ifndef LZ77_H_
#define LZ77_H_

#include "utility.h"

class CLZ77
{
public:
	static bool GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize);
	static u32 GetCompressBoundSize(u32 a_uCompressedSize, n32 a_nCompressAlign);
	static bool Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize);
	static bool CompressLZ(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign);
	static bool CompressLZEx(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign);
private:
	typedef n8 s8;
	typedef n16 s16;
	typedef n32 s32;
	typedef n64 s64;
	// Temporary information for LZ high-speed encoding
	struct LZCompressInfo
	{
		u16     windowPos;                 // Initial position of the history window
		u16     windowLen;                 // Length of the history window

		s16    *LZOffsetTable;             // Offset buffer of the history window
		s16    *LZByteTable;               // Pointer to the most recent character history
		s16    *LZEndTable;                // Pointer to the oldest character history
	};
	static const int LZ_COMPRESS_WORK_SIZE = (4096 + 256 + 256) * sizeof(s16);
	CLZ77();
	static bool compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, bool a_bExFormat);
	static void LZInitTable(LZCompressInfo * info, void *work);
	static u32 SearchLZ(LZCompressInfo * info, const u8 *nextp, u32 remainSize, u16 *offset, u32 maxLength);
	static void SlideByte(LZCompressInfo * info, const u8 *srcp);
	static inline void LZSlide(LZCompressInfo * info, const u8 *srcp, u32 n);
};

#endif	// LZ77_H_
