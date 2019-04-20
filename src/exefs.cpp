#include "exefs.h"
#include "backwardlz77.h"
#include <openssl/sha.h>

const int CExeFs::s_nBlockSize = 0x200;

CExeFs::CExeFs()
	: m_bVerbose(false)
	, m_bUncompress(false)
	, m_bCompress(false)
	, m_fpExeFs(nullptr)
{
	memset(&m_ExeFsSuperBlock, 0, sizeof(m_ExeFsSuperBlock));
	m_mPath[USTR("banner")] = USTR("banner.bnr");
	m_mPath[USTR("icon")] = USTR("icon.icn");
	m_mPath[USTR("logo")] = USTR("logo.darc.lz");
}

CExeFs::~CExeFs()
{
}

void CExeFs::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CExeFs::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CExeFs::SetHeaderFileName(const UString& a_sHeaderFileName)
{
	m_sHeaderFileName = a_sHeaderFileName;
}

void CExeFs::SetExeFsDirName(const UString& a_sExeFsDirName)
{
	m_sExeFsDirName = a_sExeFsDirName;
}

void CExeFs::SetUncompress(bool a_bUncompress)
{
	m_bUncompress = a_bUncompress;
}

void CExeFs::SetCompress(bool a_bCompress)
{
	m_bCompress = a_bCompress;
}

bool CExeFs::ExtractFile()
{
	bool bResult = true;
	m_fpExeFs = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (m_fpExeFs == nullptr)
	{
		return false;
	}
	fread(&m_ExeFsSuperBlock, sizeof(m_ExeFsSuperBlock), 1, m_fpExeFs);
	if (!m_sExeFsDirName.empty())
	{
		if (!UMakeDir(m_sExeFsDirName.c_str()))
		{
			fclose(m_fpExeFs);
			return false;
		}
	}
	if (!extractHeader())
	{
		bResult = false;
	}
	if (!m_sExeFsDirName.empty())
	{
		for (int i = 0; i < 8; i++)
		{
			if (!extractSection(i))
			{
				bResult = false;
			}
		}
	}
	fclose(m_fpExeFs);
	return bResult;
}

bool CExeFs::CreateFile()
{
	bool bResult = true;
	m_fpExeFs = UFopen(m_sFileName.c_str(), USTR("wb"));
	if (m_fpExeFs == nullptr)
	{
		return false;
	}
	if (!createHeader())
	{
		fclose(m_fpExeFs);
		return false;
	}
	for (int i = 0; i < 8; i++)
	{
		if (!createSection(i))
		{
			bResult = false;
			i--;
		}
	}
	Fseek(m_fpExeFs, 0, SEEK_SET);
	fwrite(&m_ExeFsSuperBlock, sizeof(m_ExeFsSuperBlock), 1, m_fpExeFs);
	fclose(m_fpExeFs);
	return bResult;
}

bool CExeFs::IsExeFsFile(const UString& a_sFileName, n64 a_nOffset)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	ExeFsSuperBlock exeFsSuperBlock;
	fread(&exeFsSuperBlock, sizeof(exeFsSuperBlock), 1, fp);
	fclose(fp);
	return IsExeFsSuperBlock(exeFsSuperBlock);
}

bool CExeFs::IsExeFsSuperBlock(const ExeFsSuperBlock& a_ExeFsSuperBlock)
{
	static const u8 c_uReserved[sizeof(a_ExeFsSuperBlock.m_Reserved)] = {};
	return a_ExeFsSuperBlock.m_Header[0].offset == 0 && memcmp(a_ExeFsSuperBlock.m_Reserved, c_uReserved, sizeof(a_ExeFsSuperBlock.m_Reserved)) == 0;
}

bool CExeFs::extractHeader()
{
	bool bResult = true;
	if (!m_sHeaderFileName.empty())
	{
		FILE* fp = UFopen(m_sHeaderFileName.c_str(), USTR("wb"));
		if (fp == nullptr)
		{
			bResult = false;
		}
		else
		{
			if (m_bVerbose)
			{
				UPrintf(USTR("save: %") PRIUS USTR("\n"), m_sHeaderFileName.c_str());
			}
			fwrite(&m_ExeFsSuperBlock, sizeof(m_ExeFsSuperBlock), 1, fp);
			fclose(fp);
		}
	}
	else if (m_bVerbose)
	{
		UPrintf(USTR("INFO: exefs header is not extract\n"));
	}
	return bResult;
}

bool CExeFs::extractSection(int a_nIndex)
{
	bool bResult = true;
	UString sPath;
	UString sName = AToU(reinterpret_cast<const char*>(m_ExeFsSuperBlock.m_Header[a_nIndex].name));
	if (!sName.empty())
	{
		bool bTopSection = false;
		unordered_map<UString, UString>::const_iterator it = m_mPath.find(sName);
		if (it == m_mPath.end())
		{
			if (a_nIndex == 0)
			{
				sPath = m_sExeFsDirName + USTR("/code.bin");
				bTopSection = true;
			}
			else
			{
				if (m_bVerbose)
				{
					UPrintf(USTR("INFO: unknown entry name %") PRIUS USTR("\n"), sName.c_str());
				}
				sPath = m_sExeFsDirName + USTR("/") + sName + USTR(".bin");
			}
		}
		else
		{
			sPath = m_sExeFsDirName + USTR("/") + it->second;
		}
		FILE* fp = UFopen(sPath.c_str(), USTR("wb"));
		if (fp == nullptr)
		{
			bResult = false;
		}
		else
		{
			if (m_bVerbose)
			{
				UPrintf(USTR("save: %") PRIUS USTR("\n"), sPath.c_str());
			}
			if (bTopSection && m_bUncompress)
			{
				u32 uCompressedSize = m_ExeFsSuperBlock.m_Header[a_nIndex].size;
				Fseek(m_fpExeFs, sizeof(m_ExeFsSuperBlock) + m_ExeFsSuperBlock.m_Header[a_nIndex].offset, SEEK_SET);
				u8* pCompressed = new u8[uCompressedSize];
				fread(pCompressed, 1, uCompressedSize, m_fpExeFs);
				u32 uUncompressedSize = 0;
				bResult = CBackwardLz77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
				if (bResult)
				{
					u8* pUncompressed = new u8[uUncompressedSize];
					bResult = CBackwardLz77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
					if (bResult)
					{
						fwrite(pUncompressed, 1, uUncompressedSize, fp);
					}
					else
					{
						UPrintf(USTR("ERROR: uncompress error\n\n"));
					}
					delete[] pUncompressed;
				}
				else
				{
					UPrintf(USTR("ERROR: get uncompressed size error\n\n"));
				}
				delete[] pCompressed;
			}
			if (!bTopSection || !m_bUncompress || !bResult)
			{
				CopyFile(fp, m_fpExeFs, sizeof(m_ExeFsSuperBlock) + m_ExeFsSuperBlock.m_Header[a_nIndex].offset, m_ExeFsSuperBlock.m_Header[a_nIndex].size);
			}
			fclose(fp);
		}
	}
	return bResult;
}

bool CExeFs::createHeader()
{
	FILE* fp = UFopen(m_sHeaderFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	n64 nFileSize = Ftell(fp);
	if (nFileSize < sizeof(m_ExeFsSuperBlock))
	{
		fclose(fp);
		UPrintf(USTR("ERROR: exefs header is too short\n\n"));
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sHeaderFileName.c_str());
	}
	Fseek(fp, 0, SEEK_SET);
	fread(&m_ExeFsSuperBlock, sizeof(m_ExeFsSuperBlock), 1, fp);
	fclose(fp);
	fwrite(&m_ExeFsSuperBlock, sizeof(m_ExeFsSuperBlock), 1, m_fpExeFs);
	return true;
}

bool CExeFs::createSection(int a_nIndex)
{
	bool bResult = true;
	UString sPath;
	UString sName = AToU(reinterpret_cast<const char*>(m_ExeFsSuperBlock.m_Header[a_nIndex].name));
	if (!sName.empty())
	{
		bool bTopSection = false;
		unordered_map<UString, UString>::const_iterator it = m_mPath.find(sName);
		if (it == m_mPath.end())
		{
			if (a_nIndex == 0)
			{
				sPath = m_sExeFsDirName + USTR("/code.bin");
				bTopSection = true;
			}
			else
			{
				if (m_bVerbose)
				{
					UPrintf(USTR("INFO: unknown entry name %") PRIUS USTR("\n"), sName.c_str());
				}
				sPath = m_sExeFsDirName + USTR("/") + sName + USTR(".bin");
			}
		}
		else
		{
			sPath = m_sExeFsDirName + USTR("/") + it->second;
		}
		FILE* fp = UFopen(sPath.c_str(), USTR("rb"));
		if (fp == nullptr)
		{
			clearSection(a_nIndex);
			bResult = false;
		}
		else
		{
			m_ExeFsSuperBlock.m_Header[a_nIndex].offset = static_cast<u32>(Ftell(m_fpExeFs)) - sizeof(m_ExeFsSuperBlock);
			if (m_bVerbose)
			{
				UPrintf(USTR("load: %") PRIUS USTR("\n"), sPath.c_str());
			}
			Fseek(fp, 0, SEEK_END);
			u32 uFileSize = static_cast<u32>(Ftell(fp));
			Fseek(fp, 0, SEEK_SET);
			u8* pData = new u8[uFileSize];
			fread(pData, 1, uFileSize, fp);
			fclose(fp);
			bool bCompressResult = false;
			if (bTopSection && m_bCompress)
			{
				u32 uCompressedSize = uFileSize;
				u8* pCompressed = new u8[uCompressedSize];
				bCompressResult = CBackwardLz77::Compress(pData, uFileSize, pCompressed, uCompressedSize);
				if (bCompressResult)
				{
					SHA256(pCompressed, uCompressedSize, m_ExeFsSuperBlock.m_Hash[7 - a_nIndex]);
					fwrite(pCompressed, 1, uCompressedSize, m_fpExeFs);
					m_ExeFsSuperBlock.m_Header[a_nIndex].size = uCompressedSize;
				}
				delete[] pCompressed;
			}
			if (!bTopSection || !m_bCompress || !bCompressResult)
			{
				SHA256(pData, uFileSize, m_ExeFsSuperBlock.m_Hash[7 - a_nIndex]);
				fwrite(pData, 1, uFileSize, m_fpExeFs);
				m_ExeFsSuperBlock.m_Header[a_nIndex].size = uFileSize;
			}
			delete[] pData;
			PadFile(m_fpExeFs, Align(Ftell(m_fpExeFs), s_nBlockSize) - Ftell(m_fpExeFs), 0);
		}
	}
	return bResult;
}

void CExeFs::clearSection(int a_nIndex)
{
	if (a_nIndex != 7)
	{
		memmove(&m_ExeFsSuperBlock.m_Header[a_nIndex], &m_ExeFsSuperBlock.m_Header[a_nIndex + 1], sizeof(m_ExeFsSuperBlock.m_Header[0]) * (7 - a_nIndex));
	}
	memset(&m_ExeFsSuperBlock.m_Header[7], 0, sizeof(m_ExeFsSuperBlock.m_Header[7]));
	memmove(m_ExeFsSuperBlock.m_Hash[1], m_ExeFsSuperBlock.m_Hash[0], sizeof(m_ExeFsSuperBlock.m_Hash[0]) * (7 - a_nIndex));
	memset(m_ExeFsSuperBlock.m_Hash[0], 0, sizeof(m_ExeFsSuperBlock.m_Hash[0]));
}
