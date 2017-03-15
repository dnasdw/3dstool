#include "backwardlz77.h"

const int CBackwardLz77::s_nCompressWorkSize = (4098 + 4098 + 256 + 256) * sizeof(n16);

bool CBackwardLz77::GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= sizeof(CompFooter))
	{
		const CompFooter* pCompFooter = reinterpret_cast<const CompFooter*>(a_pCompressed + a_uCompressedSize - sizeof(CompFooter));
		a_uUncompressedSize = a_uCompressedSize + pCompFooter->originalBottom;
	}
	else
	{
		bResult = false;
	}
	return bResult;
}

bool CBackwardLz77::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= sizeof(CompFooter))
	{
		const CompFooter* pCompFooter = reinterpret_cast<const CompFooter*>(a_pCompressed + a_uCompressedSize - sizeof(CompFooter));
		u32 uTop = pCompFooter->bufferTopAndBottom & 0xFFFFFF;
		u32 uBottom = pCompFooter->bufferTopAndBottom >> 24 & 0xFF;
		if (uBottom >= sizeof(CompFooter) && uBottom <= sizeof(CompFooter) + 3 && uTop >= uBottom && uTop <= a_uCompressedSize && a_uUncompressedSize >= a_uCompressedSize + pCompFooter->originalBottom)
		{
			a_uUncompressedSize = a_uCompressedSize + pCompFooter->originalBottom;
			memcpy(a_pUncompressed, a_pCompressed, a_uCompressedSize);
			u8* pDest = a_pUncompressed + a_uUncompressedSize;
			u8* pSrc = a_pUncompressed + a_uCompressedSize - uBottom;
			u8* pEnd = a_pUncompressed + a_uCompressedSize - uTop;
			while (pSrc - pEnd > 0)
			{
				u8 uFlag = *--pSrc;
				for (int i = 0; i < 8; i++)
				{
					if ((uFlag << i & 0x80) == 0)
					{
						if (pDest - pEnd < 1 || pSrc - pEnd < 1)
						{
							bResult = false;
							break;
						}
						*--pDest = *--pSrc;
					}
					else
					{
						if (pSrc - pEnd < 2)
						{
							bResult = false;
							break;
						}
						int nSize = *--pSrc;
						int nOffset = (((nSize & 0x0F) << 8) | *--pSrc) + 3;
						nSize = (nSize >> 4 & 0x0F) + 3;
						if (nSize > pDest - pEnd)
						{
							bResult = false;
							break;
						}
						u8* pData = pDest + nOffset;
						if (pData > a_pUncompressed + a_uUncompressedSize)
						{
							bResult = false;
							break;
						}
						for (int j = 0; j < nSize; j++)
						{
							*--pDest = *--pData;
						}
					}
					if (pSrc - pEnd <= 0)
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

bool CBackwardLz77::Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize)
{
	bool bResult = true;
	if (a_uUncompressedSize > sizeof(CompFooter) && a_uCompressedSize >= a_uUncompressedSize)
	{
		u8* pWork = new u8[s_nCompressWorkSize];
		do
		{
			SCompressInfo info;
			initTable(&info, pWork);
			const int nMaxSize = 0xF + 3;
			const u8* pSrc = a_pUncompressed + a_uUncompressedSize;
			u8* pDest = a_pCompressed + a_uUncompressedSize;
			while (pSrc - a_pUncompressed > 0 && pDest - a_pCompressed > 0)
			{
				u8* pFlag = --pDest;
				*pFlag = 0;
				for (int i = 0; i < 8; i++)
				{
					int nOffset = 0;
					int nSize = search(&info, pSrc, nOffset, static_cast<int>(min<n64>(min<n64>(nMaxSize, pSrc - a_pUncompressed), a_pUncompressed + a_uUncompressedSize - pSrc)));
					if (nSize < 3)
					{
						if (pDest - a_pCompressed < 1)
						{
							bResult = false;
							break;
						}
						slide(&info, pSrc, 1);
						*--pDest = *--pSrc;
					}
					else
					{
						if (pDest - a_pCompressed < 2)
						{
							bResult = false;
							break;
						}
						*pFlag |= 0x80 >> i;
						slide(&info, pSrc, nSize);
						pSrc -= nSize;
						nSize -= 3;
						*--pDest = (nSize << 4 & 0xF0) | ((nOffset - 3) >> 8 & 0x0F);
						*--pDest = (nOffset - 3) & 0xFF;
					}
					if (pSrc - a_pUncompressed <= 0)
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
			a_uCompressedSize = static_cast<u32>(a_pCompressed + a_uUncompressedSize - pDest);
		} while (false);
		delete[] pWork;
	}
	else
	{
		bResult = false;
	}
	if (bResult)
	{
		u32 uOrigSize = a_uUncompressedSize;
		u8* pCompressBuffer = a_pCompressed + a_uUncompressedSize - a_uCompressedSize;
		u32 uCompressBufferSize = a_uCompressedSize;
		u32 uOrigSafe = 0;
		u32 uCompressSafe = 0;
		bool bOver = false;
		while (uOrigSize > 0)
		{
			u8 uFlag = pCompressBuffer[--uCompressBufferSize];
			for (int i = 0; i < 8; i++)
			{
				if ((uFlag << i & 0x80) == 0)
				{
					uCompressBufferSize--;
					uOrigSize--;
				}
				else
				{
					int nSize = (pCompressBuffer[--uCompressBufferSize] >> 4 & 0x0F) + 3;
					uCompressBufferSize--;
					uOrigSize -= nSize;
					if (uOrigSize < uCompressBufferSize)
					{
						uOrigSafe = uOrigSize;
						uCompressSafe = uCompressBufferSize;
						bOver = true;
						break;
					}
				}
				if (uOrigSize <= 0)
				{
					break;
				}
			}
			if (bOver)
			{
				break;
			}
		}
		u32 uCompressedSize = a_uCompressedSize - uCompressSafe;
		u32 uPadOffset = uOrigSafe + uCompressedSize;
		u32 uCompFooterOffset = static_cast<u32>(Align(uPadOffset, 4));
		a_uCompressedSize = uCompFooterOffset + sizeof(CompFooter);
		u32 uTop = a_uCompressedSize - uOrigSafe;
		u32 uBottom = a_uCompressedSize - uPadOffset;
		if (a_uCompressedSize >= a_uUncompressedSize || uTop > 0xFFFFFF)
		{
			bResult = false;
		}
		else
		{
			memcpy(a_pCompressed, a_pUncompressed, uOrigSafe);
			memmove(a_pCompressed + uOrigSafe, pCompressBuffer + uCompressSafe, uCompressedSize);
			memset(a_pCompressed + uPadOffset, 0xFF, uCompFooterOffset - uPadOffset);
			CompFooter* pCompFooter = reinterpret_cast<CompFooter*>(a_pCompressed + uCompFooterOffset);
			pCompFooter->bufferTopAndBottom = uTop | (uBottom << 24);
			pCompFooter->originalBottom = a_uUncompressedSize - a_uCompressedSize;
		}
	}
	return bResult;
}

CBackwardLz77::CBackwardLz77()
{
}

void CBackwardLz77::initTable(SCompressInfo* a_pInfo, void* a_pWork)
{
	a_pInfo->WindowPos = 0;
	a_pInfo->WindowLen = 0;
	a_pInfo->OffsetTable = static_cast<n16*>(a_pWork);
	a_pInfo->ReversedOffsetTable = static_cast<n16*>(a_pWork) + 4098;
	a_pInfo->ByteTable = static_cast<n16*>(a_pWork) + 4098 + 4098;
	a_pInfo->EndTable = static_cast<n16*>(a_pWork) + 4098 + 4098 + 256;
	for (int i = 0; i < 256; i++)
	{
		a_pInfo->ByteTable[i] = -1;
		a_pInfo->EndTable[i] = -1;
	}
}

int CBackwardLz77::search(SCompressInfo* a_pInfo, const u8* a_pSrc, int& a_nOffset, int a_nMaxSize)
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
	for (n16 nOffset = a_pInfo->EndTable[*(a_pSrc - 1)]; nOffset != -1; nOffset = pReversedOffsetTable[nOffset])
	{
		if (nOffset < uWindowPos)
		{
			pSearch = a_pSrc + uWindowPos - nOffset;
		}
		else
		{
			pSearch = a_pSrc + uWindowLen + uWindowPos - nOffset;
		}
		if (pSearch - a_pSrc < 3)
		{
			continue;
		}
		if (*(pSearch - 2) != *(a_pSrc - 2) || *(pSearch - 3) != *(a_pSrc - 3))
		{
			continue;
		}
		int nMaxSize = static_cast<int>(min<n64>(a_nMaxSize, pSearch - a_pSrc));
		int nCurrentSize = 3;
		while (nCurrentSize < nMaxSize && *(pSearch - nCurrentSize - 1) == *(a_pSrc - nCurrentSize - 1))
		{
			nCurrentSize++;
		}
		if (nCurrentSize > nSize)
		{
			nSize = nCurrentSize;
			a_nOffset = static_cast<int>(pSearch - a_pSrc);
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

inline void CBackwardLz77::slide(SCompressInfo* a_pInfo, const u8* a_pSrc, int a_nSize)
{
	for (int i = 0; i < a_nSize; i++)
	{
		slideByte(a_pInfo, a_pSrc--);
	}
}

void CBackwardLz77::slideByte(SCompressInfo* a_pInfo, const u8* a_pSrc)
{
	u8 uInData = *(a_pSrc - 1);
	u16 uInsertOffset = 0;
	const u16 uWindowPos = a_pInfo->WindowPos;
	const u16 uWindowLen = a_pInfo->WindowLen;
	n16* pOffsetTable = a_pInfo->OffsetTable;
	n16* pReversedOffsetTable = a_pInfo->ReversedOffsetTable;
	n16* pByteTable = a_pInfo->ByteTable;
	n16* pEndTable = a_pInfo->EndTable;
	if (uWindowLen == 4098)
	{
		u8 uOutData = *(a_pSrc + 4097);
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
	if (uWindowLen == 4098)
	{
		a_pInfo->WindowPos = (uWindowPos + 1) % 4098;
	}
	else
	{
		a_pInfo->WindowLen++;
	}
}
