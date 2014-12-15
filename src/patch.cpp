#include "patch.h"
#include "ncsd.h"

const u32 CPatch::s_uSignature = CONVERT_ENDIAN('3PS\0');
const u8 CPatch::s_uCurrentVersionMajor = 1;
const u8 CPatch::s_uCurrentVersionMinor = 0;
const u8 CPatch::s_uCurrentVersionPatchLevel = 0;

CPatch::CPatch()
	: m_eFileType(C3DSTool::kFileTypeUnknown)
	, m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_pOldFileName(nullptr)
	, m_pNewFileName(nullptr)
	, m_pPatchFileName(nullptr)
	, m_fpOld(nullptr)
	, m_fpNew(nullptr)
	, m_fpPatch(nullptr)
	, m_uVersion(0)
{
	memset(&m_3DSPatchSystemHeader, 0, sizeof(m_3DSPatchSystemHeader));
}

CPatch::~CPatch()
{
}

void CPatch::SetFileType(C3DSTool::EFileType a_eFileType)
{
	m_eFileType = a_eFileType;
}

void CPatch::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CPatch::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CPatch::SetOldFileName(const char* a_pOldFileName)
{
	m_pOldFileName = a_pOldFileName;
}

void CPatch::SetNewFileName(const char* a_pNewFileName)
{
	m_pNewFileName = a_pNewFileName;
}

void CPatch::SetPatchFileName(const char* a_pPatchFileName)
{
	m_pPatchFileName = a_pPatchFileName;
}

bool CPatch::CreatePatchFile()
{
	m_fpOld = FFopen(m_pOldFileName, "rb");
	if (m_fpOld == nullptr)
	{
		return false;
	}
	FFseek(m_fpOld, 0, SEEK_END);
	n64 nFileSizeOld = FFtell(m_fpOld);
	FFseek(m_fpOld, 0, SEEK_SET);
	m_fpNew = FFopen(m_pNewFileName, "rb");
	if (m_fpNew == nullptr)
	{
		fclose(m_fpOld);
		return false;
	}
	FFseek(m_fpNew, 0, SEEK_END);
	n64 nFileSizeNew = FFtell(m_fpNew);
	FFseek(m_fpNew, 0, SEEK_SET);
	m_fpPatch = FFopen(m_pPatchFileName, "wb");
	if (m_fpPatch == nullptr)
	{
		fclose(m_fpNew);
		fclose(m_fpOld);
		return false;
	}
	m_3DSPatchSystemHeader.Signature = s_uSignature;
	m_3DSPatchSystemHeader.VersionMajor = s_uCurrentVersionMajor;
	m_3DSPatchSystemHeader.VersionMinor = s_uCurrentVersionMinor;
	m_3DSPatchSystemHeader.VersionPatchLevel = s_uCurrentVersionPatchLevel;
	m_3DSPatchSystemHeader.Reserved = 0;
	m_3DSPatchSystemHeader.ExtDataOffset = 0;
	fwrite(&m_3DSPatchSystemHeader, sizeof(m_3DSPatchSystemHeader), 1, m_fpPatch);
	bool bResult = true;
	if (m_bVerbose)
	{
		printf("INFO: create patch from %s and %s\n", m_pOldFileName, m_pNewFileName);
	}
	if (m_eFileType == C3DSTool::kFileTypeCci && CNcsd::IsNcsdFile(m_pOldFileName) && CNcsd::IsNcsdFile(m_pNewFileName))
	{
		if (!createNcsdPatchFile())
		{
			bResult = false;
		}
	}
	else if (m_eFileType == C3DSTool::kFileTypeCxi && CNcch::IsCxiFile(m_pOldFileName) && CNcch::IsCxiFile(m_pNewFileName))
	{
		if (!createNcchPatchFile(m_eFileType, 0, 0))
		{
			bResult = false;
		}
	}
	else if (m_eFileType == C3DSTool::kFileTypeCfa && CNcch::IsCfaFile(m_pOldFileName) && CNcch::IsCfaFile(m_pNewFileName))
	{
		if (!createNcchPatchFile(m_eFileType, 0, 0))
		{
			bResult = false;
		}
	}
	else
	{
		if (!createPatchFile(0, nFileSizeOld, 0, nFileSizeNew))
		{
			bResult = false;
		}
	}
	fclose(m_fpNew);
	fclose(m_fpOld);
	writePatch(kPatchCommandOver, nullptr);
	m_3DSPatchSystemHeader.ExtDataOffset = FFtell(m_fpPatch);
	n64 nOffset = FFtell(m_fpPatch) + sizeof(n64);
	fwrite(&nOffset, sizeof(nOffset), 1, m_fpPatch);
	FFseek(m_fpPatch, 0, SEEK_SET);
	fwrite(&m_3DSPatchSystemHeader, sizeof(m_3DSPatchSystemHeader), 1, m_fpPatch);
	fclose(m_fpPatch);
	return bResult;
}

bool CPatch::ApplyPatchFile()
{
	m_fpOld = FFopen(m_pFileName, "rb+");
	if (m_fpOld == nullptr)
	{
		return false;
	}
	m_fpPatch = FFopen(m_pPatchFileName, "rb");
	if (m_fpPatch == nullptr)
	{
		fclose(m_fpOld);
		return false;
	}
	fread(&m_3DSPatchSystemHeader, sizeof(m_3DSPatchSystemHeader), 1, m_fpPatch);
	if (m_3DSPatchSystemHeader.Signature != s_uSignature)
	{
		printf("ERROR: not support patch file %s\n\n", m_pPatchFileName);
		fclose(m_fpPatch);
		fclose(m_fpOld);
		return false;
	}
	calculateVersion();
	if (m_uVersion > 0x010000)
	{
		printf("ERROR: not support patch file version %u.%u.%u\n\n", m_3DSPatchSystemHeader.VersionMajor, m_3DSPatchSystemHeader.VersionMinor, m_3DSPatchSystemHeader.VersionPatchLevel);
		fclose(m_fpPatch);
		fclose(m_fpOld);
		return false;
	}
	if (m_bVerbose)
	{
		printf("INFO: apply patch to %s\n", m_pFileName);
	}
	bool bResult = false;
	u8 uPatchCommand = 0;
	do
	{
		fread(&uPatchCommand, 1, 1, m_fpPatch);
		if (uPatchCommand == kPatchCommandOver)
		{
			if (m_bVerbose)
			{
				printf("INFO: patch complete\n");
			}
			bResult = true;
			break;
		}
		else if (uPatchCommand == kPatchCommandMove)
		{
			n64 nFromOffset = 0;
			n64 nToOffset = 0;
			n64 nSize = 0;
			fread(&nFromOffset, 8, 1, m_fpPatch);
			fread(&nToOffset, 8, 1, m_fpPatch);
			fread(&nSize, 8, 1, m_fpPatch);
			executeMove(nFromOffset, nToOffset, nSize);
		}
		else if (uPatchCommand == kPatchCommandSet)
		{
			n64 nStartOffset = 0;
			n64 nSize = 0;
			u8 uData = 0;
			fread(&nStartOffset, 8, 1, m_fpPatch);
			fread(&nSize, 8, 1, m_fpPatch);
			fread(&uData, 1, 1, m_fpPatch);
			executeSet(nStartOffset, nSize, uData);
		}
		else if (uPatchCommand >= kPatchCommandSeekWrite && uPatchCommand <= kPatchCommandSeekWrite + 0xF)
		{
			bool bSeekSet = (uPatchCommand & 8) == 0;
			n64 nOffset = 0;
			size_t nSize = 0;
			const size_t nBufferSize = 0x10000;
			static u8 uBuffer[nBufferSize] = {};
			size_t nOffsetByte = 1 << (uPatchCommand >> 1 & 3);
			size_t nSizeByte = 1 << (uPatchCommand & 1);
			fread(&nOffset, nOffsetByte, 1, m_fpPatch);
			fread(&nSize, nSizeByte, 1, m_fpPatch);
			nSize++;
			fread(uBuffer, 1, nSize, m_fpPatch);
			executeSeekWrite(bSeekSet, nOffset, nSize, uBuffer);
		}
		else
		{
			printf("ERROR: unknown patch command %02X\n\n", uPatchCommand);
			break;
		}
	} while (true);
	fclose(m_fpPatch);
	fclose(m_fpOld);
	return bResult;
}

bool CPatch::createNcsdPatchFile()
{
	CNcsd ncsdOld;
	ncsdOld.SetFilePtr(m_fpOld);
	ncsdOld.Analyze();
	SNcsdHeader& ncsdHeaderOld = ncsdOld.GetNcsdHeader();
	n64 nMediaUnitSizeOld = ncsdOld.GetMediaUnitSize();
	CNcsd ncsdNew;
	ncsdNew.SetFilePtr(m_fpNew);
	ncsdNew.Analyze();
	SNcsdHeader& ncsdHeaderNew = ncsdNew.GetNcsdHeader();
	n64 nMediaUnitSizeNew = ncsdNew.GetMediaUnitSize();
	n64 nPartitionOffsetAndSizeOld[16] = {};
	n64 nPartitionOffsetAndSizeNew[16] = {};
	for (int i = 0; i < 8; i++)
	{
		nPartitionOffsetAndSizeOld[i * 2] = ncsdHeaderOld.Ncsd.ParitionOffsetAndSize[i * 2] * nMediaUnitSizeOld;
		nPartitionOffsetAndSizeOld[i * 2 + 1] = ncsdHeaderOld.Ncsd.ParitionOffsetAndSize[i * 2 + 1] * nMediaUnitSizeOld;
		nPartitionOffsetAndSizeNew[i * 2] = ncsdHeaderNew.Ncsd.ParitionOffsetAndSize[i * 2] * nMediaUnitSizeNew;
		nPartitionOffsetAndSizeNew[i * 2 + 1] = ncsdHeaderNew.Ncsd.ParitionOffsetAndSize[i * 2 + 1] * nMediaUnitSizeNew;
	}
	bitset<8> bsOver;
	while (!bsOver.all())
	{
		for (int i = 7; i >= 0; i--)
		{
			if (!bsOver.test(i))
			{
				n64 nOffsetOld = nPartitionOffsetAndSizeOld[i * 2];
				n64 nSizeOld = nPartitionOffsetAndSizeOld[i * 2 + 1];
				n64 nOffsetNew = nPartitionOffsetAndSizeNew[i * 2];
				n64 nSizeNew = nPartitionOffsetAndSizeNew[i * 2 + 1];
				if (i != 0 && nOffsetNew < nOffsetOld && !bsOver.test(i - 1))
				{
					continue;
				}
				printf("INFO: create patch from partition %d\n", i);
				if (nOffsetNew != nOffsetOld)
				{
					if (nSizeOld != 0 && nSizeNew != 0)
					{
						writeMove(nOffsetOld, nOffsetNew, min(nSizeOld, nSizeNew));
					}
				}
				if (nSizeNew != 0)
				{
					if (!createNcchPatchFile(i == 0 ? C3DSTool::kFileTypeCxi : C3DSTool::kFileTypeCfa, nOffsetOld, nOffsetNew))
					{
						return false;
					}
				}
				bsOver.set(i);
				break;
			}
		}
	}
	printf("INFO: create patch from ncsd header\n");
	createPatchFile(0, sizeof(SNcsdHeader) + sizeof(CardInfoHeaderStruct), 0, sizeof(SNcsdHeader) + sizeof(CardInfoHeaderStruct));
	if (nPartitionOffsetAndSizeNew[14] + nPartitionOffsetAndSizeNew[15] < nPartitionOffsetAndSizeOld[14] + nPartitionOffsetAndSizeOld[15])
	{
		writeSet(nPartitionOffsetAndSizeNew[14] + nPartitionOffsetAndSizeNew[15], nPartitionOffsetAndSizeOld[14] + nPartitionOffsetAndSizeOld[15] - (nPartitionOffsetAndSizeNew[14] + nPartitionOffsetAndSizeNew[15]), 0xFF);
	}
	return true;
}

bool CPatch::createNcchPatchFile(C3DSTool::EFileType a_eFileType, n64 a_nOffsetOld, n64 a_nOffsetNew)
{
	static const char* pPartName[5] = { "extendedheader", "logoregion", "plainregion", "exefs", "romfs" };
	CNcch ncchOld;
	ncchOld.SetFileType(a_eFileType);
	ncchOld.SetFilePtr(m_fpOld);
	ncchOld.SetOffset(a_nOffsetOld);
	ncchOld.Analyze();
	n64* pOffsetAndSizeOld = ncchOld.GetOffsetAndSize();
	CNcch ncchNew;
	ncchNew.SetFileType(a_eFileType);
	ncchNew.SetFilePtr(m_fpNew);
	ncchNew.SetOffset(a_nOffsetNew);
	ncchNew.Analyze();
	n64* pOffsetAndSizeNew = ncchNew.GetOffsetAndSize();
	bitset<CNcch::kOffsetSizeIndexCount> bsOver;
	if (!bsOver.all())
	{
		for (int i = CNcch::kOffsetSizeIndexRomFs; i >= 0; i--)
		{
			if (!bsOver.test(i))
			{
				n64 nOffsetOld = pOffsetAndSizeOld[i * 2];
				n64 nSizeOld = pOffsetAndSizeOld[i * 2 + 1];
				n64 nOffsetNew = pOffsetAndSizeNew[i * 2];
				n64 nSizeNew = pOffsetAndSizeNew[i * 2 + 1];
				if (i != 0 && nOffsetNew < nOffsetOld && !bsOver.test(i - 1))
				{
					continue;
				}
				printf("INFO: create patch from %s\n", pPartName[i]);
				if (nOffsetNew != nOffsetOld)
				{
					if (nSizeOld != 0 && nSizeNew != 0)
					{
						writeMove(nOffsetOld, nOffsetNew, min(nSizeOld, nSizeNew));
					}
				}
				if (nSizeNew != 0)
				{
					if (!createPatchFile(a_nOffsetOld + nOffsetOld, nSizeOld, a_nOffsetNew + nOffsetNew, nSizeNew))
					{
						return false;
					}
				}
				bsOver.set(i);
				break;
			}
		}
	}
	printf("INFO: create patch from ncch header\n");
	createPatchFile(a_nOffsetOld, sizeof(SNcchHeader), a_nOffsetNew, sizeof(SNcchHeader));
	return true;
}

bool CPatch::createPatchFile(n64 a_nOffsetOld, n64 a_nSizeOld, n64 a_nOffsetNew, n64 a_nSizeNew)
{
	FFseek(m_fpOld, a_nOffsetOld, SEEK_SET);
	FFseek(m_fpNew, a_nOffsetNew, SEEK_SET);
	bool bSeekSet = true;
	n64 nBaseOffset = 0;
	n64 nSeekOffset = 0;
	const size_t nBufferSize = 0x10000;
	u8 uBuffer[nBufferSize] = {};
	size_t nBufferPos = 0;
	const size_t nFileBufferSize = 0x100000;
	u8* pFileBufferOld = new u8[nFileBufferSize];
	u8* pFileBufferNew = new u8[nFileBufferSize];
	n64 nCommonSize = min(a_nSizeOld, a_nSizeNew);
	for (n64 i = 0; i < nCommonSize; i++)
	{
		if (i % nFileBufferSize == 0)
		{
			fread(pFileBufferOld, 1, static_cast<size_t>(nCommonSize - i > nFileBufferSize ? nFileBufferSize : nCommonSize - i), m_fpOld);
			fread(pFileBufferNew, 1, static_cast<size_t>(nCommonSize - i > nFileBufferSize ? nFileBufferSize : nCommonSize - i), m_fpNew);
		}
		if (pFileBufferOld[i % nFileBufferSize] == pFileBufferNew[i % nFileBufferSize])
		{
			if (nBufferPos != 0)
			{
				nSeekOffset = a_nOffsetNew + i - nBufferPos - nBaseOffset;
				nBaseOffset += nSeekOffset + nBufferPos;
				writeSeekWrite(bSeekSet, nSeekOffset, nBufferPos, uBuffer);
				bSeekSet = false;
				nBufferPos = 0;
			}
		}
		else
		{
			uBuffer[nBufferPos++] = pFileBufferNew[i % nFileBufferSize];
			if (nBufferPos >= nBufferSize)
			{
				nSeekOffset = a_nOffsetNew + i - nBufferPos - nBaseOffset;
				nBaseOffset += nSeekOffset + nBufferPos;
				writeSeekWrite(bSeekSet, nSeekOffset, nBufferPos, uBuffer);
				bSeekSet = false;
				nBufferPos = 0;
			}
		}
	}
	if (nBufferPos != 0)
	{
		nSeekOffset = a_nOffsetNew + nCommonSize - nBufferPos - nBaseOffset;
		nBaseOffset += nSeekOffset + nBufferPos;
		writeSeekWrite(bSeekSet, nSeekOffset, nBufferPos, uBuffer);
		bSeekSet = false;
		nBufferPos = 0;
	}
	delete[] pFileBufferNew;
	delete[] pFileBufferOld;
	if (a_nSizeNew > a_nSizeOld)
	{
		n64 nRemainSize = a_nSizeNew - a_nSizeOld;
		while (nRemainSize > 0)
		{
			nBufferPos = static_cast<size_t>(nRemainSize > nBufferSize ? nBufferSize : nRemainSize);
			fread(uBuffer, 1, nBufferPos, m_fpNew);
			nSeekOffset = a_nOffsetNew + a_nSizeNew - nRemainSize - nBaseOffset;
			nBaseOffset += nSeekOffset + nBaseOffset;
			writeSeekWrite(bSeekSet, nSeekOffset, nBufferPos, uBuffer);
			bSeekSet = false;
			nRemainSize -= nBufferPos;
			nBufferPos = 0;
		}
	}
	return true;
}

void CPatch::writeMove(n64 a_nFromOffset, n64 a_nToOffset, n64 a_nSize)
{
	n64 nArg[3] = { a_nFromOffset, a_nToOffset, a_nSize };
	writePatch(kPatchCommandMove, nArg);
}

void CPatch::writeSet(n64 a_nStartOffset, n64 a_nSize, u8 a_uData)
{
	n64 nArg[3] = { a_nStartOffset, a_nSize, a_uData };
	writePatch(kPatchCommandSet, nArg);
}

void CPatch::writeSeekWrite(bool a_bSeekSet, n64 a_nOffset, size_t a_nSize, u8* a_pData)
{
	u8 uPatchCommand = kPatchCommandSeekWrite;
	if (!a_bSeekSet)
	{
		uPatchCommand |= 1 << 3;
	}
	n64 nArg[2] = { a_nOffset, static_cast<n64>(a_nSize) - 1 };
	if (nArg[0] >= 0x100LL)
	{
		if (nArg[0] < 0x10000LL)
		{
			uPatchCommand |= 1 << 1;
		}
		else if (nArg[0] < 0x100000000LL)
		{
			uPatchCommand |= 2 << 1;
		}
		else
		{
			uPatchCommand |= 3 << 1;
		}
	}
	if (nArg[1] >= 0x100)
	{
		uPatchCommand |= 1;
	}
	writePatch(uPatchCommand, nArg);
	fwrite(a_pData, a_nSize, 1, m_fpPatch);
}

void CPatch::writePatch(u8 a_uPatchCommand, n64* a_pArg)
{
	fwrite(&a_uPatchCommand, 1, 1, m_fpPatch);
	if (a_uPatchCommand == kPatchCommandMove)
	{
		fwrite(a_pArg, 8, 3, m_fpPatch);
	}
	else if (a_uPatchCommand == kPatchCommandSet)
	{
		fwrite(a_pArg, 8, 2, m_fpPatch);
		fwrite(a_pArg + 2, 1, 1, m_fpPatch);
	}
	else if (a_uPatchCommand >= kPatchCommandSeekWrite && a_uPatchCommand <= kPatchCommandSeekWrite + 0xF)
	{
		size_t nOffsetByte = 1 << (a_uPatchCommand >> 1 & 3);
		size_t nSizeByte = 1 << (a_uPatchCommand & 1);
		fwrite(a_pArg, nOffsetByte, 1, m_fpPatch);
		fwrite(a_pArg + 1, nSizeByte, 1, m_fpPatch);
	}
}

void CPatch::calculateVersion()
{
	m_uVersion = m_3DSPatchSystemHeader.VersionMajor << 16 | m_3DSPatchSystemHeader.VersionMinor << 8 | m_3DSPatchSystemHeader.VersionPatchLevel;
}

void CPatch::executeMove(n64 a_nFromOffset, n64 a_nToOffset, n64 a_nSize)
{
	if (a_nFromOffset != a_nToOffset)
	{
		const n64 nBufferSize = 0x100000;
		u8* pBuffer = new u8[nBufferSize];
		int nIndex = 0;
		if (a_nFromOffset > a_nToOffset)
		{
			while (a_nSize > 0)
			{
				n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
				FFseek(m_fpOld, a_nFromOffset + nIndex * nBufferSize, SEEK_SET);
				fread(pBuffer, 1, static_cast<size_t>(nSize), m_fpOld);
				FFseek(m_fpOld, a_nToOffset + nIndex * nBufferSize, SEEK_SET);
				fwrite(pBuffer, 1, static_cast<size_t>(nSize), m_fpOld);
				a_nSize -= nSize;
				nIndex++;
			}
		}
		else
		{
			while (a_nSize > 0)
			{
				n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
				a_nSize -= nSize;
				FFseek(m_fpOld, a_nFromOffset + a_nSize, SEEK_SET);
				fread(pBuffer, 1, static_cast<size_t>(nSize), m_fpOld);
				FFseek(m_fpOld, a_nToOffset + a_nSize, SEEK_SET);
				fwrite(pBuffer, 1, static_cast<size_t>(nSize), m_fpOld);
			}
		}
		delete[] pBuffer;
	}
}

void CPatch::executeSet(n64 a_nStartOffset, n64 a_nSize, u8 a_uData)
{
	FFseek(m_fpOld, a_nStartOffset, SEEK_SET);
	FPadFile(m_fpOld, a_nSize, a_uData);
}

void CPatch::executeSeekWrite(bool a_bSeekSet, n64 a_nOffset, size_t a_nSize, u8* a_pData)
{
	FFseek(m_fpOld, a_nOffset, a_bSeekSet ? SEEK_SET : SEEK_CUR);
	fwrite(a_pData, 1, a_nSize, m_fpOld);
}
