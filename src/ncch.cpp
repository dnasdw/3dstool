#include "ncch.h"
#include "3dscrypt.h"
#include "exefs.h"
#include "extendedheader.h"
#include "romfs.h"
#include "url_manager.h"
#include <curl/curl.h>
#include <openssl/sha.h>

const u32 CNcch::s_uSignature = SDW_CONVERT_ENDIAN32('NCCH');
const int CNcch::s_nBlockSize = 0x1000;
const CBigNum CNcch::s_DevSlot0x18KeyX = "304BF1468372EE64115EBD4093D84276";
const CBigNum CNcch::s_DevSlot0x1BKeyX = "6C8B2944A0726035F941DFC018524FB6";
const CBigNum CNcch::s_DevSlot0x25KeyX = "81907A4B6F1B47323A677974CE4AD71B";
const CBigNum CNcch::s_DevSlot0x2CKeyX = "510207515507CBB18E243DCB85E23A1D";
const CBigNum CNcch::s_RetailSlot0x18KeyX = "82E9C9BEBFB8BDB875ECC0A07D474374";
const CBigNum CNcch::s_RetailSlot0x1BKeyX = "45AD04953992C7C893724A9A7BCE6182";
const CBigNum CNcch::s_RetailSlot0x25KeyX = "CEE7D8AB30C00DAE850EF5E382AC5AF3";
const CBigNum CNcch::s_RetailSlot0x2CKeyX = "B98E95CECA3E4D171F76A94DE934C053";
const CBigNum CNcch::s_SystemFixedKey = "527CE630A9CA305F3696F3CDE954194B";
const CBigNum CNcch::s_NormalFixedKey = "00000000000000000000000000000000";

CNcch::CNcch()
	: m_eFileType(C3dsTool::kFileTypeUnknown)
	, m_bVerbose(false)
	, m_nEncryptMode(kEncryptModeNone)
	, m_bRemoveExtKey(true)
	, m_bDev(false)
	, m_nDownloadBegin(-1)
	, m_nDownloadEnd(-1)
	, m_fpNcch(nullptr)
	, m_nOffset(0)
	, m_nMediaUnitSize(1 << 9)
	, m_bAlignToBlockSize(false)
	, m_nKeyIndex(0)
{
	memset(&m_NcchHeader, 0, sizeof(m_NcchHeader));
	memset(m_nOffsetAndSize, 0, sizeof(m_nOffsetAndSize));
}

CNcch::~CNcch()
{
}

void CNcch::SetFileType(C3dsTool::EFileType a_eFileType)
{
	m_eFileType = a_eFileType;
}

void CNcch::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CNcch::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CNcch::SetHeaderFileName(const UString& a_sHeaderFileName)
{
	m_sHeaderFileName = a_sHeaderFileName;
}

void CNcch::SetEncryptMode(int a_nEncryptMode)
{
	m_nEncryptMode = a_nEncryptMode;
}

void CNcch::SetRemoveExtKey(bool a_bRemoveExtKey)
{
	m_bRemoveExtKey = a_bRemoveExtKey;
}

void CNcch::SetDev(bool a_bDev)
{
	m_bDev = a_bDev;
}

void CNcch::SetDownloadBegin(n32 a_nDownloadBegin)
{
	m_nDownloadBegin = a_nDownloadBegin;
}

void CNcch::SetDownloadEnd(n32 a_nDownloadEnd)
{
	m_nDownloadEnd = a_nDownloadEnd;
}

void CNcch::SetExtendedHeaderFileName(const UString& a_sExtendedHeaderFileName)
{
	m_sExtendedHeaderFileName = a_sExtendedHeaderFileName;
}

void CNcch::SetLogoRegionFileName(const UString& a_sLogoRegionFileName)
{
	m_sLogoRegionFileName = a_sLogoRegionFileName;
}

void CNcch::SetPlainRegionFileName(const UString& a_sPlainRegionFileName)
{
	m_sPlainRegionFileName = a_sPlainRegionFileName;
}

void CNcch::SetExeFsFileName(const UString& a_sExeFsFileName)
{
	m_sExeFsFileName = a_sExeFsFileName;
}

void CNcch::SetRomFsFileName(const UString& a_sRomFsFileName)
{
	m_sRomFsFileName = a_sRomFsFileName;
}

void CNcch::SetFilePtr(FILE* a_fpNcch)
{
	m_fpNcch = a_fpNcch;
}

void CNcch::SetOffset(n64 a_nOffset)
{
	m_nOffset = a_nOffset;
}

SNcchHeader& CNcch::GetNcchHeader()
{
	return m_NcchHeader;
}

n64* CNcch::GetOffsetAndSize()
{
	return m_nOffsetAndSize;
}

bool CNcch::ExtractFile()
{
	bool bResult = true;
	m_fpNcch = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	calculateMediaUnitSize();
	calculateOffsetSize();
	calculateKey();
	if (!extractFile(m_sHeaderFileName, 0, sizeof(m_NcchHeader), true, USTR("ncch header")))
	{
		bResult = false;
	}
	m_nKeyIndex = kEncryptKeyIndexOld;
	calculateCounter(kAesCtrTypeExtendedHeader);
	if (!extractFile(m_sExtendedHeaderFileName, m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2], m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2 + 1], false, USTR("extendedheader")))
	{
		bResult = false;
	}
	if (!extractFile(m_sLogoRegionFileName, m_nOffsetAndSize[kOffsetSizeIndexLogoRegion * 2], m_nOffsetAndSize[kOffsetSizeIndexLogoRegion * 2 + 1], true, USTR("logoregion")))
	{
		bResult = false;
	}
	if (!extractFile(m_sPlainRegionFileName, m_nOffsetAndSize[kOffsetSizeIndexPlainRegion * 2], m_nOffsetAndSize[kOffsetSizeIndexPlainRegion * 2 + 1], true, USTR("plainregion")))
	{
		bResult = false;
	}
	if (!m_sExeFsFileName.empty())
	{
		if (m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] != 0)
		{
			Fseek(m_fpNcch, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], SEEK_SET);
			ExeFsSuperBlock exeFsSuperBlock;
			fread(&exeFsSuperBlock, sizeof(exeFsSuperBlock), 1, m_fpNcch);
			calculateCounter(kAesCtrTypeExeFs);
			if (m_nEncryptMode == kEncryptModeFixedKey || m_nEncryptMode == kEncryptModeAuto)
			{
				FEncryptAesCtrData(&exeFsSuperBlock, m_Key[kEncryptKeyIndexOld], m_Counter, sizeof(exeFsSuperBlock), 0);
			}
			if (CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
			{
				Fseek(m_fpNcch, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], SEEK_SET);
				u8* pExeFs = new u8[static_cast<size_t>(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1])];
				fread(pExeFs, 1, static_cast<size_t>(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1]), m_fpNcch);
				if (m_nEncryptMode == kEncryptModeFixedKey || m_nEncryptMode == kEncryptModeAuto)
				{
					n64 nXorOffset = 0;
					FEncryptAesCtrData(pExeFs + nXorOffset, m_Key[kEncryptKeyIndexOld], m_Counter, sizeof(exeFsSuperBlock), nXorOffset);
					nXorOffset += sizeof(exeFsSuperBlock);
					FEncryptAesCtrData(pExeFs + nXorOffset, m_Key[kEncryptKeyIndexNew], m_Counter, exeFsSuperBlock.m_Header[0].size, nXorOffset);
					nXorOffset += exeFsSuperBlock.m_Header[0].size;
					FEncryptAesCtrData(pExeFs + nXorOffset, m_Key[kEncryptKeyIndexOld], m_Counter, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] - nXorOffset, nXorOffset);
				}
				FILE* fp = UFopen(m_sExeFsFileName.c_str(), USTR("wb"));
				if (fp == nullptr)
				{
					bResult = false;
				}
				else
				{
					if (m_bVerbose)
					{
						UPrintf(USTR("save: %") PRIUS USTR("\n"), m_sExeFsFileName.c_str());
					}
					fwrite(pExeFs, 1, static_cast<size_t>(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1]), fp);
					fclose(fp);
				}
				delete[] pExeFs;
			}
			else
			{
				bResult = false;
				extractFile(m_sExeFsFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1], true, USTR("exefs"));
			}
		}
		else if (m_bVerbose)
		{
			UPrintf(USTR("INFO: %") PRIUS USTR(" is not exists, %") PRIUS USTR(" will not be create\n"), USTR("exefs"), m_sExeFsFileName.c_str());
		}
	}
	else if (m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] != 0 && m_bVerbose)
	{
		UPrintf(USTR("INFO: %") PRIUS USTR(" is not extract\n"), USTR("exefs"));
	}
	m_nKeyIndex = kEncryptKeyIndexNew;
	calculateCounter(kAesCtrTypeRomFs);
	if (!extractFile(m_sRomFsFileName, m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2], m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2 + 1], false, USTR("romfs")))
	{
		bResult = false;
	}
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::CreateFile()
{
	bool bResult = true;
	m_fpNcch = UFopen(m_sFileName.c_str(), USTR("wb"));
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	if (!createHeader())
	{
		fclose(m_fpNcch);
		return false;
	}
	calculateMediaUnitSize();
	calculateAlignment();
	calculateKey();
	if (!createExtendedHeader())
	{
		bResult = false;
	}
	alignFileSize(m_nMediaUnitSize);
	if (!createLogoRegion())
	{
		bResult = false;
	}
	alignFileSize(m_nMediaUnitSize);
	if (!createPlainRegion())
	{
		bResult = false;
	}
	alignFileSize(m_nMediaUnitSize);
	if (!createExeFs())
	{
		bResult = false;
	}
	alignFileSize(m_bAlignToBlockSize ? s_nBlockSize : m_nMediaUnitSize);
	if (!createRomFs())
	{
		bResult = false;
	}
	alignFileSize(m_bAlignToBlockSize ? s_nBlockSize : m_nMediaUnitSize);
	Fseek(m_fpNcch, 0, SEEK_SET);
	fwrite(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::Download(bool a_bReadExtKey /* = true */)
{
	if (a_bReadExtKey)
	{
		readExtKey();
	}
	CUrlManager urlManager;
	u32 uCount = m_nDownloadEnd - m_nDownloadBegin + 1;
	u32 uDownloadCount = 0;
	u32 uTotalLoadCount = 0;
	u32 uLoadCount = 0;
	while (uDownloadCount != uCount)
	{
		while (uTotalLoadCount != uCount && uLoadCount < 256)
		{
			size_t uUserData = m_nDownloadBegin + uTotalLoadCount;
			CUrl* pUrl = urlManager.HttpsGet(Format("https://kagiya-ctr.cdn.nintendo.net/title/0x000400000%05X00/ext_key?country=JP", m_nDownloadBegin + uTotalLoadCount), *this, &CNcch::onHttpsGetExtKey, reinterpret_cast<void*>(uUserData));
			if (pUrl == nullptr)
			{
				urlManager.Cleanup();
				return false;
			}
			uTotalLoadCount++;
			uLoadCount++;
		}
		while (urlManager.GetCount() != 0)
		{
			u32 uCount0 = urlManager.GetCount();
			urlManager.Perform();
			u32 uCount1 = urlManager.GetCount();
			if (uCount1 != uCount0)
			{
				uDownloadCount += uCount0 - uCount1;
				uLoadCount -= uCount0 - uCount1;
				if (m_bVerbose)
				{
					UPrintf(USTR("download: %u/%u/%u\n"), uDownloadCount, uTotalLoadCount, uCount);
				}
				if (uTotalLoadCount != uCount)
				{
					break;
				}
			}
		}
	}
	return writeExtKey();
}

void CNcch::Analyze()
{
	if (m_fpNcch != nullptr)
	{
		n64 nFilePos = Ftell(m_fpNcch);
		Fseek(m_fpNcch, m_nOffset, SEEK_SET);
		fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
		calculateMediaUnitSize();
		calculateOffsetSize();
		if (m_eFileType == C3dsTool::kFileTypeCfa)
		{
			for (int i = kOffsetSizeIndexExtendedHeader; i < kOffsetSizeIndexRomFs; i++)
			{
				m_nOffsetAndSize[i * 2] = 0;
				m_nOffsetAndSize[i * 2 + 1] = 0;
			}
		}
		for (int i = kOffsetSizeIndexRomFs - 1; i >= kOffsetSizeIndexExtendedHeader; i--)
		{
			if (m_nOffsetAndSize[i * 2] == 0 && m_nOffsetAndSize[i * 2 + 1] == 0)
			{
				m_nOffsetAndSize[i * 2] = m_nOffsetAndSize[(i + 1) * 2];
			}
		}
		for (int i = kOffsetSizeIndexExtendedHeader + 1; i < kOffsetSizeIndexCount; i++)
		{
			if (m_nOffsetAndSize[i * 2] == 0 && m_nOffsetAndSize[i * 2 + 1] == 0)
			{
				m_nOffsetAndSize[i * 2] = m_nOffsetAndSize[(i - 1) * 2] + m_nOffsetAndSize[(i - 1) * 2 + 1];
			}
		}
		Fseek(m_fpNcch, nFilePos, SEEK_SET);
	}
}

bool CNcch::IsCxiFile(const UString& a_sFileName)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	SNcchHeader ncchHeader;
	fread(&ncchHeader, sizeof(ncchHeader), 1, fp);
	fclose(fp);
	bool bIsCxiFile = ncchHeader.Ncch.Signature == s_uSignature;
	if (bIsCxiFile)
	{
		switch (ncchHeader.Ncch.Flags[ContentType] & 3)
		{
		case ExecutableContentWithoutRomFs:
		case ExecutableContent:
			break;
		default:
			bIsCxiFile = false;
			break;
		}
	}
	return bIsCxiFile;
}

bool CNcch::IsCfaFile(const UString& a_sFileName)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	SNcchHeader ncchHeader;
	fread(&ncchHeader, sizeof(ncchHeader), 1, fp);
	fclose(fp);
	bool bIsCfaFile = ncchHeader.Ncch.Signature == s_uSignature;
	if (bIsCfaFile)
	{
		switch (ncchHeader.Ncch.Flags[ContentType] & 3)
		{
		case SimpleContent:
			break;
		default:
			bIsCfaFile = false;
			break;
		}
	}
	return bIsCfaFile;
}

void CNcch::calculateMediaUnitSize()
{
	m_nMediaUnitSize = 1LL << (m_NcchHeader.Ncch.Flags[SizeType] + 9);
}

void CNcch::calculateOffsetSize()
{
	m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2] = sizeof(m_NcchHeader);
	m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2 + 1] = m_NcchHeader.Ncch.ExtendedHeaderSize == sizeof(NcchExtendedHeader) ? sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended) : 0;
	m_nOffsetAndSize[kOffsetSizeIndexLogoRegion * 2] = m_NcchHeader.Ncch.LogoRegionOffset * m_nMediaUnitSize;
	m_nOffsetAndSize[kOffsetSizeIndexLogoRegion * 2 + 1] = m_NcchHeader.Ncch.LogoRegionSize * m_nMediaUnitSize;
	m_nOffsetAndSize[kOffsetSizeIndexPlainRegion * 2] = m_NcchHeader.Ncch.PlainRegionOffset * m_nMediaUnitSize;
	m_nOffsetAndSize[kOffsetSizeIndexPlainRegion * 2 + 1] = m_NcchHeader.Ncch.PlainRegionSize * m_nMediaUnitSize;
	m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2] = m_NcchHeader.Ncch.ExeFsOffset * m_nMediaUnitSize;
	m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] = m_NcchHeader.Ncch.ExeFsSize * m_nMediaUnitSize;
	m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2] = m_NcchHeader.Ncch.RomFsOffset * m_nMediaUnitSize;
	m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2 + 1] = m_NcchHeader.Ncch.RomFsSize * m_nMediaUnitSize;
}

void CNcch::calculateAlignment()
{
	m_bAlignToBlockSize = m_NcchHeader.Ncch.ContentSize % 8 == 0 && m_NcchHeader.Ncch.RomFsOffset % 8 == 0 && m_NcchHeader.Ncch.RomFsSize % 8 == 0;
}

void CNcch::calculateKey()
{
	if (m_nEncryptMode == kEncryptModeAuto)
	{
		if ((m_NcchHeader.Ncch.Flags[Flag] & SDW_BIT32(NoEncrypto)) != 0)
		{
			m_nEncryptMode = kEncryptModeNotEncrypt;
		}
		else if ((m_NcchHeader.Ncch.Flags[Flag] & SDW_BIT32(FixedCryptoKey)) != 0)
		{
			m_nEncryptMode = kEncryptModeFixedKey;
		}
	}
	if (m_nEncryptMode == kEncryptModeNotEncrypt)
	{
		return;
	}
	else if (m_nEncryptMode == kEncryptModeFixedKey)
	{
		u32 uProgramIdHigh = static_cast<u32>(m_NcchHeader.Ncch.ProgramId >> 32);
		if ((uProgramIdHigh >> 14) == 0x10 && (uProgramIdHigh & 0x10) != 0)
		{
			m_Key[kEncryptKeyIndexOld] = s_SystemFixedKey;
		}
		else
		{
			m_Key[kEncryptKeyIndexOld] = s_NormalFixedKey;
		}
		m_Key[kEncryptKeyIndexNew] = m_Key[kEncryptKeyIndexOld];
		return;
	}
	CBigNum keyX[kEncryptKeyIndexCount] = { m_bDev ? s_DevSlot0x2CKeyX : s_RetailSlot0x2CKeyX, m_bDev ? s_DevSlot0x2CKeyX : s_RetailSlot0x2CKeyX };
	switch (m_NcchHeader.Ncch.Flags[Encrypt7x])
	{
	case 0x01:
		keyX[kEncryptKeyIndexNew] = m_bDev ? s_DevSlot0x25KeyX : s_RetailSlot0x25KeyX;
		break;
	case 0x0A:
		keyX[kEncryptKeyIndexNew] = m_bDev ? s_DevSlot0x18KeyX : s_RetailSlot0x18KeyX;
		break;
	case 0x0B:
		keyX[kEncryptKeyIndexNew] = m_bDev ? s_DevSlot0x1BKeyX : s_RetailSlot0x1BKeyX;
		break;
	default:
		break;
	}
	string sKeyY;
	for (int i = 0; i < 16; i++)
	{
		sKeyY += Format("%02X", m_NcchHeader.RSASignature[i]);
	}
	CBigNum keyY[kEncryptKeyIndexCount] = { sKeyY.c_str() };
	while ((m_NcchHeader.Ncch.Flags[Flag] & SDW_BIT32(kFlagExtKey)) != 0)
	{
		readExtKey();
		u8* pProgramId = reinterpret_cast<u8*>(&m_NcchHeader.Ncch.ProgramId);
		string sProgramId;
		for (int i = 0; i < 8; i++)
		{
			sProgramId += Format("%02X", pProgramId[7 - i]);
		}
		string sExtKey;
		map<string, string>::const_iterator it = m_mExtKey.find(sProgramId);
		if (it != m_mExtKey.end())
		{
			sExtKey = it->second;
		}
		else
		{
			m_nDownloadBegin = SToN32(sProgramId.substr(9, 5), 16);
			m_nDownloadEnd = m_nDownloadBegin;
			if (!Download(false))
			{
				UPrintf(USTR("INFO: download failed\n"));
			}
			it = m_mExtKey.find(sProgramId);
			if (it == m_mExtKey.end())
			{
				UPrintf(USTR("ERROR: can not find ext key for %") PRIUS USTR("\n\n"), AToU(sProgramId).c_str());
				break;
			}
			sExtKey = it->second;
		}
		if (sExtKey.size() != 32 || sExtKey.find_first_not_of("0123456789ABCDEFabcdef") != string::npos)
		{
			UPrintf(USTR("ERROR: can not find ext key for %") PRIUS USTR("\n\n"), AToU(sProgramId).c_str());
			break;
		}
		string sExtKeyWithProgramId = sExtKey;
		for (int i = 0; i < 8; i++)
		{
			sExtKeyWithProgramId += Format("%02X", pProgramId[i]);
		}
		CBigNum bigNum = sExtKeyWithProgramId.c_str();
		u8 uBytes[32] = {};
		bigNum.GetBytes(uBytes, 24);
		u8 uSHA256[32] = {};
		SHA256(uBytes, 24, uSHA256);
		u8* pReserved0 = m_NcchHeader.Ncch.Reserved0;
		for (int i = 0; i < static_cast<int>(SDW_ARRAY_COUNT(m_NcchHeader.Ncch.Reserved0)); i++)
		{
			if (pReserved0[i] != uSHA256[i])
			{
				UPrintf(USTR("ERROR: ext key verification failed\n\n"));
				return;
			}
		}
		sKeyY += sExtKey;
		bigNum = sKeyY.c_str();
		bigNum.GetBytes(uBytes, 32);
		SHA256(uBytes, 32, uSHA256);
		sKeyY.clear();
		for (int i = 0; i < 16; i++)
		{
			sKeyY += Format("%02X", uSHA256[i]);
		}
		break;
	}
	keyY[kEncryptKeyIndexNew] = sKeyY.c_str();
	m_Key[kEncryptKeyIndexOld] = ((keyX[kEncryptKeyIndexOld].Crol(2, 128) ^ keyY[kEncryptKeyIndexOld]) + "1FF9E9AAC5FE0408024591DC5D52768A").Crol(87, 128);
	m_Key[kEncryptKeyIndexNew] = ((keyX[kEncryptKeyIndexNew].Crol(2, 128) ^ keyY[kEncryptKeyIndexNew]) + "1FF9E9AAC5FE0408024591DC5D52768A").Crol(87, 128);
}

void CNcch::readExtKey()
{
	UString sExtKeyPath = UGetModuleDirName() + USTR("/ext_key.txt");
	FILE* fp = UFopen(sExtKeyPath.c_str(), USTR("rb"));
	if (fp != nullptr)
	{
		Fseek(fp, 0, SEEK_END);
		u32 uSize = static_cast<u32>(Ftell(fp));
		Fseek(fp, 0, SEEK_SET);
		char* pTxt = new char[uSize + 1];
		fread(pTxt, 1, uSize, fp);
		fclose(fp);
		pTxt[uSize] = '\0';
		string sTxt(pTxt);
		delete[] pTxt;
		vector<string> vTxt = SplitOf(sTxt, "\r\n");
		for (vector<string>::const_iterator it = vTxt.begin(); it != vTxt.end(); ++it)
		{
			sTxt = Trim(*it);
			if (!sTxt.empty() && !StartWith(sTxt, "//"))
			{
				vector<string> vExtKey = Split(sTxt, " ");
				if (vExtKey.size() == 2)
				{
					if (!m_mExtKey.insert(make_pair(vExtKey[0], vExtKey[1])).second)
					{
						UPrintf(USTR("INFO: multiple ext key for %") PRIUS USTR("\n"), AToU(vExtKey[0]).c_str());
					}
				}
				else
				{
					UPrintf(USTR("INFO: unknown ext key record %") PRIUS USTR("\n"), AToU(sTxt).c_str());
				}
			}
		}
	}
}

bool CNcch::writeExtKey()
{
	UString sExtKeyPath = UGetModuleDirName() + USTR("/ext_key.txt");
	FILE* fp = UFopen(sExtKeyPath.c_str(), USTR("wb"));
	if (fp == nullptr)
	{
		return false;
	}
	for (map<string, string>::const_iterator it = m_mExtKey.begin(); it != m_mExtKey.end(); ++it)
	{
		fprintf(fp, "%s %s\r\n", it->first.c_str(), it->second.c_str());
	}
	fclose(fp);
	return true;
}

void CNcch::calculateCounter(EAesCtrType a_eAesCtrType)
{
	m_Counter = 0;
	u8* pPartitionId = reinterpret_cast<u8*>(&m_NcchHeader.Ncch.PartitionId);
	string sCounter;
	if (m_NcchHeader.Ncch.NcchVersion == 2 || m_NcchHeader.Ncch.NcchVersion == 0)
	{
		for (int i = 0; i < 8; i++)
		{
			sCounter += Format("%02X", pPartitionId[7 - i]);
		}
		m_Counter = sCounter.c_str();
		m_Counter = (m_Counter << 8 | a_eAesCtrType) << 56;
	}
	else if (m_NcchHeader.Ncch.NcchVersion == 1)
	{
		for (int i = 0; i < 8; i++)
		{
			sCounter += Format("%02X", pPartitionId[i]);
		}
		m_Counter = sCounter.c_str();
		m_Counter <<= 64;
		n64 nOffset = 0;
		switch (a_eAesCtrType)
		{
		case kAesCtrTypeExtendedHeader:
			nOffset = sizeof(m_NcchHeader);
			break;
		case kAesCtrTypeExeFs:
			nOffset = m_NcchHeader.Ncch.ExeFsOffset * m_nMediaUnitSize;
			break;
		case kAesCtrTypeRomFs:
			nOffset = m_NcchHeader.Ncch.RomFsOffset * m_nMediaUnitSize;
			break;
		default:
			break;
		}
		m_Counter += static_cast<u32>(nOffset);
	}
}

bool CNcch::extractFile(const UString& a_sFileName, n64 a_nOffset, n64 a_nSize, bool a_bPlainData, const UChar* a_pType)
{
	bool bResult = true;
	if (!a_sFileName.empty())
	{
		if (a_nSize != 0)
		{
			FILE* fp = UFopen(a_sFileName.c_str(), USTR("wb"));
			if (fp == nullptr)
			{
				bResult = false;
			}
			else
			{
				if (m_bVerbose)
				{
					UPrintf(USTR("save: %") PRIUS USTR("\n"), a_sFileName.c_str());
				}
				if (a_bPlainData || m_nEncryptMode == kEncryptModeNotEncrypt)
				{
					CopyFile(fp, m_fpNcch, a_nOffset, a_nSize);
				}
				else
				{
					FEncryptAesCtrCopyFile(fp, m_fpNcch, m_Key[m_nKeyIndex], m_Counter, a_nOffset, a_nSize);
				}
				fclose(fp);
			}
		}
		else if (m_bVerbose)
		{
			UPrintf(USTR("INFO: %") PRIUS USTR(" is not exists, %") PRIUS USTR(" will not be create\n"), a_pType, a_sFileName.c_str());
		}
	}
	else if (a_nSize != 0 && m_bVerbose)
	{
		UPrintf(USTR("INFO: %") PRIUS USTR(" is not extract\n"), a_pType);
	}
	return bResult;
}

bool CNcch::createHeader()
{
	FILE* fp = UFopen(m_sHeaderFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sHeaderFileName.c_str());
	}
	Fseek(fp, 0, SEEK_END);
	n64 nFileSize = Ftell(fp);
	if (nFileSize < sizeof(m_NcchHeader))
	{
		fclose(fp);
		UPrintf(USTR("ERROR: ncch header is too short\n\n"));
		return false;
	}
	Fseek(fp, 0, SEEK_SET);
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, fp);
	fclose(fp);
	if (m_nEncryptMode == kEncryptModeNotEncrypt)
	{
		m_NcchHeader.Ncch.Flags[Flag] |= SDW_BIT32(NoEncrypto);
	}
	else if (m_nEncryptMode == kEncryptModeFixedKey)
	{
		m_NcchHeader.Ncch.Flags[Flag] &= ~SDW_BIT32(NoEncrypto);
		m_NcchHeader.Ncch.Flags[Flag] |= SDW_BIT32(FixedCryptoKey);
	}
	else
	{
		m_NcchHeader.Ncch.Flags[Flag] &= ~SDW_BIT32(NoEncrypto);
		m_NcchHeader.Ncch.Flags[Flag] &= ~SDW_BIT32(FixedCryptoKey);
		if (m_bRemoveExtKey)
		{
			m_NcchHeader.Ncch.Flags[Flag] &= ~SDW_BIT32(kFlagExtKey);
		}
	}
	fwrite(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	return true;
}

bool CNcch::createExtendedHeader()
{
	if (!m_sExtendedHeaderFileName.empty())
	{
		FILE* fp = UFopen(m_sExtendedHeaderFileName.c_str(), USTR("rb"));
		if (fp == nullptr)
		{
			clearExtendedHeader();
			return false;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sExtendedHeaderFileName.c_str());
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		if (nFileSize < sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended))
		{
			fclose(fp);
			clearExtendedHeader();
			UPrintf(USTR("ERROR: extendedheader is too short\n\n"));
			return false;
		}
		m_NcchHeader.Ncch.ExtendedHeaderSize = sizeof(NcchExtendedHeader);
		Fseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended)];
		fread(pBuffer, 1, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended), fp);
		SHA256(pBuffer, m_NcchHeader.Ncch.ExtendedHeaderSize, m_NcchHeader.Ncch.ExtendedHeaderHash);
		delete[] pBuffer;
		if (m_nEncryptMode == kEncryptModeNotEncrypt)
		{
			CopyFile(m_fpNcch, fp, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended));
		}
		else
		{
			calculateCounter(kAesCtrTypeExtendedHeader);
			FEncryptAesCtrCopyFile(m_fpNcch, fp, m_Key[kEncryptKeyIndexOld], m_Counter, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended));
		}
		fclose(fp);
	}
	else
	{
		clearExtendedHeader();
	}
	return true;
}

bool CNcch::createLogoRegion()
{
	if (!m_sLogoRegionFileName.empty())
	{
		FILE* fp = UFopen(m_sLogoRegionFileName.c_str(), USTR("rb"));
		if (fp == nullptr)
		{
			clearLogoRegion();
			return false;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sLogoRegionFileName.c_str());
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		n64 nLogoRegionSize = Align(nFileSize, m_nMediaUnitSize);
		m_NcchHeader.Ncch.LogoRegionOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.LogoRegionSize = static_cast<u32>(nLogoRegionSize / m_nMediaUnitSize);
		Fseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<size_t>(nLogoRegionSize)];
		memset(pBuffer, 0, static_cast<size_t>(nLogoRegionSize));
		fread(pBuffer, 1, static_cast<size_t>(nFileSize), fp);
		fclose(fp);
		fwrite(pBuffer, 1, static_cast<size_t>(nLogoRegionSize), m_fpNcch);
		SHA256(pBuffer, static_cast<size_t>(nLogoRegionSize), m_NcchHeader.Ncch.LogoRegionHash);
		delete[] pBuffer;
	}
	else
	{
		clearLogoRegion();
	}
	return true;
}

bool CNcch::createPlainRegion()
{
	if (!m_sPlainRegionFileName.empty())
	{
		FILE* fp = UFopen(m_sPlainRegionFileName.c_str(), USTR("rb"));
		if (fp == nullptr)
		{
			clearPlainRegion();
			return false;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sPlainRegionFileName.c_str());
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		m_NcchHeader.Ncch.PlainRegionOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.PlainRegionSize = static_cast<u32>(Align(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		Fseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<size_t>(nFileSize)];
		fread(pBuffer, 1, static_cast<size_t>(nFileSize), fp);
		fclose(fp);
		fwrite(pBuffer, 1, static_cast<size_t>(nFileSize), m_fpNcch);
		delete[] pBuffer;
	}
	else
	{
		clearPlainRegion();
	}
	return true;
}

bool CNcch::createExeFs()
{
	if (!m_sExeFsFileName.empty())
	{
		FILE* fp = UFopen(m_sExeFsFileName.c_str(), USTR("rb"));
		if (fp == nullptr)
		{
			clearExeFs();
			return false;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sExeFsFileName.c_str());
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		n64 nSuperBlockSize = Align(sizeof(ExeFsSuperBlock), m_nMediaUnitSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearExeFs();
			UPrintf(USTR("ERROR: exefs is too short\n\n"));
			return false;
		}
		m_NcchHeader.Ncch.ExeFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.ExeFsSize = static_cast<u32>(Align(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		m_NcchHeader.Ncch.ExeFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
		Fseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<size_t>(nSuperBlockSize)];
		fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
		ExeFsSuperBlock& exeFsSuperBlock = *reinterpret_cast<ExeFsSuperBlock*>(pBuffer);
		if (CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
		{
			SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.ExeFsSuperBlockHash);
			Fseek(fp, 0, SEEK_SET);
			u8* pExeFs = new u8[static_cast<size_t>(nFileSize)];
			fread(pExeFs, 1, static_cast<size_t>(nFileSize), fp);
			if (m_nEncryptMode == kEncryptModeNotEncrypt)
			{
				CopyFile(m_fpNcch, fp, 0, nFileSize);
			}
			else
			{
				calculateCounter(kAesCtrTypeExeFs);
				n64 nXorOffset = 0;
				FEncryptAesCtrData(pExeFs + nXorOffset, m_Key[kEncryptKeyIndexOld], m_Counter, sizeof(exeFsSuperBlock), nXorOffset);
				nXorOffset += sizeof(exeFsSuperBlock);
				FEncryptAesCtrData(pExeFs + nXorOffset, m_Key[kEncryptKeyIndexNew], m_Counter, exeFsSuperBlock.m_Header[0].size, nXorOffset);
				nXorOffset += exeFsSuperBlock.m_Header[0].size;
				FEncryptAesCtrData(pExeFs + nXorOffset, m_Key[kEncryptKeyIndexOld], m_Counter, nFileSize - nXorOffset, nXorOffset);
				fwrite(pExeFs, 1, static_cast<size_t>(nFileSize), m_fpNcch);
			}
			delete[] pExeFs;
			delete[] pBuffer;
			fclose(fp);
		}
		else
		{
			delete[] pBuffer;
			fclose(fp);
			clearExeFs();
			UPrintf(USTR("INFO: exefs is encrypted\n"));
			return false;
		}
	}
	else
	{
		clearExeFs();
	}
	return true;
}

bool CNcch::createRomFs()
{
	if (!m_sRomFsFileName.empty())
	{
		bool bEncrypted = !CRomFs::IsRomFsFile(m_sRomFsFileName);
		if (bEncrypted)
		{
			clearRomFs();
			UPrintf(USTR("INFO: romfs is encrypted\n"));
			return false;
		}
		FILE* fp = UFopen(m_sRomFsFileName.c_str(), USTR("rb"));
		if (fp == nullptr)
		{
			clearRomFs();
			return false;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sRomFsFileName.c_str());
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		n64 nSuperBlockSize = Align(sizeof(SRomFsHeader), CRomFs::s_nSHA256BlockSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearRomFs();
			UPrintf(USTR("ERROR: romfs is too short\n\n"));
			return false;
		}
		Fseek(fp, 0, SEEK_SET);
		SRomFsHeader romFsHeader;
		fread(&romFsHeader, sizeof(romFsHeader), 1, fp);
		nSuperBlockSize = Align(Align(sizeof(SRomFsHeader), CRomFs::s_nSHA256BlockSize) + romFsHeader.Level0Size, m_nMediaUnitSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearRomFs();
			UPrintf(USTR("ERROR: romfs is too short\n\n"));
			return false;
		}
		m_NcchHeader.Ncch.RomFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
		Fseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<size_t>(nSuperBlockSize)];
		fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
		SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.RomFsSuperBlockHash);
		delete[] pBuffer;
		m_NcchHeader.Ncch.RomFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.RomFsSize = static_cast<u32>(Align(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		if (m_nEncryptMode == kEncryptModeNotEncrypt)
		{
			CopyFile(m_fpNcch, fp, 0, nFileSize);
		}
		else
		{
			calculateCounter(kAesCtrTypeRomFs);
			FEncryptAesCtrCopyFile(m_fpNcch, fp, m_Key[kEncryptKeyIndexNew], m_Counter, 0, nFileSize);
		}
		fclose(fp);
	}
	else
	{
		clearRomFs();
	}
	return true;
}

void CNcch::clearExtendedHeader()
{
	memset(m_NcchHeader.Ncch.ExtendedHeaderHash, 0, sizeof(m_NcchHeader.Ncch.ExtendedHeaderHash));
	m_NcchHeader.Ncch.ExtendedHeaderSize = 0;
}

void CNcch::clearLogoRegion()
{
	m_NcchHeader.Ncch.LogoRegionOffset = 0;
	m_NcchHeader.Ncch.LogoRegionSize = 0;
	memset(m_NcchHeader.Ncch.LogoRegionHash, 0, sizeof(m_NcchHeader.Ncch.LogoRegionHash));
}

void CNcch::clearPlainRegion()
{
	m_NcchHeader.Ncch.PlainRegionOffset = 0;
	m_NcchHeader.Ncch.PlainRegionSize = 0;
}

void CNcch::clearExeFs()
{
	m_NcchHeader.Ncch.ExeFsOffset = 0;
	m_NcchHeader.Ncch.ExeFsSize = 0;
	m_NcchHeader.Ncch.ExeFsHashRegionSize = 0;
	memset(m_NcchHeader.Ncch.ExeFsSuperBlockHash, 0, sizeof(m_NcchHeader.Ncch.ExeFsSuperBlockHash));
}

void CNcch::clearRomFs()
{
	m_NcchHeader.Ncch.RomFsOffset = 0;
	m_NcchHeader.Ncch.RomFsSize = 0;
	m_NcchHeader.Ncch.RomFsHashRegionSize = 0;
	memset(m_NcchHeader.Ncch.RomFsSuperBlockHash, 0, sizeof(m_NcchHeader.Ncch.RomFsSuperBlockHash));
}

void CNcch::alignFileSize(n64 a_nAlignment)
{
	Fseek(m_fpNcch, 0, SEEK_END);
	n64 nFileSize = Align(Ftell(m_fpNcch), a_nAlignment);
	Seek(m_fpNcch, nFileSize);
	m_NcchHeader.Ncch.ContentSize = static_cast<u32>(nFileSize / m_nMediaUnitSize);
}

void CNcch::onHttpsGetExtKey(CUrl* a_pUrl, void* a_pUserData)
{
	size_t uUserData = reinterpret_cast<size_t>(a_pUserData);
	u32 uUniqueId = static_cast<u32>(uUserData);
	if (a_pUrl != nullptr)
	{
		const string& sData = a_pUrl->GetData();
		if (!sData.empty())
		{
			string sExtKey;
			for (int i = 0; i < static_cast<int>(sData.size()); i++)
			{
				sExtKey += Format("%02X", static_cast<u8>(sData[i]));
			}
			string sProgramId = Format("000400000%05X00", uUniqueId);
			m_mExtKey.insert(make_pair(sProgramId, sExtKey));
			if (m_bVerbose)
			{
				UPrintf(USTR("download: %") PRIUS USTR(" %") PRIUS USTR("\n"), AToU(sProgramId).c_str(), AToU(sExtKey).c_str());
			}
		}
	}
}
