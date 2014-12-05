#include "ncch.h"
#include "exefs.h"
#include "extendedheader.h"
#include "romfs.h"
#include <openssl/sha.h>

const u32 CNcch::s_uSignature = CONVERT_ENDIAN('NCCH');
const int CNcch::s_nBlockSize = 0x1000;

CNcch::CNcch()
	: m_pFileName(nullptr)
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
	, m_pRomFsXorFileName(nullptr)
	, m_fpNcch(nullptr)
	, m_nMediaUnitSize(1 << 9)
	, m_nExtendedHeaderOffset(0)
	, m_nExtendedHeaderSize(0)
	, m_nLogoRegionOffset(0)
	, m_nLogoRegionSize(0)
	, m_nPlainRegionOffset(0)
	, m_nPlainRegionSize(0)
	, m_nExeFsOffset(0)
	, m_nExeFsSize(0)
	, m_nRomFsOffset(0)
	, m_nRomFsSize(0)
	, m_pXorFileName(nullptr)
{
	memset(m_uKey, 0, sizeof(m_uKey));
	memset(&m_NcchHeader, 0, sizeof(m_NcchHeader));
	memset(m_uAesCtr, 0, sizeof(m_uAesCtr));
}

CNcch::~CNcch()
{
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

void CNcch::SetKey(u8 a_uKey[16])
{
	memcpy(m_uKey, a_uKey, sizeof(m_uKey));
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

void CNcch::SetRomFsXorFileName(const char* a_pRomFsXorFileName)
{
	m_pRomFsXorFileName = a_pRomFsXorFileName;
}

bool CNcch::ExtractFile()
{
	bool bResult = true;
	m_fpNcch = FFopen(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	calculateMediaUnitSize();
	calculateOffsetSize();
	if (!extractFile(m_pHeaderFileName, 0, sizeof(m_NcchHeader), true, "ncch header"))
	{
		bResult = false;
	}
	getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeExtendedHeader, m_nMediaUnitSize, m_uAesCtr);
	m_pXorFileName = m_pExtendedHeaderXorFileName;
	if (!extractFile(m_pExtendedHeaderFileName, m_nExtendedHeaderOffset, m_nExtendedHeaderSize, false, "extendedheader"))
	{
		bResult = false;
	}
	if (!extractFile(m_pLogoRegionFileName, m_nLogoRegionOffset, m_nLogoRegionSize, true, "logoregion"))
	{
		bResult = false;
	}
	if (!extractFile(m_pPlainRegionFileName, m_nPlainRegionOffset, m_nPlainRegionSize, true, "plainregion"))
	{
		bResult = false;
	}
	if (m_pExeFsXorFileName != nullptr && m_pExeFsTopXorFileName != nullptr && m_nExeFsSize != 0)
	{
		FFseek(m_fpNcch, m_nExeFsOffset, SEEK_SET);
		ExeFsSuperBlock exeFsSuperBlock;
		fread(&exeFsSuperBlock, sizeof(exeFsSuperBlock), 1, m_fpNcch);
		FFseek(m_fpNcch, m_nExeFsOffset, SEEK_SET);
		u8* pExeFs = new u8[static_cast<size_t>(m_nExeFsSize)];
		fread(pExeFs, 1, static_cast<size_t>(m_nExeFsSize), m_fpNcch);
		bool bEncryptResult = true;
		if (!CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
		{
			bEncryptResult = FEncryptXorData(&exeFsSuperBlock, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), 0, m_bVerbose);
		}
		if (bEncryptResult)
		{
			n64 nXorOffset = 0;
			if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), nXorOffset, m_bVerbose))
			{
				bEncryptResult = false;
			}
			nXorOffset += sizeof(exeFsSuperBlock);
			if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsTopXorFileName, exeFsSuperBlock.m_Header[0].size, nXorOffset, m_bVerbose))
			{
				bEncryptResult = false;
			}
			nXorOffset += exeFsSuperBlock.m_Header[0].size;
			if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, m_nExeFsSize - nXorOffset, nXorOffset, m_bVerbose))
			{
				bEncryptResult = false;
			}
		}
		if (bEncryptResult)
		{
			FILE* fp = FFopen(m_pExeFsFileName, "wb");
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
				fwrite(pExeFs, 1, static_cast<size_t>(m_nExeFsSize), fp);
				fclose(fp);
			}
			delete[] pExeFs;
		}
		else
		{
			delete[] pExeFs;
			bResult = false;
			extractFile(m_pExeFsFileName, m_nExeFsOffset, m_nExeFsSize, true, "exefs");
		}
	}
	else
	{
		getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeExeFs, m_nMediaUnitSize, m_uAesCtr);
		m_pXorFileName = m_pExeFsXorFileName;
		if (!extractFile(m_pExeFsFileName, m_nExeFsOffset, m_nExeFsSize, false, "exefs"))
		{
			bResult = false;
		}
	}
	getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeRomFs, m_nMediaUnitSize, m_uAesCtr);
	m_pXorFileName = m_pRomFsXorFileName;
	if (!extractFile(m_pRomFsFileName, m_nRomFsOffset, m_nRomFsSize, false, "romfs"))
	{
		bResult = false;
	}
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::CreateFile()
{
	bool bResult = true;
	m_fpNcch = FFopen(m_pFileName, "wb");
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
	alignFileSize(s_nBlockSize);
	if (!createRomFs())
	{
		bResult = false;
	}
	alignFileSize(s_nBlockSize);
	FFseek(m_fpNcch, 0, SEEK_SET);
	fwrite(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::EncryptFile()
{
	bool bResult = true;
	m_fpNcch = FFopen(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	fclose(m_fpNcch);
	calculateMediaUnitSize();
	calculateOffsetSize();
	if (m_nEncryptMode == kEncryptModeAesCtr)
	{
		getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeExtendedHeader, m_nMediaUnitSize, m_uAesCtr);
		if (!encryptAesCtrFile(m_nExtendedHeaderOffset, m_nExtendedHeaderSize, "extendedheader"))
		{
			bResult = false;
		}
		getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeExeFs, m_nMediaUnitSize, m_uAesCtr);
		if (!encryptAesCtrFile(m_nExeFsOffset, m_nExeFsSize, "exefs"))
		{
			bResult = false;
		}
		getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeRomFs, m_nMediaUnitSize, m_uAesCtr);
		if (!encryptAesCtrFile(m_nRomFsOffset, m_nRomFsSize, "romfs"))
		{
			bResult = false;
		}
	}
	else if (m_nEncryptMode == kEncryptModeXor)
	{
		if (!encryptXorFile(m_pExtendedHeaderXorFileName, m_nExtendedHeaderOffset, m_nExtendedHeaderSize, 0, "extendedheader"))
		{
			bResult = false;
		}
		if (m_pExeFsTopXorFileName == nullptr)
		{
			if (!encryptXorFile(m_pExeFsXorFileName, m_nExeFsOffset, m_nExeFsSize, 0, "exefs"))
			{
				bResult = false;
			}
		}
		else if (m_nExeFsSize != 0)
		{
			m_fpNcch = FFopen(m_pFileName, "rb");
			if (m_fpNcch == nullptr)
			{
				bResult = false;
			}
			else
			{
				FFseek(m_fpNcch, m_nExeFsOffset, SEEK_SET);
				ExeFsSuperBlock exeFsSuperBlock;
				fread(&exeFsSuperBlock, sizeof(exeFsSuperBlock), 1, m_fpNcch);
				fclose(m_fpNcch);
				bool bEncryptResult = true;
				if (!CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
				{
					bEncryptResult = FEncryptXorData(&exeFsSuperBlock, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), 0, m_bVerbose);
				}
				if (!bEncryptResult)
				{
					bResult = false;
				}
				else
				{
					n64 nXorOffset = 0;
					if (!encryptXorFile(m_pExeFsXorFileName, m_nExeFsOffset + nXorOffset, sizeof(exeFsSuperBlock), nXorOffset, "exefs super block"))
					{
						bResult = false;
					}
					nXorOffset += sizeof(exeFsSuperBlock);
					if (!encryptXorFile(m_pExeFsTopXorFileName, m_nExeFsOffset + nXorOffset, exeFsSuperBlock.m_Header[0].size, nXorOffset, "exefs top section"))
					{
						bResult = false;
					}
					nXorOffset += exeFsSuperBlock.m_Header[0].size;
					if (!encryptXorFile(m_pExeFsXorFileName, m_nExeFsOffset + nXorOffset, m_nExeFsSize - nXorOffset, nXorOffset, "exefs other section"))
					{
						bResult = false;
					}
				}
			}
		}
		if (!encryptXorFile(m_pRomFsXorFileName, m_nRomFsOffset, m_nRomFsSize, 0, "romfs"))
		{
			bResult = false;
		}
	}
	return true;
}

bool CNcch::IsCxiFile(const char* a_pFileName)
{
	FILE* fp = FFopen(a_pFileName, "rb");
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
	FILE* fp = FFopen(a_pFileName, "rb");
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
	m_nExtendedHeaderOffset = sizeof(m_NcchHeader);
	m_nExtendedHeaderSize = m_NcchHeader.Ncch.ExtendedHeaderSize == sizeof(NcchExtendedHeader) ? sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended) : 0;
	m_nLogoRegionOffset = m_NcchHeader.Ncch.LogoRegionOffset * m_nMediaUnitSize;
	m_nLogoRegionSize = m_NcchHeader.Ncch.LogoRegionSize * m_nMediaUnitSize;
	m_nPlainRegionOffset = m_NcchHeader.Ncch.PlainRegionOffset * m_nMediaUnitSize;
	m_nPlainRegionSize = m_NcchHeader.Ncch.PlainRegionSize * m_nMediaUnitSize;
	m_nExeFsOffset = m_NcchHeader.Ncch.ExeFsOffset * m_nMediaUnitSize;
	m_nExeFsSize = m_NcchHeader.Ncch.ExeFsSize * m_nMediaUnitSize;
	m_nRomFsOffset = m_NcchHeader.Ncch.RomFsOffset * m_nMediaUnitSize;
	m_nRomFsSize = m_NcchHeader.Ncch.RomFsSize * m_nMediaUnitSize;
}

bool CNcch::extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, bool a_bPlainData, const char* a_pType)
{
	bool bResult = true;
	if (a_pFileName != nullptr)
	{
		if (a_nSize != 0)
		{
			FILE* fp = FFopen(a_pFileName, "wb");
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
				if (a_bPlainData || m_nEncryptMode == kEncryptModeNone || (m_nEncryptMode == kEncryptModeXor && m_pXorFileName == nullptr))
				{
					FCopyFile(fp, m_fpNcch, a_nOffset, a_nSize);
				}
				else if (m_nEncryptMode == kEncryptModeAesCtr)
				{
					FEncryptAesCtrCopyFile(fp, m_fpNcch, m_uKey, m_uAesCtr, a_nOffset, a_nSize, m_bVerbose);
				}
				else if (m_nEncryptMode == kEncryptModeXor)
				{
					FEncryptXorCopyFile(fp, m_fpNcch, m_pXorFileName, a_nOffset, a_nSize, m_bVerbose);
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
	FILE* fp = FFopen(m_pHeaderFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		printf("load: %s\n", m_pHeaderFileName);
	}
	FFseek(fp, 0, SEEK_END);
	n64 nFileSize = FFtell(fp);
	if (nFileSize < sizeof(m_NcchHeader))
	{
		fclose(fp);
		printf("ERROR: ncch header is too short\n\n");
		return false;
	}
	FFseek(fp, 0, SEEK_SET);
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, fp);
	fclose(fp);
	if (m_nEncryptMode == kEncryptModeAesCtr)
	{
		static const u8 uFixedCryptoKey[16] = {};
		if (memcmp(m_uKey, uFixedCryptoKey, sizeof(uFixedCryptoKey)) == 0)
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
		FILE* fp = FFopen(m_pExtendedHeaderFileName, "rb");
		if (fp == nullptr)
		{
			clearExtendedHeader();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pExtendedHeaderFileName);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		if (nFileSize < sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended))
		{
			fclose(fp);
			clearExtendedHeader();
			printf("ERROR: extendedheader is too short\n\n");
			return false;
		}
		m_NcchHeader.Ncch.ExtendedHeaderSize = sizeof(NcchExtendedHeader);
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended)];
		fread(pBuffer, 1, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended), fp);
		if (!m_bNotUpdateExtendedHeaderHash)
		{
			SHA256(pBuffer, m_NcchHeader.Ncch.ExtendedHeaderSize, m_NcchHeader.Ncch.ExtendedHeaderHash);
		}
		delete[] pBuffer;
		if (m_nEncryptMode == kEncryptModeNone || (m_nEncryptMode == kEncryptModeXor && m_pExtendedHeaderXorFileName == nullptr))
		{
			FCopyFile(m_fpNcch, fp, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended));
		}
		else if (m_nEncryptMode == kEncryptModeAesCtr)
		{
			getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeExtendedHeader, m_nMediaUnitSize, m_uAesCtr);
			FEncryptAesCtrCopyFile(m_fpNcch, fp, m_uKey, m_uAesCtr, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended), m_bVerbose);
		}
		else if (m_nEncryptMode == kEncryptModeXor && !FEncryptXorCopyFile(m_fpNcch, fp, m_pExtendedHeaderXorFileName, 0, sizeof(NcchExtendedHeader) + sizeof(NcchAccessControlExtended), m_bVerbose))
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
		FILE* fp = FFopen(m_pLogoRegionFileName, "rb");
		if (fp == nullptr)
		{
			clearLogoRegion();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pLogoRegionFileName);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		n64 nLogoRegionSize = FAlign(nFileSize, m_nMediaUnitSize);
		m_NcchHeader.Ncch.LogoRegionOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.LogoRegionSize = static_cast<u32>(nLogoRegionSize / m_nMediaUnitSize);
		FFseek(fp, 0, SEEK_SET);
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
		FILE* fp = FFopen(m_pPlainRegionFileName, "rb");
		if (fp == nullptr)
		{
			clearPlainRegion();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pPlainRegionFileName);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		m_NcchHeader.Ncch.PlainRegionOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.PlainRegionSize = static_cast<u32>(FAlign(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		FFseek(fp, 0, SEEK_SET);
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
		FILE* fp = FFopen(m_pExeFsFileName, "rb");
		if (fp == nullptr)
		{
			clearExeFs();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pExeFsFileName);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		n64 nSuperBlockSize = FAlign(sizeof(ExeFsSuperBlock), m_nMediaUnitSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearExeFs();
			printf("ERROR: exefs is too short\n\n");
			return false;
		}
		m_NcchHeader.Ncch.ExeFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.ExeFsSize = static_cast<u32>(FAlign(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		if (!m_bNotUpdateExeFsHash)
		{
			m_NcchHeader.Ncch.ExeFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
		}
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<size_t>(nSuperBlockSize)];
		fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
		if (!m_bNotUpdateExeFsHash)
		{
			SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.ExeFsSuperBlockHash);
		}
		if (m_pExeFsXorFileName != nullptr && m_pExeFsTopXorFileName != nullptr)
		{
			FFseek(fp, 0, SEEK_SET);
			u8* pExeFs = new u8[static_cast<size_t>(nFileSize)];
			fread(pExeFs, 1, static_cast<size_t>(nFileSize), fp);
			ExeFsSuperBlock& exeFsSuperBlock = *reinterpret_cast<ExeFsSuperBlock*>(pBuffer);
			bool bEncryptResult = true;
			if (!CExeFs::IsExeFsSuperBlock(exeFsSuperBlock))
			{
				bEncryptResult = FEncryptXorData(&exeFsSuperBlock, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), 0, m_bVerbose);
			}
			if (bEncryptResult)
			{
				n64 nXorOffset = 0;
				if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, sizeof(exeFsSuperBlock), nXorOffset, m_bVerbose))
				{
					bEncryptResult = false;
				}
				nXorOffset += sizeof(exeFsSuperBlock);
				if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsTopXorFileName, exeFsSuperBlock.m_Header[0].size, nXorOffset, m_bVerbose))
				{
					bEncryptResult = false;
				}
				nXorOffset += exeFsSuperBlock.m_Header[0].size;
				if (!FEncryptXorData(pExeFs + nXorOffset, m_pExeFsXorFileName, m_nExeFsSize - nXorOffset, nXorOffset, m_bVerbose))
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
				FCopyFile(m_fpNcch, fp, 0, nFileSize);
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
				FCopyFile(m_fpNcch, fp, 0, nFileSize);
			}
			else if (m_nEncryptMode == kEncryptModeAesCtr)
			{
				getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeExeFs, m_nMediaUnitSize, m_uAesCtr);
				FEncryptAesCtrCopyFile(m_fpNcch, fp, m_uKey, m_uAesCtr, 0, nFileSize, m_bVerbose);
			}
			else if (m_nEncryptMode == kEncryptModeXor && !FEncryptXorCopyFile(m_fpNcch, fp, m_pExeFsXorFileName, 0, nFileSize, m_bVerbose))
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
		FILE* fp = FFopen(m_pRomFsFileName, "rb");
		if (fp == nullptr)
		{
			clearRomFs();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pRomFsFileName);
		}
		SRomFsHeader romFsHeader;
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		if (nFileSize < sizeof(romFsHeader))
		{
			fclose(fp);
			clearRomFs();
			printf("ERROR: romfs is too short\n\n");
			return false;
		}
		FFseek(fp, 0, SEEK_SET);
		fread(&romFsHeader, sizeof(romFsHeader), 1, fp);
		n64 nSuperBlockSize = FAlign(FAlign(sizeof(romFsHeader), CRomFs::s_nSHA256BlockSize) + romFsHeader.Level0Size, m_nMediaUnitSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearRomFs();
			printf("ERROR: romfs is too short\n\n");
			return false;
		}
		m_NcchHeader.Ncch.RomFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.RomFsSize = static_cast<u32>(FAlign(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		if (!m_bNotUpdateRomFsHash)
		{
			m_NcchHeader.Ncch.RomFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
		}
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<size_t>(nSuperBlockSize)];
		fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
		if (!m_bNotUpdateRomFsHash)
		{
			SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.RomFsSuperBlockHash);
		}
		delete[] pBuffer;
		if (m_nEncryptMode == kEncryptModeNone || (m_nEncryptMode == kEncryptModeXor && m_pRomFsXorFileName == nullptr))
		{
			FCopyFile(m_fpNcch, fp, 0, nFileSize);
		}
		else if (m_nEncryptMode == kEncryptModeAesCtr)
		{
			getAesCounter(&m_NcchHeader.Ncch, kAesCtrTypeRomFs, m_nMediaUnitSize, m_uAesCtr);
			FEncryptAesCtrCopyFile(m_fpNcch, fp, m_uKey, m_uAesCtr, 0, nFileSize, m_bVerbose);
		}
		else if (m_nEncryptMode == kEncryptModeXor && !FEncryptXorCopyFile(m_fpNcch, fp, m_pRomFsXorFileName, 0, nFileSize, m_bVerbose))
		{
			fclose(fp);
			clearRomFs();
			return false;
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
	FFseek(m_fpNcch, 0, SEEK_END);
	n64 nFileSize = FAlign(FFtell(m_fpNcch), a_nAlignment);
	FSeek(m_fpNcch, nFileSize);
	m_NcchHeader.Ncch.ContentSize = static_cast<u32>(nFileSize / m_nMediaUnitSize);
}

bool CNcch::encryptAesCtrFile(n64 a_nOffset, n64 a_nSize, const char* a_pType)
{
	bool bResult = true;
	if (a_nSize != 0)
	{
		bResult = FEncryptAesCtrFile(m_pFileName, m_uKey, m_uAesCtr, a_nOffset, a_nSize, false, m_bVerbose);
	}
	else if (m_bVerbose)
	{
		printf("INFO: %s is not exists\n", a_pType);
	}
	return bResult;
}

bool CNcch::encryptXorFile(const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const char* a_pType)
{
	bool bResult = true;
	if (a_pXorFileName != nullptr)
	{
		if (a_nSize != 0)
		{
			bResult = FEncryptXorFile(m_pFileName, a_pXorFileName, a_nOffset, a_nSize, false, a_nXorOffset, m_bVerbose);
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

void CNcch::getAesCounter(NcchCommonHeaderStruct* a_pNcch, EAesCtrType a_eAesCtrType, n64 a_nMediaUnitSize, u8 a_uAesCtr[16])
{
	memset(a_uAesCtr, 0, 16);
	if (a_pNcch->NcchVersion == 2 || a_pNcch->NcchVersion == 0)
	{
		u8* pPartitionId = (u8*)&a_pNcch->PartitionId;
		for (int i = 0; i < 8; i++)
		{
			a_uAesCtr[i] = pPartitionId[7 - i];
		}
		a_uAesCtr[8] = a_eAesCtrType;
	}
	else if (a_pNcch->NcchVersion == 1)
	{
		memcpy(a_uAesCtr, &a_pNcch->PartitionId, 8);
		n64 nSize = 0;
		switch (a_eAesCtrType)
		{
		case kAesCtrTypeExtendedHeader:
			nSize = 0x200;
			break;
		case kAesCtrTypeExeFs:
			nSize = a_pNcch->ExeFsOffset * a_nMediaUnitSize;
			break;
		case kAesCtrTypeRomFs:
			nSize = a_pNcch->RomFsOffset * a_nMediaUnitSize;
			break;
		default:
			break;
		}
		u8* pSize = (u8*)&nSize;
		for (int i = 0; i < 4; i++)
		{
			a_uAesCtr[i + 12] = pSize[3 - i];
		}
	}
}
