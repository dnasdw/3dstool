#include "huffman.h"

const int CHuffman::s_nCompressWorkSize = 12288 + 512 + 1536;

bool CHuffman::GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize > 4 && (a_pCompressed[0] & 0xF0) == 0x20 && ((a_pCompressed[0] & 0x0F) == 4 || (a_pCompressed[0] & 0x0F) == 8))
	{
		a_uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed) >> 8 & 0xFFFFFF;
		if (a_uUncompressedSize != 0 || a_uCompressedSize > 8)
		{
			if (a_uUncompressedSize == 0)
			{
				a_uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed + 4);
			}
			a_uUncompressedSize = static_cast<u32>(Align(a_uUncompressedSize, 4));
		}
		else
		{
			bResult = false;
		}
	}
	else
	{
		bResult = false;
	}
	return bResult;
}

u32 CHuffman::GetCompressBoundSize(u32 a_uUncompressedSize, n32 a_nCompressAlign)
{
	return ((a_uUncompressedSize != 0 && a_uUncompressedSize <= 0xFFFFFF ? 4 : 8) + 512 + a_uUncompressedSize * 8 + a_nCompressAlign - 1) / a_nCompressAlign * a_nCompressAlign;
}

bool CHuffman::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize > 4 && (a_pCompressed[0] & 0xF0) == 0x20 && ((a_pCompressed[0] & 0x0F) == 4 || (a_pCompressed[0] & 0x0F) == 8))
	{
		u32 uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed) >> 8 & 0xFFFFFF;
		n32 nHeaderSize = 4;
		if (uUncompressedSize != 0 || a_uCompressedSize > 8)
		{
			if (uUncompressedSize == 0)
			{
				uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed + 4);
				nHeaderSize = 8;
			}
			const u8* pTree = a_pCompressed + nHeaderSize;
			n32 nTreeSize = (*pTree + 1) * 2;
			u8 uBitSize = a_pCompressed[0] & 0x0F;
			if (a_uUncompressedSize >= uUncompressedSize && static_cast<u32>(nHeaderSize + nTreeSize) <= a_uCompressedSize && verifyHuffmanTable(pTree, uBitSize))
			{
				a_uUncompressedSize = uUncompressedSize;
				const u32* pSrc = reinterpret_cast<const u32*>(a_pCompressed + nHeaderSize + nTreeSize);
				u32* pDest = reinterpret_cast<u32*>(a_pUncompressed);
				pTree = a_pCompressed + nHeaderSize + 1;
				u32 uDestTmp = 0;
				n32 nDestTmpCount = 0;
				u32 uDestTmpDataNum = 4 + (uBitSize & 7);
				while (a_pUncompressed + a_uUncompressedSize - reinterpret_cast<u8*>(pDest) > 0)
				{
					if (a_pCompressed + a_uCompressedSize - reinterpret_cast<const u8*>(pSrc) < 4)
					{
						bResult = false;
						break;
					}
					u32 uSrcTmp = *pSrc++;
					n32 nSrcTmpCount = 32;
					while (--nSrcTmpCount >= 0)
					{
						u32 uTreeShift = uSrcTmp >> 31 & 1;
						u32 uTreeCheck = *pTree;
						uTreeCheck <<= uTreeShift;
						pTree = reinterpret_cast<const u8*>(reinterpret_cast<size_t>(pTree) / 2 * 2 + ((*pTree & 0x3F) + 1) * 2 + uTreeShift);
						if ((uTreeCheck & 0x80) != 0)
						{
							uDestTmp >>= uBitSize;
							uDestTmp |= *pTree << (32 - uBitSize);
							pTree = a_pCompressed + nHeaderSize + 1;
							nDestTmpCount++;
							if (a_pUncompressed + a_uUncompressedSize - reinterpret_cast<u8*>(pDest) <= nDestTmpCount * uBitSize / 8)
							{
								uDestTmp >>= (uDestTmpDataNum - nDestTmpCount) * uBitSize;
								nDestTmpCount = uDestTmpDataNum;
							}
							if (nDestTmpCount == uDestTmpDataNum)
							{
								*pDest++ = uDestTmp;
								nDestTmpCount = 0;
							}
						}
						if (a_pUncompressed + a_uUncompressedSize - reinterpret_cast<u8*>(pDest) <= 0)
						{
							break;
						}
						uSrcTmp <<= 1;
					}
				}
			}
			else
			{
				bResult = false;
			}
		}
		else
		{
			bResult = false;
		}
	}
	else
	{
		bResult = false;
	}
	return bResult;
}

bool CHuffman::CompressH4(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign)
{
	return compress(a_pUncompressed, a_uUncompressedSize, a_pCompressed, a_uCompressedSize, a_nCompressAlign, 4);
}

bool CHuffman::CompressH8(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign)
{
	return compress(a_pUncompressed, a_uUncompressedSize, a_pCompressed, a_uCompressedSize, a_nCompressAlign, 8);
}

CHuffman::CHuffman()
{
}

bool CHuffman::verifyHuffmanTable(const u8* a_pTree, u8 a_uBitSize)
{
	n32 nTreeSize = *a_pTree;
	if (a_uBitSize == 4 && nTreeSize >= 0x10)
	{
		return false;
	}
	const u8* pTreeEnd = a_pTree + (nTreeSize + 1) * 2;
	u8 uEndFlags[512 / 8] = {};
	n32 nIdx = 1;
	for (const u8* pTree = a_pTree + 1; pTree < pTreeEnd; pTree++, nIdx++)
	{
		if ((uEndFlags[nIdx / 8] & (1 << (nIdx % 8))) == 0)
		{
			if (*pTree == 0 && nIdx >= nTreeSize * 2)
			{
				continue;
			}
			n32 nOffset = ((*pTree & 0x3F) + 1) * 2;
			const u8* pNode = reinterpret_cast<const u8*>(reinterpret_cast<size_t>(pTree) / 2 * 2 + nOffset);
			if (pNode >= pTreeEnd)
			{
				return false;
			}
			if ((*pTree & 0x80) != 0)
			{
				n32 nLeft = nIdx / 2 * 2 + nOffset;
				uEndFlags[nLeft / 8] |= 1 << (nLeft % 8);
			}
			if ((*pTree & 0x40) != 0)
			{
				n32 nRight = nIdx / 2 * 2 + nOffset + 1;
				uEndFlags[nRight / 8] |= 1 << (nRight % 8);
			}
		}
	}
	return true;
}

bool CHuffman::compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, u8 a_uBitSize)
{
	bool bResult = true;
	u8* pWork = new u8[s_nCompressWorkSize];
	do
	{
		n32 nHeaderSize = 4;
		if (a_uUncompressedSize != 0 && a_uUncompressedSize <= 0xFFFFFF)
		{
			if (a_uCompressedSize < 4)
			{
				bResult = false;
				break;
			}
			*reinterpret_cast<u32*>(a_pCompressed) = a_uUncompressedSize << 8 | 0x20 | a_uBitSize;
		}
		else
		{
			if (a_uCompressedSize < 8)
			{
				bResult = false;
				break;
			}
			*reinterpret_cast<u32*>(a_pCompressed) = 0x20 | a_uBitSize;
			*reinterpret_cast<u32*>(a_pCompressed + 4) = a_uUncompressedSize;
			nHeaderSize = 8;
		}
		u16 uDataNum = 1 << a_uBitSize;
		SCompressionInfo info;
		initTable(&info, pWork, uDataNum);
		countData(info.Table, a_pUncompressed, a_uUncompressedSize, a_uBitSize);
		u16 uRootNo = constructTree(info.Table, uDataNum);
		makeHuffTree(&info, uRootNo);
		info.TreeTop = static_cast<u8>(Align(info.TreeTop, 2));
		info.Tree[0] = --info.TreeTop;
		const u8* pSrc = a_pUncompressed;
		u8* pDest = a_pCompressed + nHeaderSize;
		if (a_pCompressed + a_uCompressedSize - pDest < (info.TreeTop + 1) * 2)
		{
			bResult = false;
			break;
		}
		for (int i = 0; i < (info.TreeTop + 1) * 2; i++)
		{
			*pDest++ = info.Tree[i];
		}
		u32 uBitStream = 0;
		n32 nStreamLength = 0;
		while (a_pUncompressed + a_uUncompressedSize - pSrc > 0)
		{
			u8 uVal = *pSrc++;
			if (a_uBitSize == 8)
			{
				uBitStream = uBitStream << info.Table[uVal].PaDepth | info.Table[uVal].HuffCode;
				nStreamLength += info.Table[uVal].PaDepth;
				if (a_pCompressed + a_uCompressedSize - pDest < nStreamLength / 8)
				{
					bResult = false;
					break;
				}
				for (int i = 0; i < nStreamLength / 8; i++)
				{
					*pDest++ = uBitStream >> (nStreamLength - (i + 1) * 8);
				}
				nStreamLength %= 8;
			}
			else
			{
				u8 uSrcTmp = 0;
				for (int i = 0; i < 2; i++)
				{
					if (i == 0)
					{
						uSrcTmp = uVal & 0x0F;
					}
					else
					{
						uSrcTmp = uVal >> 4 & 0x0F;
					}
					uBitStream = uBitStream << info.Table[uSrcTmp].PaDepth | info.Table[uSrcTmp].HuffCode;
					nStreamLength += info.Table[uSrcTmp].PaDepth;
					if (a_pCompressed + a_uCompressedSize - pDest < nStreamLength / 8)
					{
						bResult = false;
						break;
					}
					for (int j = 0; j < nStreamLength / 8; j++)
					{
						*pDest++ = uBitStream >> (nStreamLength - (j + 1) * 8);
					}
					nStreamLength %= 8;
				}
			}
		}
		if (!bResult)
		{
			break;
		}
		if (nStreamLength != 0)
		{
			if (a_pCompressed + a_uCompressedSize - pDest < 1)
			{
				bResult = false;
				break;
			}
			*pDest++ = uBitStream << (8 - nStreamLength);
		}
		while ((pDest - a_pCompressed) % 4 != 0)
		{
			if (a_pCompressed + a_uCompressedSize - pDest < 1)
			{
				bResult = false;
				break;
			}
			*pDest++ = 0;
		}
		if (!bResult)
		{
			break;
		}
		for (u32 i = nHeaderSize + (info.TreeTop + 1) * 2; i < static_cast<u32>(pDest - a_pCompressed); i += 4)
		{
			*reinterpret_cast<u32*>(a_pCompressed + i) = SDW_CONVERT_ENDIAN32(*reinterpret_cast<u32*>(a_pCompressed + i));
		}
		while ((pDest - a_pCompressed) % a_nCompressAlign != 0)
		{
			if (a_pCompressed + a_uCompressedSize - pDest < 1)
			{
				bResult = false;
				break;
			}
			*pDest++ = 0;
		}
		if (!bResult)
		{
			break;
		}
		a_uCompressedSize = static_cast<u32>(pDest - a_pCompressed);
	} while (false);
	delete[] pWork;
	return bResult;
}

void CHuffman::initTable(SCompressionInfo* a_pInfo, void* a_pWork, u16 a_uDataNum)
{
	a_pInfo->Table = reinterpret_cast<SData*>(a_pWork);
	a_pInfo->Tree = reinterpret_cast<u8*>(a_pWork) + sizeof(SData) * 512;
	a_pInfo->TreeCtrl = reinterpret_cast<STreeCtrlData*>(reinterpret_cast<u8*>(a_pWork) + sizeof(SData) * 512 + 512);
	a_pInfo->TreeTop = 1;
	SData* pTable = a_pInfo->Table;
	const SData tableInitData = { 0, 0, 0, { -1, -1 }, 0, 0, 0, 0, 0, 0 };
	for (int i = 0; i < a_uDataNum * 2; i++)
	{
		pTable[i] = tableInitData;
		pTable[i].No = static_cast<u16>(i);
	}
	u8* pTree = a_pInfo->Tree;
	STreeCtrlData* pTreeCtrl = a_pInfo->TreeCtrl;
	const STreeCtrlData treeCtrlInitData = { 1, 1, 0, 0 };
	for (int i = 0; i < 256; i++)
	{
		pTree[i * 2] = 0;
		pTree[i * 2 + 1] = 0;
		pTreeCtrl[i] = treeCtrlInitData;
	}
}

void CHuffman::countData(SData* a_pTable, const u8* a_pSrc, u32 a_uSize, u8 a_uBitSize)
{
	if (a_uBitSize == 8)
	{
		for (u32 i = 0; i < a_uSize; i++)
		{
			a_pTable[a_pSrc[i]].Freq++;
		}
	}
	else
	{
		for (u32 i = 0; i < a_uSize; i++)
		{
			a_pTable[a_pSrc[i] >> 4 & 0x0F].Freq++;
			a_pTable[a_pSrc[i] & 0x0F].Freq++;
		}
	}
}

u16 CHuffman::constructTree(SData* a_pTable, u16 a_uDataNum)
{
	u16 uRootNo = 0;
	u16 uTableTop = a_uDataNum;
	while (true)
	{
		n32 nLeftNo = -1;
		n32 nRightNo = -1;
		for (int i = 0; i < uTableTop; i++)
		{
			if (a_pTable[i].Freq == 0 || a_pTable[i].PaNo != 0)
			{
				continue;
			}
			if (nLeftNo < 0)
			{
				nLeftNo = i;
			}
			else if (a_pTable[i].Freq < a_pTable[nLeftNo].Freq)
			{
				nLeftNo = i;
			}
		}
		for (int i = 0; i < uTableTop; i++)
		{
			if (a_pTable[i].Freq == 0 || a_pTable[i].PaNo != 0 || i == nLeftNo)
			{
				continue;
			}
			if (nRightNo < 0)
			{
				nRightNo = i;
			}
			else if (a_pTable[i].Freq < a_pTable[nRightNo].Freq)
			{
				nRightNo = i;
			}
		}
		if (nRightNo < 0)
		{
			if (uTableTop == a_uDataNum)
			{
				if (nLeftNo < 0)
				{
					nLeftNo = 0;
				}
				a_pTable[uTableTop].Freq = a_pTable[nLeftNo].Freq;
				a_pTable[uTableTop].ChNo[0] = static_cast<n16>(nLeftNo);
				a_pTable[uTableTop].ChNo[1] = static_cast<n16>(nLeftNo);
				a_pTable[uTableTop].LeafDepth = 1;
				a_pTable[nLeftNo].PaNo = static_cast<n16>(uTableTop);
				a_pTable[nLeftNo].Bit = 0;
				a_pTable[nLeftNo].PaDepth = 1;
			}
			else
			{
				uTableTop--;
			}
			uRootNo = uTableTop;
			break;
		}
		a_pTable[uTableTop].Freq = a_pTable[nLeftNo].Freq + a_pTable[nRightNo].Freq;
		a_pTable[uTableTop].ChNo[0] = static_cast<n16>(nLeftNo);
		a_pTable[uTableTop].ChNo[1] = static_cast<n16>(nRightNo);
		if (a_pTable[nLeftNo].LeafDepth > a_pTable[nRightNo].LeafDepth)
		{
			a_pTable[uTableTop].LeafDepth = a_pTable[nLeftNo].LeafDepth + 1;
		}
		else
		{
			a_pTable[uTableTop].LeafDepth = a_pTable[nRightNo].LeafDepth + 1;
		}
		a_pTable[nLeftNo].PaNo = static_cast<n16>(uTableTop);
		a_pTable[nRightNo].PaNo = static_cast<n16>(uTableTop);
		a_pTable[nLeftNo].Bit = 0;
		a_pTable[nRightNo].Bit = 1;
		addParentDepthToTable(a_pTable, static_cast<u16>(nLeftNo), static_cast<u16>(nRightNo));
		uTableTop++;
	}
	addCodeToTable(a_pTable, uRootNo, 0);
	addCountHWordToTable(a_pTable, uRootNo);
	return uRootNo;
}

void CHuffman::addParentDepthToTable(SData* a_pTable, u16 a_uLeftNo, u16 a_uRightNo)
{
	a_pTable[a_uLeftNo].PaDepth++;
	a_pTable[a_uRightNo].PaDepth++;
	if (a_pTable[a_uLeftNo].LeafDepth != 0)
	{
		addParentDepthToTable(a_pTable, static_cast<u16>(a_pTable[a_uLeftNo].ChNo[0]), static_cast<u16>(a_pTable[a_uLeftNo].ChNo[1]));
	}
	if (a_pTable[a_uRightNo].LeafDepth != 0)
	{
		addParentDepthToTable(a_pTable, static_cast<u16>(a_pTable[a_uRightNo].ChNo[0]), static_cast<u16>(a_pTable[a_uRightNo].ChNo[1]));
	}
}

void CHuffman::addCodeToTable(SData* a_pTable, u16 a_uNodeNo, u32 a_uPaHuffCode)
{
	a_pTable[a_uNodeNo].HuffCode = a_uPaHuffCode << 1 | a_pTable[a_uNodeNo].Bit;
	if (a_pTable[a_uNodeNo].LeafDepth != 0)
	{
		addCodeToTable(a_pTable, static_cast<u16>(a_pTable[a_uNodeNo].ChNo[0]), a_pTable[a_uNodeNo].HuffCode);
		addCodeToTable(a_pTable, static_cast<u16>(a_pTable[a_uNodeNo].ChNo[1]), a_pTable[a_uNodeNo].HuffCode);
	}
}

u8 CHuffman::addCountHWordToTable(SData* a_pTable, u16 a_uNodeNo)
{
	u8 uLeftHWord = 0;
	u8 uRightHWord = 0;
	switch (a_pTable[a_uNodeNo].LeafDepth)
	{
	case 0:
		return 0;
	case 1:
		uLeftHWord = 0;
		uRightHWord = 0;
		break;
	default:
		uLeftHWord = addCountHWordToTable(a_pTable, static_cast<u16>(a_pTable[a_uNodeNo].ChNo[0]));
		uRightHWord = addCountHWordToTable(a_pTable, static_cast<u16>(a_pTable[a_uNodeNo].ChNo[1]));
		break;
	}
	a_pTable[a_uNodeNo].HWord = static_cast<u16>(uLeftHWord + uRightHWord + 1);
	return uLeftHWord + uRightHWord + 1;
}

void CHuffman::makeHuffTree(SCompressionInfo* a_pInfo, u16 a_uRootNo)
{
	n16 nCostMaxRightFlag = 0;
	n16 nCostOffsetNeed = 0;
	a_pInfo->TreeTop = 1;
	a_pInfo->TreeCtrl[0].LeftOffsetNeed = 0;
	a_pInfo->TreeCtrl[0].RightNodeNo = a_uRootNo;
	while (true)
	{
		u8 uOffsetNeedNum = 0;
		for (int i = 0; i < a_pInfo->TreeTop; i++)
		{
			if (a_pInfo->TreeCtrl[i].LeftOffsetNeed != 0)
			{
				uOffsetNeedNum++;
			}
			if (a_pInfo->TreeCtrl[i].RightOffsetNeed != 0)
			{
				uOffsetNeedNum++;
			}
		}
		n16 nCostHWord = -1;
		n16 nCostMaxKey = -1;
		u8 uTmpRightFlag = 0;
		for (int i = 0; i < a_pInfo->TreeTop; i++)
		{
			n16 nTmpCostOffsetNeed = static_cast<u8>(a_pInfo->TreeTop - i);
			do
			{
				if (a_pInfo->TreeCtrl[i].LeftOffsetNeed != 0)
				{
					n16 nTmpCostHWord = static_cast<n16>(a_pInfo->Table[a_pInfo->TreeCtrl[i].LeftNodeNo].HWord);
					if (nTmpCostHWord + uOffsetNeedNum > 64)
					{
						break;
					}
					if (!remainingNodeCanSetOffset(a_pInfo, static_cast<u8>(nTmpCostHWord)))
					{
						break;
					}
					if (nTmpCostHWord > nCostHWord)
					{
						nCostMaxKey = i;
						nCostMaxRightFlag = 0;
					}
					else if (nTmpCostHWord == nCostHWord && nTmpCostOffsetNeed > nCostOffsetNeed)
					{
						nCostMaxKey = i;
						nCostMaxRightFlag = 0;
					}
				}
			} while (false);
			do
			{
				if (a_pInfo->TreeCtrl[i].RightOffsetNeed != 0)
				{
					n16 nTmpCostHWord = static_cast<n16>(a_pInfo->Table[a_pInfo->TreeCtrl[i].RightNodeNo].HWord);
					if (nTmpCostHWord + uOffsetNeedNum > 64)
					{
						break;
					}
					if (!remainingNodeCanSetOffset(a_pInfo, static_cast<u8>(nTmpCostHWord)))
					{
						break;
					}
					if (nTmpCostHWord > nCostHWord)
					{
						nCostMaxKey = i;
						nCostMaxRightFlag = 1;
					}
					else if (nTmpCostHWord == nCostHWord && nTmpCostOffsetNeed > nCostOffsetNeed)
					{
						nCostMaxKey = i;
						nCostMaxRightFlag = 1;
					}
				}
			} while (false);
		}
		bool bNextTreeMaking = false;
		do
		{
			if (nCostMaxKey >= 0)
			{
				makeSubsetHuffTree(a_pInfo, static_cast<u8>(nCostMaxKey), static_cast<u8>(nCostMaxRightFlag));
				bNextTreeMaking = true;
				break;
			}
			else
			{
				for (int i = 0; i < a_pInfo->TreeTop; i++)
				{
					u16 uTmp = 0;
					uTmpRightFlag = 0;
					if (a_pInfo->TreeCtrl[i].LeftOffsetNeed != 0)
					{
						uTmp = a_pInfo->Table[a_pInfo->TreeCtrl[i].LeftNodeNo].HWord;
					}
					if (a_pInfo->TreeCtrl[i].RightOffsetNeed != 0)
					{
						if (a_pInfo->Table[a_pInfo->TreeCtrl[i].RightNodeNo].HWord > uTmp)
						{
							uTmpRightFlag = 1;
						}
					}
					if (uTmp != 0 || uTmpRightFlag != 0)
					{
						setOneNodeOffset(a_pInfo, static_cast<u8>(i), uTmpRightFlag);
						bNextTreeMaking = true;
						break;
					}
				}
			}
		} while (false);
		if (!bNextTreeMaking)
		{
			return;
		}
	}
}

bool CHuffman::remainingNodeCanSetOffset(SCompressionInfo* a_pInfo, u8 a_uCostHWord)
{
	n16 nCapacity = static_cast<n16>(64 - a_uCostHWord);
	for (int i = 0; i < a_pInfo->TreeTop; i++)
	{
		if (a_pInfo->TreeCtrl[i].LeftOffsetNeed != 0)
		{
			if (a_pInfo->TreeTop - i <= nCapacity)
			{
				nCapacity--;
			}
			else
			{
				return false;
			}
		}
		if (a_pInfo->TreeCtrl[i].RightOffsetNeed != 0)
		{
			if (a_pInfo->TreeTop - i <= nCapacity)
			{
				nCapacity--;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

void CHuffman::makeSubsetHuffTree(SCompressionInfo* a_pInfo, u16 a_uHuffTreeNo, u8 a_uRightNodeFlag)
{
	u8 i = a_pInfo->TreeTop;
	setOneNodeOffset(a_pInfo, a_uHuffTreeNo, a_uRightNodeFlag);
	if (a_uRightNodeFlag != 0)
	{
		a_pInfo->TreeCtrl[a_uHuffTreeNo].RightOffsetNeed = 0;
	}
	else
	{
		a_pInfo->TreeCtrl[a_uHuffTreeNo].LeftOffsetNeed = 0;
	}
	while (i < a_pInfo->TreeTop)
	{
		if (a_pInfo->TreeCtrl[i].LeftOffsetNeed != 0)
		{
			setOneNodeOffset(a_pInfo, i, 0);
			a_pInfo->TreeCtrl[i].LeftOffsetNeed = 0;
		}
		if (a_pInfo->TreeCtrl[i].RightOffsetNeed != 0)
		{
			setOneNodeOffset(a_pInfo, i, 1);
			a_pInfo->TreeCtrl[i].RightOffsetNeed = 0;
		}
		i++;
	}
}

void CHuffman::setOneNodeOffset(SCompressionInfo* a_pInfo, u16 a_uHuffTreeNo, u8 a_uRightNodeFlag)
{
	u16 uNodeNo = 0;
	SData* pTable = a_pInfo->Table;
	u8* pTree = a_pInfo->Tree;
	STreeCtrlData* pTreeCtrl = a_pInfo->TreeCtrl;
	u8 uTreeTop = a_pInfo->TreeTop;
	if (a_uRightNodeFlag != 0)
	{
		uNodeNo = pTreeCtrl[a_uHuffTreeNo].RightNodeNo;
		pTreeCtrl[a_uHuffTreeNo].RightOffsetNeed = 0;
	}
	else
	{
		uNodeNo = pTreeCtrl[a_uHuffTreeNo].LeftNodeNo;
		pTreeCtrl[a_uHuffTreeNo].LeftOffsetNeed = 0;
	}
	u8 uOffsetData = 0;
	if (pTable[pTable[uNodeNo].ChNo[0]].LeafDepth == 0)
	{
		uOffsetData |= 0x80;
		pTree[uTreeTop * 2] = static_cast<u8>(pTable[uNodeNo].ChNo[0]);
		pTreeCtrl[uTreeTop].LeftNodeNo = static_cast<u8>(pTable[uNodeNo].ChNo[0]);
		pTreeCtrl[uTreeTop].LeftOffsetNeed = 0;
	}
	else
	{
		pTreeCtrl[uTreeTop].LeftNodeNo = static_cast<u16>(pTable[uNodeNo].ChNo[0]);
	}
	if (pTable[pTable[uNodeNo].ChNo[1]].LeafDepth == 0)
	{
		uOffsetData |= 0x40;
		pTree[uTreeTop * 2 + 1] = static_cast<u8>(pTable[uNodeNo].ChNo[1]);
		pTreeCtrl[uTreeTop].RightNodeNo = static_cast<u8>(pTable[uNodeNo].ChNo[1]);
		pTreeCtrl[uTreeTop].RightOffsetNeed = 0;
	}
	else
	{
		pTreeCtrl[uTreeTop].RightNodeNo = static_cast<u16>(pTable[uNodeNo].ChNo[1]);
	}
	uOffsetData |= static_cast<u8>(uTreeTop - a_uHuffTreeNo - 1);
	pTree[a_uHuffTreeNo * 2 + a_uRightNodeFlag] = uOffsetData;
	a_pInfo->TreeTop++;
}
