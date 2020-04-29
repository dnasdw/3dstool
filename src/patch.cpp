#include "patch.h"
#include "ncch.h"
#include "ncsd.h"
#include <openssl/sha.h>

const u32 CPatch::s_uSignature = SDW_CONVERT_ENDIAN32('3PS\0');
const u8 CPatch::s_uCurrentVersionMajor = 1;
const u8 CPatch::s_uCurrentVersionMinor = 0;
const u8 CPatch::s_uCurrentVersionPatchLevel = 0;

CPatch::CPatch()
	: m_eFileType(C3dsTool::kFileTypeUnknown)
	, m_bVerbose(false)
	, m_fpOld(nullptr)
	, m_fpNew(nullptr)
	, m_fpPatch(nullptr)
	, m_uVersion(0)
{
	memset(&m_3dsPatchSystemHeader, 0, sizeof(m_3dsPatchSystemHeader));
}

CPatch::~CPatch()
{
}

void CPatch::SetFileType(C3dsTool::EFileType a_eFileType)
{
	m_eFileType = a_eFileType;
}

void CPatch::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CPatch::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CPatch::SetOldFileName(const UString& a_sOldFileName)
{
	m_sOldFileName = a_sOldFileName;
}

void CPatch::SetNewFileName(const UString& a_sNewFileName)
{
	m_sNewFileName = a_sNewFileName;
}

void CPatch::SetPatchFileName(const UString& a_sPatchFileName)
{
	m_sPatchFileName = a_sPatchFileName;
}

bool CPatch::CreatePatchFile()
{
	m_fpOld = UFopen(m_sOldFileName.c_str(), USTR("rb"));
	if (m_fpOld == nullptr)
	{
		return false;
	}
	Fseek(m_fpOld, 0, SEEK_END);
	n64 nFileSizeOld = Ftell(m_fpOld);
	Fseek(m_fpOld, 0, SEEK_SET);
	m_fpNew = UFopen(m_sNewFileName.c_str(), USTR("rb"));
	if (m_fpNew == nullptr)
	{
		fclose(m_fpOld);
		return false;
	}
	Fseek(m_fpNew, 0, SEEK_END);
	n64 nFileSizeNew = Ftell(m_fpNew);
	Fseek(m_fpNew, 0, SEEK_SET);
	m_fpPatch = UFopen(m_sPatchFileName.c_str(), USTR("wb"));
	if (m_fpPatch == nullptr)
	{
		fclose(m_fpNew);
		fclose(m_fpOld);
		return false;
	}
	m_3dsPatchSystemHeader.Signature = s_uSignature;
	m_3dsPatchSystemHeader.VersionMajor = s_uCurrentVersionMajor;
	m_3dsPatchSystemHeader.VersionMinor = s_uCurrentVersionMinor;
	m_3dsPatchSystemHeader.VersionPatchLevel = s_uCurrentVersionPatchLevel;
	m_3dsPatchSystemHeader.Reserved = 0;
	m_3dsPatchSystemHeader.ExtDataOffset = 0;
	fwrite(&m_3dsPatchSystemHeader, sizeof(m_3dsPatchSystemHeader), 1, m_fpPatch);
	bool bResult = true;
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: create patch from %") PRIUS USTR(" and %") PRIUS USTR("\n"), m_sOldFileName.c_str(), m_sNewFileName.c_str());
	}
	if (m_eFileType == C3dsTool::kFileTypeCci && CNcsd::IsNcsdFile(m_sOldFileName) && CNcsd::IsNcsdFile(m_sNewFileName))
	{
		if (!createNcsdPatchFile())
		{
			bResult = false;
		}
	}
	else if (m_eFileType == C3dsTool::kFileTypeCxi && CNcch::IsCxiFile(m_sOldFileName) && CNcch::IsCxiFile(m_sNewFileName))
	{
		if (!createNcchPatchFile(m_eFileType, 0, 0, true))
		{
			bResult = false;
		}
	}
	else if (m_eFileType == C3dsTool::kFileTypeCfa && CNcch::IsCfaFile(m_sOldFileName) && CNcch::IsCfaFile(m_sNewFileName))
	{
		if (!createNcchPatchFile(m_eFileType, 0, 0, true))
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
	if (nFileSizeNew < nFileSizeOld)
	{
		writeChangeSize(nFileSizeNew);
	}
	writeOver();
	m_3dsPatchSystemHeader.ExtDataOffset = Ftell(m_fpPatch);
	n64 nOffset = Ftell(m_fpPatch) + sizeof(n64);
	fwrite(&nOffset, sizeof(nOffset), 1, m_fpPatch);
	Fseek(m_fpPatch, 0, SEEK_SET);
	fwrite(&m_3dsPatchSystemHeader, sizeof(m_3dsPatchSystemHeader), 1, m_fpPatch);
	fclose(m_fpPatch);
	return bResult;
}

bool CPatch::ApplyPatchFile()
{
	m_fpOld = UFopen(m_sFileName.c_str(), USTR("rb+"));
	if (m_fpOld == nullptr)
	{
		return false;
	}
	m_fpPatch = UFopen(m_sPatchFileName.c_str(), USTR("rb"));
	if (m_fpPatch == nullptr)
	{
		fclose(m_fpOld);
		return false;
	}
	Fseek(m_fpPatch, -8, SEEK_END);
	n64 n3psOffset = 0;
	fread(&n3psOffset, 8, 1, m_fpPatch);
	Fseek(m_fpPatch, -n3psOffset, SEEK_END);
	fread(&m_3dsPatchSystemHeader, sizeof(m_3dsPatchSystemHeader), 1, m_fpPatch);
	if (m_3dsPatchSystemHeader.Signature != s_uSignature)
	{
		fclose(m_fpPatch);
		fclose(m_fpOld);
		UPrintf(USTR("ERROR: not support patch file %") PRIUS USTR("\n\n"), m_sPatchFileName.c_str());
		return false;
	}
	calculateVersion();
	if (m_uVersion > 0x010000)
	{
		fclose(m_fpPatch);
		fclose(m_fpOld);
		UPrintf(USTR("ERROR: not support patch file version %") PRIUS USTR(".%") PRIUS USTR(".%") PRIUS USTR("\n\n"), AToU(Format("%" PRIu8, m_3dsPatchSystemHeader.VersionMajor)).c_str(), AToU(Format("%" PRIu8, m_3dsPatchSystemHeader.VersionMinor)).c_str(), AToU(Format("%" PRIu8, m_3dsPatchSystemHeader.VersionPatchLevel)).c_str());
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: apply patch to %") PRIUS USTR("\n"), m_sFileName.c_str());
	}
	bool bResult = false;
	bool bPatched = false;
	u8 uPatchCommand = 0;
	fread(&uPatchCommand, 1, 1, m_fpPatch);
	if (uPatchCommand == kPatchCommandCheck)
	{
		bPatched = true;
	}
	while (uPatchCommand == kPatchCommandCheck)
	{
		n64 nOffset = 0;
		n64 nSize = 0;
		u8 uSHA256New[32] = {};
		fread(&nOffset, 8, 1, m_fpPatch);
		fread(&nSize, 8, 1, m_fpPatch);
		fread(uSHA256New, 1, 32, m_fpPatch);
		Fseek(m_fpOld, nOffset, SEEK_SET);
		u8* pData = new u8[static_cast<size_t>(nSize)];
		fread(pData, 1, static_cast<size_t>(nSize), m_fpOld);
		u8 uSHA256Old[32] = {};
		SHA256(pData, static_cast<size_t>(nSize), uSHA256Old);
		if (memcmp(uSHA256Old, uSHA256New, 32) != 0)
		{
			bPatched = false;
			break;
		}
		fread(&uPatchCommand, 1, 1, m_fpPatch);
	}
	if (bPatched)
	{
		fclose(m_fpPatch);
		fclose(m_fpOld);
		UPrintf(USTR("ERROR: %") PRIUS USTR(" was already patched\n\n"), m_sFileName.c_str());
		return true;
	}
	Fseek(m_fpPatch, -n3psOffset, SEEK_END);
	Fseek(m_fpPatch, sizeof(m_3dsPatchSystemHeader), SEEK_CUR);
	do
	{
		fread(&uPatchCommand, 1, 1, m_fpPatch);
		if (uPatchCommand == kPatchCommandOver)
		{
			if (m_bVerbose)
			{
				UPrintf(USTR("INFO: patch complete\n"));
			}
			bResult = true;
			break;
		}
		else if (uPatchCommand == kPatchCommandCheck)
		{
			Fseek(m_fpPatch, 48, SEEK_CUR);
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
		else if (uPatchCommand == kPatchCommandChangeSize)
		{
			n64 nSize = 0;
			fread(&nSize, 8, 1, m_fpPatch);
			executeChangeSize(nSize);
		}
		else if (uPatchCommand >= kPatchCommandSeekWrite && uPatchCommand <= kPatchCommandSeekWrite + 0xF)
		{
			bool bSeekSet = (uPatchCommand & 8) == 0;
			n64 nOffset = 0;
			size_t uSize = 0;
			const size_t uBufferSize = 0x10000;
			static u8 uBuffer[uBufferSize] = {};
			size_t uOffsetByte = static_cast<size_t>(SDW_BIT64(uPatchCommand >> 1 & 3));
			size_t uSizeByte = static_cast<size_t>(SDW_BIT64(uPatchCommand & 1));
			fread(&nOffset, uOffsetByte, 1, m_fpPatch);
			fread(&uSize, uSizeByte, 1, m_fpPatch);
			uSize++;
			fread(uBuffer, 1, uSize, m_fpPatch);
			executeSeekWrite(bSeekSet, nOffset, uSize, uBuffer);
		}
		else
		{
			UPrintf(USTR("ERROR: unknown patch command %") PRIUS USTR("\n\n"), AToU(Format("%02" PRIX8, uPatchCommand)).c_str());
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
	n64* pOffsetAndSizeOld = ncsdOld.GetOffsetAndSize();
	CNcsd ncsdNew;
	ncsdNew.SetFilePtr(m_fpNew);
	ncsdNew.Analyze();
	n64* pOffsetAndSizeNew = ncsdNew.GetOffsetAndSize();
	u8 uSHA256[32] = {};
	SNcsdHeader& ncsdHeaderNew = ncsdNew.GetNcsdHeader();
	SHA256(reinterpret_cast<u8*>(&ncsdHeaderNew.Ncsd), sizeof(ncsdHeaderNew.Ncsd), uSHA256);
	writeCheck(reinterpret_cast<u8*>(&ncsdHeaderNew.Ncsd) - reinterpret_cast<u8*>(&ncsdHeaderNew), sizeof(ncsdHeaderNew.Ncsd), uSHA256);
	for (int i = 0; i < 8; i++)
	{
		if (pOffsetAndSizeNew[i * 2 + 1] != 0)
		{
			Fseek(m_fpNew, pOffsetAndSizeNew[i * 2], SEEK_SET);
			CNcch ncch;
			ncch.SetFileType(i == 0 ? C3dsTool::kFileTypeCxi : C3dsTool::kFileTypeCfa);
			ncch.SetFilePtr(m_fpNew);
			ncch.SetOffset(pOffsetAndSizeNew[i * 2]);
			ncch.Analyze();
			SNcchHeader& ncchHeader = ncch.GetNcchHeader();
			SHA256(reinterpret_cast<u8*>(&ncchHeader.Ncch), sizeof(ncchHeader.Ncch), uSHA256);
			writeCheck(pOffsetAndSizeNew[i * 2] + reinterpret_cast<u8*>(&ncchHeader.Ncch) - reinterpret_cast<u8*>(&ncchHeader), sizeof(ncchHeader.Ncch), uSHA256);
		}
	}
	bitset<8> bsOver;
	while (bsOver.count() != bsOver.size())
	{
		for (int i = 7; i >= 0; i--)
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
				UPrintf(USTR("INFO: create patch from partition %d\n"), i);
				if (nOffsetNew != nOffsetOld)
				{
					if (nSizeOld != 0 && nSizeNew != 0)
					{
						writeMove(nOffsetOld, nOffsetNew, min(nSizeOld, nSizeNew));
					}
				}
				if (nSizeNew != 0)
				{
					if (!createNcchPatchFile(i == 0 ? C3dsTool::kFileTypeCxi : C3dsTool::kFileTypeCfa, nOffsetOld, nOffsetNew, false))
					{
						return false;
					}
				}
				bsOver.set(i);
				break;
			}
		}
	}
	UPrintf(USTR("INFO: create patch from ncsd header\n"));
	createPatchFile(0, sizeof(SNcsdHeader) + sizeof(CardInfoHeaderStruct), 0, sizeof(SNcsdHeader) + sizeof(CardInfoHeaderStruct));
	if (pOffsetAndSizeNew[14] + pOffsetAndSizeNew[15] < pOffsetAndSizeOld[14] + pOffsetAndSizeOld[15])
	{
		writeSet(pOffsetAndSizeNew[14] + pOffsetAndSizeNew[15], pOffsetAndSizeOld[14] + pOffsetAndSizeOld[15] - (pOffsetAndSizeNew[14] + pOffsetAndSizeNew[15]), 0xFF);
	}
	return true;
}

bool CPatch::createNcchPatchFile(C3dsTool::EFileType a_eFileType, n64 a_nOffsetOld, n64 a_nOffsetNew, bool a_bCreateCheck)
{
	static const UChar* c_pPartName[5] = { USTR("extendedheader"), USTR("logoregion"), USTR("plainregion"), USTR("exefs"), USTR("romfs") };
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
	if (a_bCreateCheck)
	{
		u8 uSHA256[32] = {};
		SNcchHeader& ncchHeader = ncchNew.GetNcchHeader();
		SHA256(reinterpret_cast<u8*>(&ncchHeader.Ncch), sizeof(ncchHeader.Ncch), uSHA256);
		writeCheck(a_nOffsetNew + reinterpret_cast<u8*>(&ncchHeader.Ncch) - reinterpret_cast<u8*>(&ncchHeader), sizeof(ncchHeader.Ncch), uSHA256);
	}
	bitset<CNcch::kOffsetSizeIndexCount> bsOver;
	while (bsOver.count() != bsOver.size())
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
				UPrintf(USTR("INFO: create patch from %") PRIUS USTR("\n"), c_pPartName[i]);
				if (nOffsetNew != nOffsetOld)
				{
					if (nSizeOld != 0 && nSizeNew != 0)
					{
						writeMove(a_nOffsetOld + nOffsetOld, a_nOffsetNew + nOffsetNew, min(nSizeOld, nSizeNew));
					}
				}
				if (nSizeNew != 0)
				{
					if (!createPatchFile(a_nOffsetOld + nOffsetOld, nSizeOld, a_nOffsetNew + nOffsetNew, nSizeNew))
					{
						return false;
					}
				}
				if (nSizeNew < nSizeOld)
				{
					writeSet(a_nOffsetNew + nOffsetNew + nSizeNew, nSizeOld - nSizeNew, 0);
				}
				bsOver.set(i);
				break;
			}
		}
	}
	UPrintf(USTR("INFO: create patch from ncch header\n"));
	createPatchFile(a_nOffsetOld, sizeof(SNcchHeader), a_nOffsetNew, sizeof(SNcchHeader));
	return true;
}

bool CPatch::createPatchFile(n64 a_nOffsetOld, n64 a_nSizeOld, n64 a_nOffsetNew, n64 a_nSizeNew)
{
	Fseek(m_fpOld, a_nOffsetOld, SEEK_SET);
	Fseek(m_fpNew, a_nOffsetNew, SEEK_SET);
	bool bSeekSet = true;
	n64 nBaseOffset = 0;
	n64 nSeekOffset = 0;
	const size_t uBufferSize = 0x10000;
	u8 uBuffer[uBufferSize] = {};
	size_t uBufferPos = 0;
	const size_t uFileBufferSize = 0x100000;
	u8* pFileBufferOld = new u8[uFileBufferSize];
	u8* pFileBufferNew = new u8[uFileBufferSize];
	n64 nCommonSize = min(a_nSizeOld, a_nSizeNew);
	for (n64 i = 0; i < nCommonSize; i++)
	{
		if (i % uFileBufferSize == 0)
		{
			fread(pFileBufferOld, 1, static_cast<size_t>(nCommonSize - i > uFileBufferSize ? uFileBufferSize : nCommonSize - i), m_fpOld);
			fread(pFileBufferNew, 1, static_cast<size_t>(nCommonSize - i > uFileBufferSize ? uFileBufferSize : nCommonSize - i), m_fpNew);
		}
		if (pFileBufferOld[i % uFileBufferSize] == pFileBufferNew[i % uFileBufferSize])
		{
			if (uBufferPos != 0)
			{
				nSeekOffset = a_nOffsetNew + i - uBufferPos - nBaseOffset;
				nBaseOffset += nSeekOffset + uBufferPos;
				writeSeekWrite(bSeekSet, nSeekOffset, uBufferPos, uBuffer);
				bSeekSet = false;
				uBufferPos = 0;
			}
		}
		else
		{
			uBuffer[uBufferPos++] = pFileBufferNew[i % uFileBufferSize];
			if (uBufferPos >= uBufferSize)
			{
				nSeekOffset = a_nOffsetNew + i + 1 - uBufferPos - nBaseOffset;
				nBaseOffset += nSeekOffset + uBufferPos;
				writeSeekWrite(bSeekSet, nSeekOffset, uBufferPos, uBuffer);
				bSeekSet = false;
				uBufferPos = 0;
			}
		}
	}
	if (uBufferPos != 0)
	{
		nSeekOffset = a_nOffsetNew + nCommonSize - uBufferPos - nBaseOffset;
		nBaseOffset += nSeekOffset + uBufferPos;
		writeSeekWrite(bSeekSet, nSeekOffset, uBufferPos, uBuffer);
		bSeekSet = false;
		uBufferPos = 0;
	}
	delete[] pFileBufferNew;
	delete[] pFileBufferOld;
	if (a_nSizeNew > a_nSizeOld)
	{
		n64 nRemainSize = a_nSizeNew - a_nSizeOld;
		while (nRemainSize > 0)
		{
			uBufferPos = static_cast<size_t>(nRemainSize > uBufferSize ? uBufferSize : nRemainSize);
			fread(uBuffer, 1, uBufferPos, m_fpNew);
			nSeekOffset = a_nOffsetNew + a_nSizeNew - nRemainSize - nBaseOffset;
			nBaseOffset += nSeekOffset + uBufferPos;
			writeSeekWrite(bSeekSet, nSeekOffset, uBufferPos, uBuffer);
			bSeekSet = false;
			nRemainSize -= uBufferPos;
			uBufferPos = 0;
		}
	}
	return true;
}

void CPatch::writeOver()
{
	writePatch(kPatchCommandOver);
}

void CPatch::writeCheck(n64 a_nOffset, n64 a_nSize, u8* a_pSHA256)
{
	n64* pSHA256 = reinterpret_cast<n64*>(a_pSHA256);
	n64 nArg[6] = { a_nOffset, a_nSize, pSHA256[0], pSHA256[1], pSHA256[2], pSHA256[3] };
	writePatch(kPatchCommandCheck, nArg);
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

void CPatch::writeChangeSize(n64 a_nSize)
{
	writePatch(kPatchCommandChangeSize, &a_nSize);
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
	writePatch(a_uPatchCommand);
	if (a_uPatchCommand == kPatchCommandMove)
	{
		fwrite(a_pArg, 8, 3, m_fpPatch);
	}
	else if (a_uPatchCommand == kPatchCommandCheck)
	{
		fwrite(a_pArg, 8, 6, m_fpPatch);
	}
	else if (a_uPatchCommand == kPatchCommandSet)
	{
		fwrite(a_pArg, 8, 2, m_fpPatch);
		fwrite(a_pArg + 2, 1, 1, m_fpPatch);
	}
	else if (a_uPatchCommand == kPatchCommandChangeSize)
	{
		fwrite(a_pArg, 8, 1, m_fpPatch);
	}
	else if (a_uPatchCommand >= kPatchCommandSeekWrite && a_uPatchCommand <= kPatchCommandSeekWrite + 0xF)
	{
		size_t uOffsetByte = static_cast<size_t>(SDW_BIT64(a_uPatchCommand >> 1 & 3));
		size_t uSizeByte = static_cast<size_t>(SDW_BIT64(a_uPatchCommand & 1));
		fwrite(a_pArg, uOffsetByte, 1, m_fpPatch);
		fwrite(a_pArg + 1, uSizeByte, 1, m_fpPatch);
	}
}

void CPatch::writePatch(u8 a_uPatchCommand)
{
	fwrite(&a_uPatchCommand, 1, 1, m_fpPatch);
}

void CPatch::calculateVersion()
{
	m_uVersion = m_3dsPatchSystemHeader.VersionMajor << 16 | m_3dsPatchSystemHeader.VersionMinor << 8 | m_3dsPatchSystemHeader.VersionPatchLevel;
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
				Fseek(m_fpOld, a_nFromOffset + nIndex * nBufferSize, SEEK_SET);
				fread(pBuffer, 1, static_cast<size_t>(nSize), m_fpOld);
				Fseek(m_fpOld, a_nToOffset + nIndex * nBufferSize, SEEK_SET);
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
				Fseek(m_fpOld, a_nFromOffset + a_nSize, SEEK_SET);
				fread(pBuffer, 1, static_cast<size_t>(nSize), m_fpOld);
				Seek(m_fpOld, a_nToOffset + a_nSize);
				fwrite(pBuffer, 1, static_cast<size_t>(nSize), m_fpOld);
			}
		}
		delete[] pBuffer;
	}
}

void CPatch::executeSet(n64 a_nStartOffset, n64 a_nSize, u8 a_uData)
{
	Fseek(m_fpOld, a_nStartOffset, SEEK_SET);
	PadFile(m_fpOld, a_nSize, a_uData);
}

void CPatch::executeChangeSize(n64 a_nSize)
{
	Chsize(Fileno(m_fpOld), a_nSize);
}

void CPatch::executeSeekWrite(bool a_bSeekSet, n64 a_nOffset, size_t a_nSize, u8* a_pData)
{
	Fseek(m_fpOld, a_nOffset, a_bSeekSet ? SEEK_SET : SEEK_CUR);
	fwrite(a_pData, 1, a_nSize, m_fpOld);
}
