#include "ncch.h"
#include "3dscrypt.h"
#include "exefs.h"
#include "extendedheader.h"
#include "romfs.h"
#include <curl/curl.h>
#include <openssl/sha.h>

const u32 CNcch::s_uSignature = SDW_CONVERT_ENDIAN32('NCCH');
const int CNcch::s_nBlockSize = 0x1000;
const CBigNum CNcch::s_Slot0x18KeyX = "82E9C9BEBFB8BDB875ECC0A07D474374";
const CBigNum CNcch::s_Slot0x1BKeyX = "45AD04953992C7C893724A9A7BCE6182";
const CBigNum CNcch::s_Slot0x25KeyX = "CEE7D8AB30C00DAE850EF5E382AC5AF3";

CNcch::CNcch()
	: m_eFileType(C3dsTool::kFileTypeUnknown)
	, m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_pHeaderFileName(nullptr)
	, m_nEncryptMode(kEncryptModeNone)
	, m_bNotUpdateExtendedHeaderHash(false)
	, m_bNotUpdateExeFsHash(false)
	, m_bNotUpdateRomFsHash(false)
	, m_pExtendedHeaderFileName(nullptr)
	, m_pLogoRegionFileName(nullptr)
	, m_pPlainRegionFileName(nullptr)
	, m_pExeFsFileName(nullptr)
	, m_pRomFsFileName(nullptr)
	, m_pExtendedHeaderXorFileName(nullptr)
	, m_pExeFsXorFileName(nullptr)
	, m_pExeFsTopXorFileName(nullptr)
	, m_bExeFsTopAutoKey(false)
	, m_bRomFsAutoKey(false)
	, m_fpNcch(nullptr)
	, m_nOffset(0)
	, m_nMediaUnitSize(1 << 9)
	, m_bAlignToBlockSize(false)
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

void CNcch::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CNcch::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CNcch::SetHeaderFileName(const char* a_pHeaderFileName)
{
	m_pHeaderFileName = a_pHeaderFileName;
}

void CNcch::SetEncryptMode(int a_nEncryptMode)
{
	m_nEncryptMode = a_nEncryptMode;
}

void CNcch::SetKey(const CBigNum& a_Key)
{
	m_Key = a_Key;
}

void CNcch::SetNotUpdateExtendedHeaderHash(bool a_bNotUpdateExtendedHeaderHash)
{
	m_bNotUpdateExtendedHeaderHash = a_bNotUpdateExtendedHeaderHash;
}

void CNcch::SetNotUpdateExeFsHash(bool a_bNotUpdateExeFsHash)
{
	m_bNotUpdateExeFsHash = a_bNotUpdateExeFsHash;
}

void CNcch::SetNotUpdateRomFsHash(bool a_bNotUpdateRomFsHash)
{
	m_bNotUpdateRomFsHash = a_bNotUpdateRomFsHash;
}

void CNcch::SetExtendedHeaderFileName(const char* a_pExtendedHeaderFileName)
{
	m_pExtendedHeaderFileName = a_pExtendedHeaderFileName;
}

void CNcch::SetLogoRegionFileName(const char* a_pLogoRegionFileName)
{
	m_pLogoRegionFileName = a_pLogoRegionFileName;
}

void CNcch::SetPlainRegionFileName(const char* a_pPlainRegionFileName)
{
	m_pPlainRegionFileName = a_pPlainRegionFileName;
}

void CNcch::SetExeFsFileName(const char* a_pExeFsFileName)
{
	m_pExeFsFileName = a_pExeFsFileName;
}

void CNcch::SetRomFsFileName(const char* a_pRomFsFileName)
{
	m_pRomFsFileName = a_pRomFsFileName;
}

void CNcch::SetExtendedHeaderXorFileName(const char* a_pExtendedHeaderXorFileName)
{
	m_pExtendedHeaderXorFileName = a_pExtendedHeaderXorFileName;
}

void CNcch::SetExeFsXorFileName(const char* a_pExeFsXorFileName)
{
	m_pExeFsXorFileName = a_pExeFsXorFileName;
}

void CNcch::SetExeFsTopXorFileName(const char* a_pExeFsTopXorFileName)
{
	m_pExeFsTopXorFileName = a_pExeFsTopXorFileName;
}

void CNcch::SetRomFsXorFileName(const string& a_sRomFsXorFileName)
{
	m_sRomFsXorFileName = a_sRomFsXorFileName;
}

void CNcch::SetExeFsTopAutoKey(bool a_bExeFsTopAutoKey)
{
	m_bExeFsTopAutoKey = a_bExeFsTopAutoKey;
}

void CNcch::SetRomFsAutoKey(bool a_bRomFsAutoKey)
{
	m_bRomFsAutoKey = a_bRomFsAutoKey;
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
	m_fpNcch = Fopen(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	calculateMediaUnitSize();
	calculateOffsetSize();
	calculateKey();
	if (!extractFile(m_pHeaderFileName, 0, sizeof(m_NcchHeader), true, "ncch header"))
	{
		bResult = false;
	}
	calculateCounter(kAesCtrTypeExtendedHeader);
	m_sXorFileName = m_pExtendedHeaderXorFileName;
	if (!extractFile(m_pExtendedHeaderFileName, m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2], m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2 + 1], false, "extendedheader"))
	{
		bResult = false;
	}
	if (!extractFile(m_pLogoRegionFileName, m_nOffsetAndSize[kOffsetSizeIndexLogoRegion * 2], m_nOffsetAndSize[kOffsetSizeIndexLogoRegion * 2 + 1], true, "logoregion"))
	{
		bResult = false;
	}
	if (!extractFile(m_pPlainRegionFileName, m_nOffsetAndSize[kOffsetSizeIndexPlainRegion * 2], m_nOffsetAndSize[kOffsetSizeIndexPlainRegion * 2 + 1], true, "plainregion"))
	{
		bResult = false;
	}
	calculateCounter(kAesCtrTypeExeFs);
	if (m_pExeFsXorFileName != nullptr && (m_pExeFsTopXorFileName != nullptr || m_bExeFsTopAutoKey) && m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] != 0)
	{
		Fseek(m_fpNcch, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], SEEK_SET);
		ExeFsSuperBlock exeFsSuperBlock;
		fread(&exeFsSuperBlock, sizeof(exeFsSuperBlock), 1, m_fpNcch);
		Fseek(m_fpNcch, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], SEEK_SET);
		u8* pExeFs = new u8[static_cast<size_t>(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1])];
		fread(pExeFs, 1, static_cast<size_t>(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1]), m_fpNcch);
		bool bEncryptResult = true;
		if (!CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
		{
			bEncryptResult = FEncryptXorData(&exeFsSuperBlock, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), 0);
		}
		if (bEncryptResult)
		{
			n64 nXorOffset = 0;
			if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), nXorOffset))
			{
				bEncryptResult = false;
			}
			nXorOffset += sizeof(exeFsSuperBlock);
			if (m_bExeFsTopAutoKey)
			{
				FEncryptAesCtrData(pExeFs + nXorOffset, m_Key, m_Counter, exeFsSuperBlock.m_Header[0].size, nXorOffset);
			}
			else if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsTopXorFileName, exeFsSuperBlock.m_Header[0].size, nXorOffset))
			{
				bEncryptResult = false;
			}
			nXorOffset += exeFsSuperBlock.m_Header[0].size;
			if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] - nXorOffset, nXorOffset))
			{
				bEncryptResult = false;
			}
		}
		if (bEncryptResult)
		{
			FILE* fp = Fopen(m_pExeFsFileName, "wb");
			if (fp == nullptr)
			{
				bResult = false;
			}
			else
			{
				if (m_bVerbose)
				{
					printf("save: %s\n", m_pExeFsFileName);
				}
				fwrite(pExeFs, 1, static_cast<size_t>(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1]), fp);
				fclose(fp);
			}
			delete[] pExeFs;
		}
		else
		{
			delete[] pExeFs;
			bResult = false;
			extractFile(m_pExeFsFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1], true, "exefs");
		}
	}
	else
	{
		m_sXorFileName = m_pExeFsXorFileName;
		if (!extractFile(m_pExeFsFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1], false, "exefs"))
		{
			bResult = false;
		}
	}
	calculateCounter(kAesCtrTypeRomFs);
	m_sXorFileName = m_sRomFsXorFileName;
	if (m_bRomFsAutoKey)
	{
		m_nEncryptMode = kEncryptModeAesCtr;
		if (!extractFile(m_pRomFsFileName, m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2], m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2 + 1], false, "romfs"))
		{
			bResult = false;
		}
		m_nEncryptMode = kEncryptModeXor;
	}
	else if (!extractFile(m_pRomFsFileName, m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2], m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2 + 1], false, "romfs"))
	{
		bResult = false;
	}
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::CreateFile()
{
	bool bResult = true;
	m_fpNcch = Fopen(m_pFileName, "wb");
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

bool CNcch::EncryptFile()
{
	bool bResult = true;
	m_fpNcch = Fopen(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	fclose(m_fpNcch);
	calculateMediaUnitSize();
	calculateOffsetSize();
	calculateKey();
	if (m_nEncryptMode == kEncryptModeAesCtr)
	{
		calculateCounter(kAesCtrTypeExtendedHeader);
		if (!encryptAesCtrFile(m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2], m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2 + 1], 0, "extendedheader"))
		{
			bResult = false;
		}
		calculateCounter(kAesCtrTypeExeFs);
		if (!encryptAesCtrFile(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1], 0, "exefs"))
		{
			bResult = false;
		}
		calculateCounter(kAesCtrTypeRomFs);
		if (!encryptAesCtrFile(m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2], m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2 + 1], 0, "romfs"))
		{
			bResult = false;
		}
	}
	else if (m_nEncryptMode == kEncryptModeXor)
	{
		if (!encryptXorFile(m_pExtendedHeaderXorFileName, m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2], m_nOffsetAndSize[kOffsetSizeIndexExtendedHeader * 2 + 1], 0, "extendedheader"))
		{
			bResult = false;
		}
		if (m_pExeFsTopXorFileName == nullptr && !m_bExeFsTopAutoKey)
		{
			if (!encryptXorFile(m_pExeFsXorFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1], 0, "exefs"))
			{
				bResult = false;
			}
		}
		else if (m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] != 0)
		{
			m_fpNcch = Fopen(m_pFileName, "rb");
			if (m_fpNcch == nullptr)
			{
				bResult = false;
			}
			else
			{
				Fseek(m_fpNcch, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2], SEEK_SET);
				ExeFsSuperBlock exeFsSuperBlock;
				fread(&exeFsSuperBlock, sizeof(exeFsSuperBlock), 1, m_fpNcch);
				fclose(m_fpNcch);
				bool bEncryptResult = true;
				if (!CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
				{
					bEncryptResult = FEncryptXorData(&exeFsSuperBlock, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), 0);
				}
				if (!bEncryptResult)
				{
					bResult = false;
				}
				else
				{
					n64 nXorOffset = 0;
					if (!encryptXorFile(m_pExeFsXorFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2] + nXorOffset, sizeof(exeFsSuperBlock), nXorOffset, "exefs super block"))
					{
						bResult = false;
					}
					nXorOffset += sizeof(exeFsSuperBlock);
					if (m_bExeFsTopAutoKey)
					{
						if (!encryptAesCtrFile(m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2] + nXorOffset, exeFsSuperBlock.m_Header[0].size, nXorOffset, "exefs top section"))
						{
							bResult = false;
						}
					}
					else if (!encryptXorFile(m_pExeFsTopXorFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2] + nXorOffset, exeFsSuperBlock.m_Header[0].size, nXorOffset, "exefs top section"))
					{
						bResult = false;
					}
					nXorOffset += exeFsSuperBlock.m_Header[0].size;
					if (!encryptXorFile(m_pExeFsXorFileName, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2] + nXorOffset, m_nOffsetAndSize[kOffsetSizeIndexExeFs * 2 + 1] - nXorOffset, nXorOffset, "exefs other section"))
					{
						bResult = false;
					}
				}
			}
		}
		if (m_bRomFsAutoKey)
		{
			if (!encryptAesCtrFile(m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2], m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2 + 1], 0, "romfs"))
			{
				bResult = false;
			}
		}
		else if (!encryptXorFile(m_sRomFsXorFileName, m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2], m_nOffsetAndSize[kOffsetSizeIndexRomFs * 2 + 1], 0, "romfs"))
		{
			bResult = false;
		}
	}
	return true;
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

bool CNcch::IsCxiFile(const char* a_pFileName)
{
	FILE* fp = Fopen(a_pFileName, "rb");
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

bool CNcch::IsCfaFile(const char* a_pFileName)
{
	FILE* fp = Fopen(a_pFileName, "rb");
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

string& CNcch::getExtKey()
{
	return m_sExtKey;
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
	m_Key = 0;
	CBigNum keyX;
	switch (m_NcchHeader.Ncch.Flags[Encrypt7x])
	{
	case 0x01:
		keyX = s_Slot0x25KeyX;
		break;
	case 0x0A:
		keyX = s_Slot0x18KeyX;
		break;
	case 0x0B:
		keyX = s_Slot0x1BKeyX;
		break;
	default:
		return;
	}
	string sKeyY;
	for (int i = 0; i < 16; i++)
	{
		sKeyY += Format("%02X", m_NcchHeader.RSASignature[i]);
	}
	if ((m_NcchHeader.Ncch.Flags[Flag] & 0x20) != 0)
	{
		map<string, string> mExtKey;
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
			vector<string> vTxt = SplitOf<string>(sTxt, "\r\n");
			for (vector<string>::const_iterator it = vTxt.begin(); it != vTxt.end(); ++it)
			{
				sTxt = Trim(*it);
				if (!sTxt.empty() && !StartWith(sTxt, "//"))
				{
					vector<string> vExtKey = Split(sTxt, " ");
					if (vExtKey.size() == 2)
					{
						if (!mExtKey.insert(make_pair(vExtKey[0], vExtKey[1])).second)
						{
							printf("INFO: multiple ext key for %s\n", vExtKey[0].c_str());
						}
					}
					else
					{
						printf("INFO: unknown ext key record %s\n", sTxt.c_str());
					}
				}
			}
		}
		u8* pProgramId = reinterpret_cast<u8*>(&m_NcchHeader.Ncch.ProgramId);
		string sProgramId;
		for (int i = 0; i < 8; i++)
		{
			sProgramId += Format("%02X", pProgramId[7 - i]);
		}
		map<string, string>::const_iterator it = mExtKey.find(sProgramId);
		if (it != mExtKey.end())
		{
			m_sExtKey = it->second;
		}
		else
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);
			CURL* pCURL = curl_easy_init();
			if (pCURL != nullptr)
			{
				curl_easy_setopt(pCURL, CURLOPT_URL, Format("https://kagiya-ctr.cdn.nintendo.net/title/0x%s/ext_key?country=JP", sProgramId.c_str()).c_str());
				curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYPEER, 0L);
				curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYHOST, 0L);
				curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, &CNcch::onDownload);
				curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, this);
				CURLcode code = curl_easy_perform(pCURL);
				if (code != CURLE_OK)
				{
					printf("INFO: curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
					curl_easy_cleanup(pCURL);
					curl_global_cleanup();
					return;
				}
				curl_easy_cleanup(pCURL);
			}
			curl_global_cleanup();
			string sExtKey = m_sExtKey;
			m_sExtKey.clear();
			for (int i = 0; i < static_cast<int>(sExtKey.size()); i++)
			{
				m_sExtKey += Format("%02X", static_cast<u8>(sExtKey[i]));
			}
			mExtKey.insert(make_pair(sProgramId, m_sExtKey));
			fp = UFopen(sExtKeyPath.c_str(), USTR("wb"));
			if (fp != nullptr)
			{
				for (map<string, string>::const_iterator it = mExtKey.begin(); it != mExtKey.end(); ++it)
				{
					fprintf(fp, "%s %s\r\n", it->first.c_str(), it->second.c_str());
				}
				fclose(fp);
			}
		}
		if (m_sExtKey.size() != 32 || m_sExtKey.find_first_not_of("0123456789ABCDEFabcdef") != string::npos)
		{
			printf("ERROR: can not find ext key for %s\n\n", sProgramId.c_str());
			return;
		}
		string sExtKeyWithProgramId = m_sExtKey;
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
				printf("ERROR: ext key verification failed\n\n");
				return;
			}
		}
		sKeyY += m_sExtKey;
		bigNum = sKeyY.c_str();
		bigNum.GetBytes(uBytes, 32);
		SHA256(uBytes, 32, uSHA256);
		sKeyY.clear();
		for (int i = 0; i < 16; i++)
		{
			sKeyY += Format("%02X", uSHA256[i]);
		}
	}
	CBigNum keyY = sKeyY.c_str();
	m_Key = ((keyX.Crol(2, 128) ^ keyY) + "1FF9E9AAC5FE0408024591DC5D52768A").Crol(87, 128);
}

void CNcch::calculateCounter(EAesCtrType a_eAesCtrType)
{
	m_Counter = 0;
	if (m_NcchHeader.Ncch.NcchVersion == 2 || m_NcchHeader.Ncch.NcchVersion == 0)
	{
		u8* pPartitionId = reinterpret_cast<u8*>(&m_NcchHeader.Ncch.PartitionId);
		string sCounter;
		for (int i = 0; i < 8; i++)
		{
			sCounter += Format("%02X", pPartitionId[7 - i]);
		}
		m_Counter = sCounter.c_str();
		m_Counter = (m_Counter << 8 | a_eAesCtrType) << 56;
	}
	else if (m_NcchHeader.Ncch.NcchVersion == 1)
	{
		u8* pPartitionId = reinterpret_cast<u8*>(&m_NcchHeader.Ncch.PartitionId);
		string sCounter;
		for (int i = 0; i < 8; i++)
		{
			sCounter += Format("%02X", pPartitionId[i]);
		}
		n64 nSize = 0;
		switch (a_eAesCtrType)
		{
		case kAesCtrTypeExtendedHeader:
			nSize = sizeof(m_NcchHeader);
			break;
		case kAesCtrTypeExeFs:
			nSize = m_NcchHeader.Ncch.ExeFsOffset * m_nMediaUnitSize;
			break;
		case kAesCtrTypeRomFs:
			nSize = m_NcchHeader.Ncch.RomFsOffset * m_nMediaUnitSize;
			break;
		default:
			break;
		}
		sCounter += static_cast<u32>(nSize);
	}
}

bool CNcch::extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, bool a_bPlainData, const char* a_pType)
{
	bool bResult = true;
	if (a_pFileName != nullptr)
	{
		if (a_nSize != 0)
		{
			FILE* fp = Fopen(a_pFileName, "wb");
			if (fp == nullptr)
			{
				bResult = false;
			}
			else
			{
				if (m_bVerbose)
				{
					printf("save: %s\n", a_pFileName);
				}
				if (a_bPlainData || m_nEncryptMode == kEncryptModeNone || (m_nEncryptMode == kEncryptModeXor && m_sXorFileName.empty()))
				{
					CopyFile(fp, m_fpNcch, a_nOffset, a_nSize);
				}
				else if (m_nEncryptMode == kEncryptModeAesCtr)
				{
					FEncryptAesCtrCopyFile(fp, m_fpNcch, m_Key, m_Counter, a_nOffset, a_nSize);
				}
				else if (m_nEncryptMode == kEncryptModeXor)
				{
					FEncryptXorCopyFile(fp, m_fpNcch, m_sXorFileName, a_nOffset, a_nSize);
				}
				fclose(fp);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: %s is not exists, %s will not be create\n", a_pType, a_pFileName);
		}
	}
	else if (a_nSize != 0 && m_bVerbose)
	{
		printf("INFO: %s is not extract\n", a_pType);
	}
	return bResult;
}

bool CNcch::createHeader()
{
	FILE* fp = Fopen(m_pHeaderFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		printf("load: %s\n", m_pHeaderFileName);
	}
	Fseek(fp, 0, SEEK_END);
	n64 nFileSize = Ftell(fp);
	if (nFileSize < sizeof(m_NcchHeader))
	{
		fclose(fp);
		printf("ERROR: ncch header is too short\n\n");
		return false;
	}
	Fseek(fp, 0, SEEK_SET);
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, fp);
	fclose(fp);
	if (m_nEncryptMode == kEncryptModeAesCtr)
	{
		static const CBigNum c_FiexedCryptoKey = 0;
		if (m_Key == c_FiexedCryptoKey)
		{
			m_NcchHeader.Ncch.Flags[Encrypt7x] = 0;
			m_NcchHeader.Ncch.Flags[Flag] &= ~(1 << NoEncrypto);
			m_NcchHeader.Ncch.Flags[Flag] |= 1 << FixedCryptoKey;
		}
	}
	fwrite(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	return true;
}

bool CNcch::createExtendedHeader()
{
	if (m_pExtendedHeaderFileName != nullptr)
	{
		FILE* fp = Fopen(m_pExtendedHeaderFileName, "rb");
		if (fp == nullptr)
		{
			clearExtendedHeader();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pExtendedHeaderFileName);
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		if (nFileSize < sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended))
		{
			fclose(fp);
			clearExtendedHeader();
			printf("ERROR: extendedheader is too short\n\n");
			return false;
		}
		m_NcchHeader.Ncch.ExtendedHeaderSize = sizeof(NcchExtendedHeader);
		Fseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended)];
		fread(pBuffer, 1, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended), fp);
		if (!m_bNotUpdateExtendedHeaderHash)
		{
			SHA256(pBuffer, m_NcchHeader.Ncch.ExtendedHeaderSize, m_NcchHeader.Ncch.ExtendedHeaderHash);
		}
		delete[] pBuffer;
		if (m_nEncryptMode == kEncryptModeNone || (m_nEncryptMode == kEncryptModeXor && m_pExtendedHeaderXorFileName == nullptr))
		{
			CopyFile(m_fpNcch, fp, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended));
		}
		else if (m_nEncryptMode == kEncryptModeAesCtr)
		{
			calculateCounter(kAesCtrTypeExtendedHeader);
			FEncryptAesCtrCopyFile(m_fpNcch, fp, m_Key, m_Counter, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended));
		}
		else if (m_nEncryptMode == kEncryptModeXor && !FEncryptXorCopyFile(m_fpNcch, fp, m_pExtendedHeaderXorFileName, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended)))
		{
			fclose(fp);
			clearExtendedHeader();
			return false;
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
	if (m_pLogoRegionFileName != nullptr)
	{
		FILE* fp = Fopen(m_pLogoRegionFileName, "rb");
		if (fp == nullptr)
		{
			clearLogoRegion();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pLogoRegionFileName);
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
	if (m_pPlainRegionFileName != nullptr)
	{
		FILE* fp = Fopen(m_pPlainRegionFileName, "rb");
		if (fp == nullptr)
		{
			clearPlainRegion();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pPlainRegionFileName);
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
	if (m_pExeFsFileName != nullptr)
	{
		FILE* fp = Fopen(m_pExeFsFileName, "rb");
		if (fp == nullptr)
		{
			clearExeFs();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pExeFsFileName);
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		n64 nSuperBlockSize = Align(sizeof(ExeFsSuperBlock), m_nMediaUnitSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearExeFs();
			printf("ERROR: exefs is too short\n\n");
			return false;
		}
		m_NcchHeader.Ncch.ExeFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.ExeFsSize = static_cast<u32>(Align(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		if (!m_bNotUpdateExeFsHash)
		{
			m_NcchHeader.Ncch.ExeFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
		}
		Fseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<size_t>(nSuperBlockSize)];
		fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
		if (!m_bNotUpdateExeFsHash)
		{
			SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.ExeFsSuperBlockHash);
		}
		calculateCounter(kAesCtrTypeExeFs);
		if (m_pExeFsXorFileName != nullptr && (m_pExeFsTopXorFileName != nullptr || m_bExeFsTopAutoKey))
		{
			Fseek(fp, 0, SEEK_SET);
			u8* pExeFs = new u8[static_cast<size_t>(nFileSize)];
			fread(pExeFs, 1, static_cast<size_t>(nFileSize), fp);
			ExeFsSuperBlock& exeFsSuperBlock = *reinterpret_cast<ExeFsSuperBlock*>(pBuffer);
			bool bEncryptResult = true;
			if (!CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
			{
				bEncryptResult = FEncryptXorData(&exeFsSuperBlock, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), 0);
			}
			if (bEncryptResult)
			{
				n64 nXorOffset = 0;
				if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), nXorOffset))
				{
					bEncryptResult = false;
				}
				nXorOffset += sizeof(exeFsSuperBlock);
				if (m_bExeFsTopAutoKey)
				{
					FEncryptAesCtrData(pExeFs + nXorOffset, m_Key, m_Counter, exeFsSuperBlock.m_Header[0].size, nXorOffset);
				}
				else if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsTopXorFileName, exeFsSuperBlock.m_Header[0].size, nXorOffset))
				{
					bEncryptResult = false;
				}
				nXorOffset += exeFsSuperBlock.m_Header[0].size;
				if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, nFileSize - nXorOffset, nXorOffset))
				{
					bEncryptResult = false;
				}
			}
			if (bEncryptResult)
			{
				fwrite(pExeFs, 1, static_cast<size_t>(nFileSize), m_fpNcch);
			}
			else
			{
				delete[] pExeFs;
				delete[] pBuffer;
				CopyFile(m_fpNcch, fp, 0, nFileSize);
				fclose(fp);
				return false;
			}
			delete[] pExeFs;
		}
		else
		{
			delete[] pBuffer;
			if (m_nEncryptMode == kEncryptModeNone || (m_nEncryptMode == kEncryptModeXor && m_pExeFsXorFileName == nullptr))
			{
				CopyFile(m_fpNcch, fp, 0, nFileSize);
			}
			else if (m_nEncryptMode == kEncryptModeAesCtr)
			{
				FEncryptAesCtrCopyFile(m_fpNcch, fp, m_Key, m_Counter, 0, nFileSize);
			}
			else if (m_nEncryptMode == kEncryptModeXor && !FEncryptXorCopyFile(m_fpNcch, fp, m_pExeFsXorFileName, 0, nFileSize))
			{
				fclose(fp);
				clearExeFs();
				return false;
			}
		}
		fclose(fp);
	}
	else
	{
		clearExeFs();
	}
	return true;
}

bool CNcch::createRomFs()
{
	if (m_pRomFsFileName != nullptr)
	{
		bool bEncrypted = !CRomFs::IsRomFsFile(m_pRomFsFileName);
		FILE* fp = Fopen(m_pRomFsFileName, "rb");
		if (fp == nullptr)
		{
			clearRomFs();
			return false;
		}
		if (bEncrypted && !m_bNotUpdateRomFsHash)
		{
			printf("INFO: romfs is encrypted\n");
			m_bNotUpdateRomFsHash = true;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pRomFsFileName);
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		if (!m_bNotUpdateRomFsHash)
		{
			n64 nSuperBlockSize = Align(sizeof(SRomFsHeader), CRomFs::s_nSHA256BlockSize);
			if (nFileSize < nSuperBlockSize)
			{
				fclose(fp);
				clearRomFs();
				printf("ERROR: romfs is too short\n\n");
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
				printf("ERROR: romfs is too short\n\n");
				return false;
			}
			m_NcchHeader.Ncch.RomFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
			Fseek(fp, 0, SEEK_SET);
			u8* pBuffer = new u8[static_cast<size_t>(nSuperBlockSize)];
			fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
			SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.RomFsSuperBlockHash);
			delete[] pBuffer;
		}
		m_NcchHeader.Ncch.RomFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.RomFsSize = static_cast<u32>(Align(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		calculateCounter(kAesCtrTypeRomFs);
		if (m_nEncryptMode == kEncryptModeNone || (m_nEncryptMode == kEncryptModeXor && m_sRomFsXorFileName.empty() && !m_bRomFsAutoKey))
		{
			CopyFile(m_fpNcch, fp, 0, nFileSize);
		}
		else if (m_nEncryptMode == kEncryptModeAesCtr)
		{
			FEncryptAesCtrCopyFile(m_fpNcch, fp, m_Key, m_Counter, 0, nFileSize);
		}
		else if (m_nEncryptMode == kEncryptModeXor)
		{
			if (m_bRomFsAutoKey)
			{
				FEncryptAesCtrCopyFile(m_fpNcch, fp, m_Key, m_Counter, 0, nFileSize);
			}
			else if (!FEncryptXorCopyFile(m_fpNcch, fp, m_sRomFsXorFileName, 0, nFileSize))
			{
				fclose(fp);
				clearRomFs();
				return false;
			}
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

bool CNcch::encryptAesCtrFile(n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const char* a_pType)
{
	bool bResult = true;
	if (a_nSize != 0)
	{
		bResult = FEncryptAesCtrFile(m_pFileName, m_Key, m_Counter, a_nOffset, a_nSize, false, a_nXorOffset);
	}
	else if (m_bVerbose)
	{
		printf("INFO: %s is not exists\n", a_pType);
	}
	return bResult;
}

bool CNcch::encryptXorFile(const string& a_sXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const char* a_pType)
{
	bool bResult = true;
	if (!a_sXorFileName.empty())
	{
		if (a_nSize != 0)
		{
			bResult = FEncryptXorFile(m_pFileName, a_sXorFileName, a_nOffset, a_nSize, false, a_nXorOffset);
		}
		else if (m_bVerbose)
		{
			printf("INFO: %s is not exists\n", a_pType);
		}
	}
	else if (a_nSize != 0 && m_bVerbose)
	{
		printf("INFO: %s is not decrypt or encrypt\n", a_pType);
	}
	return bResult;
}

size_t CNcch::onDownload(char* a_pData, size_t a_uSize, size_t a_uNmemb, void* a_pUserData)
{
	size_t uSize = a_uSize * a_uNmemb;
	CNcch* pNcch = reinterpret_cast<CNcch*>(a_pUserData);
	if (pNcch != nullptr)
	{
		string& sExtKey = pNcch->getExtKey();
		sExtKey.append(a_pData, uSize);
	}
	return uSize;
}
