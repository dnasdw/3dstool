#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include "utility.h"

class CHuffman
{
public:
	static bool GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize);
	static u32 GetCompressBoundSize(u32 a_uUncompressedSize, n32 a_nCompressAlign);
	static bool Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize);
	static bool CompressH4(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign);
	static bool CompressH8(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign);
private:
	struct SData
	{
		u32 Freq;
		u16 No;
		n16 PaNo;
		n16 ChNo[2];
		u16 PaDepth;
		u16 LeafDepth;
		u32 HuffCode;
		u8 Bit;
		u8 Padding;
		u16 HWord;
	};
	struct STreeCtrlData
	{
		u8 LeftOffsetNeed;
		u8 RightOffsetNeed;
		u16 LeftNodeNo;
		u16 RightNodeNo;
	};
	struct SCompressionInfo
	{
		SData* Table;
		u8* Tree;
		STreeCtrlData* TreeCtrl;
		u8 TreeTop;
		u8 Padding[3];
	};
	CHuffman();
	static bool verifyHuffmanTable(const u8* a_pTree, u8 a_uBitSize);
	static bool compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, u8 a_uBitSize);
	static void initTable(SCompressionInfo* a_pInfo, void* a_pWork, u16 a_uDataNum);
	static void countData(SData* a_pTable, const u8* a_pSrc, u32 a_uSize, u8 a_uBitSize);
	static u16 constructTree(SData* a_pTable, u16 a_uDataNum);
	static void addParentDepthToTable(SData* a_pTable, u16 a_uLeftNo, u16 a_uRightNo);
	static void addCodeToTable(SData* a_pTable, u16 a_uNodeNo, u32 a_uPaHuffCode);
	static u8 addCountHWordToTable(SData* a_pTable, u16 a_uNodeNo);
	static void makeHuffTree(SCompressionInfo* a_pInfo, u16 a_uRootNo);
	static bool remainingNodeCanSetOffset(SCompressionInfo* a_pInfo, u8 a_uCostHWord);
	static void makeSubsetHuffTree(SCompressionInfo* a_pInfo, u16 a_uHuffTreeNo, u8 a_uRightNodeFlag);
	static void setOneNodeOffset(SCompressionInfo* a_pInfo, u16 a_uHuffTreeNo, u8 a_uRightNodeFlag);
	static const int s_nCompressWorkSize;
};

#endif	// HUFFMAN_H_
