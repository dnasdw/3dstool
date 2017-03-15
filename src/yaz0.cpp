#include "yaz0.h"

const int CYaz0::s_nCompressWorkSize = (4096 + 4096 + 256 + 256) * sizeof(n16);

bool CYaz0::GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 16 && *reinterpret_cast<const u32*>(a_pCompressed) == SDW_CONVERT_ENDIAN32('Yaz0'))
	{
		a_uUncompressedSize = SDW_CONVERT_ENDIAN32(*reinterpret_cast<const u32*>(a_pCompressed + 4));
	}
	else
	{
		bResult = false;
	}
	return bResult;
}

u32 CYaz0::GetCompressBoundSize(u32 a_uUncompressedSize, n32 a_nCompressAlign)
{
	return (16 + (a_uUncompressedSize + 7) / 8 * 9 + a_nCompressAlign - 1) / a_nCompressAlign * a_nCompressAlign;
}

bool CYaz0::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 16 && *reinterpret_cast<const u32*>(a_pCompressed) == SDW_CONVERT_ENDIAN32('Yaz0'))
	{
		u32 uUncompressedSize = SDW_CONVERT_ENDIAN32(*reinterpret_cast<const u32*>(a_pCompressed + 4));
		if (a_uUncompressedSize >= uUncompressedSize)
		{
			a_uUncompressedSize = uUncompressedSize;
			const u8* pSrc = a_pCompressed + 16;
			u8* pDest = a_pUncompressed;
			while (a_pUncompressed + a_uUncompressedSize - pDest > 0)
			{
				if (a_pCompressed + a_uCompressedSize - pSrc < 1)
				{
					bResult = false;
					break;
				}
				u8 uFlag = *pSrc++;
				for (int i = 0; i < 8; i++)
				{
					if ((uFlag << i & 0x80) != 0)
					{
						if (a_pCompressed + a_uCompressedSize - pSrc < 1)
						{
							bResult = false;
							break;
						}
						*pDest++ = *pSrc++;
					}
					else
					{
						if (a_pCompressed + a_uCompressedSize - pSrc < 2)
						{
							bResult = false;
							break;
						}
						int nSize = *pSrc >> 4 & 0x0F;
						int nOffset = (*pSrc++ & 0x0F) << 8;
						nOffset = (nOffset | *pSrc++) + 1;
						if (nSize == 0)
						{
							if (a_pCompressed + a_uCompressedSize - pSrc < 1)
							{
								bResult = false;
								break;
							}
							nSize = *pSrc++ + 0x12;
						}
						else
						{
							nSize += 2;
						}
						if (nSize > a_pUncompressed + a_uUncompressedSize - pDest)
						{
							bResult = false;
							break;
						}
						u8* pData = pDest - nOffset;
						if (pData < a_pUncompressed)
						{
							bResult = false;
							break;
						}
						for (int j = 0; j < nSize; j++)
						{
							*pDest++ = *pData++;
						}
					}
					if (a_pUncompressed + a_uUncompressedSize - pDest <= 0)
					{
						break;
					}
				}
				if (!bResult)
				{
					break;
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
	return bResult;
}

bool CYaz0::Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, n32 a_nYaz0Align)
{
	bool bResult = true;
	u8* pWork = new u8[s_nCompressWorkSize];
	do
	{
		if (a_uCompressedSize < 16)
		{
			bResult = false;
			break;
		}
		*reinterpret_cast<u32*>(a_pCompressed) = SDW_CONVERT_ENDIAN32('Yaz0');
		*reinterpret_cast<u32*>(a_pCompressed + 4) = SDW_CONVERT_ENDIAN32(a_uUncompressedSize);
		*reinterpret_cast<u32*>(a_pCompressed + 8) = SDW_CONVERT_ENDIAN32(a_nYaz0Align);
		*reinterpret_cast<u32*>(a_pCompressed + 12) = 0;
		SCompressInfo info;
		initTable(&info, pWork);
		const int nMaxSize = 0xFF + 0xF + 3;
		const u8* pSrc = a_pUncompressed;
		u8* pDest = a_pCompressed + 16;
		int nLastOffset = 0;
		int nLastSize = 0;
		bool bLastSearch = false;
		while (a_pUncompressed + a_uUncompressedSize - pSrc > 0)
		{
			if (a_pCompressed + a_uCompressedSize - pDest < 1)
			{
				bResult = false;
				break;
			}
			u8* pFlag = pDest++;
			*pFlag = 0;
			for (int i = 0; i < 8; i++)
			{
				int nOffset = 0;
				int nSize = 0;
				if (bLastSearch)
				{
					nOffset = nLastOffset;
					nSize = nLastSize;
					bLastSearch = false;
				}
				else
				{
					nSize = search(&info, pSrc, nOffset, static_cast<int>(min<n64>(nMaxSize, a_pUncompressed + a_uUncompressedSize - pSrc)));
				}
				if (nSize < 3)
				{
					if (a_pCompressed + a_uCompressedSize - pDest < 1)
					{
						bResult = false;
						break;
					}
					*pFlag |= 0x80 >> i;
					slide(&info, pSrc, 1);
					*pDest++ = *pSrc++;
				}
				else
				{
					slide(&info, pSrc, 1);
					nLastOffset = 0;
					nLastSize = search(&info, pSrc + 1, nLastOffset, static_cast<int>(min<n64>(nMaxSize, a_pUncompressed + a_uUncompressedSize - pSrc - 1)));
					if (nLastSize > nSize)
					{
						if (a_pCompressed + a_uCompressedSize - pDest < 1)
						{
							bResult = false;
							break;
						}
						*pFlag |= 0x80 >> i;
						*pDest++ = *pSrc++;
						bLastSearch = true;
					}
					else
					{
						slide(&info, pSrc + 1, nSize - 1);
						pSrc += nSize;
						if (nSize >= 0xF + 3)
						{
							if (a_pCompressed + a_uCompressedSize - pDest < 3)
							{
								bResult = false;
								break;
							}
							nSize -= 0xF + 3;
							*pDest++ = (nOffset - 1) >> 8 & 0x0F;
							*pDest++ = (nOffset - 1) & 0xFF;
							*pDest++ = nSize;
						}
						else
						{
							if (a_pCompressed + a_uCompressedSize - pDest < 2)
							{
								bResult = false;
								break;
							}
							nSize -= 2;
							*pDest++ = (nSize << 4 & 0xF0) | ((nOffset - 1) >> 8 & 0x0F);
							*pDest++ = (nOffset - 1) & 0xFF;
						}
					}
				}
				if (a_pUncompressed + a_uUncompressedSize - pSrc <= 0)
				{
					break;
				}
			}
			if (!bResult)
			{
				break;
			}
		}
		if (!bResult)
		{
			break;
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

CYaz0::CYaz0()
{
}

void CYaz0::initTable(SCompressInfo* a_pInfo, void* a_pWork)
{
	a_pInfo->WindowPos = 0;
	a_pInfo->WindowLen = 0;
	a_pInfo->OffsetTable = static_cast<n16*>(a_pWork);
	a_pInfo->ReversedOffsetTable = static_cast<n16*>(a_pWork) + 4096;
	a_pInfo->ByteTable = static_cast<n16*>(a_pWork) + 4096 + 4096;
	a_pInfo->EndTable = static_cast<n16*>(a_pWork) + 4096 + 4096 + 256;
	for (int i = 0; i < 256; i++)
	{
		a_pInfo->ByteTable[i] = -1;
		a_pInfo->EndTable[i] = -1;
	}
}

int CYaz0::search(SCompressInfo* a_pInfo, const u8* a_pSrc, int& a_nOffset, int a_nMaxSize)
{
	if (a_nMaxSize < 3)
	{
		return 0;
	}
	const u8* pSearch = nullptr;
	int nSize = 2;
	const u16 uWindowPos = a_pInfo->WindowPos;
	const u16 uWindowLen = a_pInfo->WindowLen;
	n16* pReversedOffsetTable = a_pInfo->ReversedOffsetTable;
	for (n16 nOffset = a_pInfo->EndTable[*a_pSrc]; nOffset != -1; nOffset = pReversedOffsetTable[nOffset])
	{
		if (nOffset < uWindowPos)
		{
			pSearch = a_pSrc - uWindowPos + nOffset;
		}
		else
		{
			pSearch = a_pSrc - uWindowLen - uWindowPos + nOffset;
		}
		if (*(pSearch + 1) != *(a_pSrc + 1) || *(pSearch + 2) != *(a_pSrc + 2))
		{
			continue;
		}
		int nCurrentSize = 3;
		while (nCurrentSize < a_nMaxSize && *(pSearch + nCurrentSize) == *(a_pSrc + nCurrentSize))
		{
			nCurrentSize++;
		}
		if (nCurrentSize > nSize)
		{
			nSize = nCurrentSize;
			a_nOffset = static_cast<int>(a_pSrc - pSearch);
			if (nSize == a_nMaxSize)
			{
				break;
			}
		}
	}
	if (nSize < 3)
	{
		return 0;
	}
	return nSize;
}

inline void CYaz0::slide(SCompressInfo* a_pInfo, const u8* a_pSrc, int a_nSize)
{
	for (int i = 0; i < a_nSize; i++)
	{
		slideByte(a_pInfo, a_pSrc++);
	}
}

void CYaz0::slideByte(SCompressInfo* a_pInfo, const u8* a_pSrc)
{
	u8 uInData = *a_pSrc;
	u16 uInsertOffset = 0;
	const u16 uWindowPos = a_pInfo->WindowPos;
	const u16 uWindowLen = a_pInfo->WindowLen;
	n16* pOffsetTable = a_pInfo->OffsetTable;
	n16* pReversedOffsetTable = a_pInfo->ReversedOffsetTable;
	n16* pByteTable = a_pInfo->ByteTable;
	n16* pEndTable = a_pInfo->EndTable;
	if (uWindowLen == 4096)
	{
		u8 uOutData = *(a_pSrc - 4096);
		if ((pByteTable[uOutData] = pOffsetTable[pByteTable[uOutData]]) == -1)
		{
			pEndTable[uOutData] = -1;
		}
		else
		{
			pReversedOffsetTable[pByteTable[uOutData]] = -1;
		}
		uInsertOffset = uWindowPos;
	}
	else
	{
		uInsertOffset = uWindowLen;
	}
	n16 nOffset = pEndTable[uInData];
	if (nOffset == -1)
	{
		pByteTable[uInData] = uInsertOffset;
	}
	else
	{
		pOffsetTable[nOffset] = uInsertOffset;
	}
	pEndTable[uInData] = uInsertOffset;
	pOffsetTable[uInsertOffset] = -1;
	pReversedOffsetTable[uInsertOffset] = nOffset;
	if (uWindowLen == 4096)
	{
		a_pInfo->WindowPos = (uWindowPos + 1) % 4096;
	}
	else
	{
		a_pInfo->WindowLen++;
	}
}
