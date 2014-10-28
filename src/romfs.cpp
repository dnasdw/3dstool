#include "romfs.h"

CRomFs::CRomFs()
	: m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_fpRomFsBin(nullptr)
	, m_nLevel3Offset(s_nLevel0BlockSize)
{
	memset(&m_RomFsHeader, 0, sizeof(m_RomFsHeader));
	memset(&m_RomFsLevel3Header, 0, sizeof(m_RomFsLevel3Header));
}

CRomFs::~CRomFs()
{
}

void CRomFs::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CRomFs::SetRomFsDirName(const char* a_pRomFsDirName)
{
#if _3DSTOOL_COMPILER == COMPILER_MSC
	m_sRomFsDirName = FSAToW(a_pRomFsDirName);
#else
	m_sRomFsDirName = a_pRomFsDirName;
#endif
}

void CRomFs::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CRomFs::ExtractFile()
{
	m_fpRomFsBin = fopen(m_pFileName, "rb");
	if (m_fpRomFsBin == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", m_pFileName);
		return false;
	}
	fread(&m_RomFsHeader, sizeof(m_RomFsHeader), 1, m_fpRomFsBin);
	m_nLevel3Offset = FAlign(FAlign(m_RomFsHeader.Size, 0x10) + m_RomFsHeader.Level0HashSize, s_nLevel0BlockSize);
	FFseek(m_fpRomFsBin, m_nLevel3Offset, SEEK_SET);
	fread(&m_RomFsLevel3Header, sizeof(m_RomFsLevel3Header), 1, m_fpRomFsBin);
	m_sExtractStack.push(SStackElement());
	SStackElement& root = m_sExtractStack.top();
	root.IsDir = true;
	root.EntryOffset = 0;
#if _3DSTOOL_COMPILER == COMPILER_MSC
	root.Prefix = L"/";
#else
	root.Prefix = "/";
#endif
	root.ExtractState = kExtractStateBegin;
	bool bResult = true;
	while (!m_sExtractStack.empty())
	{
		SStackElement& current = m_sExtractStack.top();
		if (current.IsDir)
		{
			if (!ExtractDirEntry())
			{
				bResult = false;
			}
		}
		else if (!ExtractFileEntry())
		{
			bResult = false;
		}
	}
	fclose(m_fpRomFsBin);
	m_fpRomFsBin = nullptr;
	return bResult;
}

bool CRomFs::ExtractDirEntry()
{
	SStackElement& current = m_sExtractStack.top();
	if (current.ExtractState == kExtractStateBegin)
	{
		FFseek(m_fpRomFsBin, m_nLevel3Offset + m_RomFsLevel3Header.Section[kSectionTypeDir].Offset + current.EntryOffset, SEEK_SET);
		fread(&current.Entry.Dir, sizeof(current.Entry.Dir), 1, m_fpRomFsBin);
		char16_t* pEntryName = new char16_t[current.Entry.Dir.DirNameSize / 2 + 1];
		fread(pEntryName, 2, current.Entry.Dir.DirNameSize / 2, m_fpRomFsBin);
		pEntryName[current.Entry.Dir.DirNameSize / 2] = 0;
#if _3DSTOOL_COMPILER == COMPILER_MSC
		current.EntryName = FSU16ToW(pEntryName);
		delete[] pEntryName;
		wstring sPrefix = current.Prefix;
		wstring sDirName = m_sRomFsDirName + sPrefix;
		if (current.Entry.Dir.DirNameSize != 0)
		{
			sPrefix += current.EntryName + L"/";
			sDirName += current.EntryName;
		}
		else
		{
			sDirName.erase(sDirName.end() - 1);
		}
#else
		current.EntryName = FSU16ToU8(pEntryName);
		delete[] pEntryName;
		string sPrefix = current.Prefix;
		string sDirName = m_sRomFsDirName + sPrefix;
		if (current.Entry.Dir.DirNameSize != 0)
		{
			sPrefix += current.EntryName + "/";
			sDirName += current.EntryName;
		}
		else
		{
			sDirName.erase(sDirName.end() - 1);
		}
#endif
		if (m_bVerbose)
		{
#if _3DSTOOL_COMPILER == COMPILER_MSC
			wprintf(L"create: %s\n", sDirName.c_str());
#else
			printf("create: %s\n", sDirName.c_str());
#endif
		}
		if (!MakeDir(sDirName.c_str()))
		{
			m_sExtractStack.pop();
			return false;
		}
		if (current.Entry.Dir.ChildFileOffset != s_nInvalidOffset)
		{
			m_sExtractStack.push(SStackElement());
			SStackElement& childFile = m_sExtractStack.top();
			childFile.IsDir = false;
			childFile.EntryOffset = current.Entry.Dir.ChildFileOffset;
			childFile.Prefix = sPrefix;
			childFile.ExtractState = kExtractStateBegin;
		}
		current.ExtractState = kExtractStateChildFile;
	}
	else if (current.ExtractState == kExtractStateChildFile)
	{
#if _3DSTOOL_COMPILER == COMPILER_MSC
		wstring sPrefix = current.Prefix;
		if (current.Entry.Dir.DirNameSize != 0)
		{
			sPrefix += current.EntryName + L"/";
		}
#else
		string sPrefix = current.Prefix;
		if (current.Entry.Dir.DirNameSize != 0)
		{
			sPrefix += current.EntryName + "/";
		}
#endif
		if (current.Entry.Dir.ChildDirOffset != s_nInvalidOffset)
		{
			m_sExtractStack.push(SStackElement());
			SStackElement& childDir = m_sExtractStack.top();
			childDir.IsDir = true;
			childDir.EntryOffset = current.Entry.Dir.ChildDirOffset;
			childDir.Prefix = sPrefix;
			childDir.ExtractState = kExtractStateBegin;
		}
		current.ExtractState = kExtractStateChildDir;
	}
	else if (current.ExtractState == kExtractStateChildDir)
	{
		if (current.Entry.Dir.SiblingDirOffset != s_nInvalidOffset)
		{
			m_sExtractStack.push(SStackElement());
			SStackElement& siblingDir = m_sExtractStack.top();
			siblingDir.IsDir = true;
			siblingDir.EntryOffset = current.Entry.Dir.SiblingDirOffset;
			siblingDir.Prefix = current.Prefix;
			siblingDir.ExtractState = kExtractStateBegin;
		}
		current.ExtractState = kExtractStateEnd;
	}
	else if (current.ExtractState == kExtractStateEnd)
	{
		m_sExtractStack.pop();
	}
	return true;
}

bool CRomFs::ExtractFileEntry()
{
	bool bResult = true;
	SStackElement& current = m_sExtractStack.top();
	if (current.ExtractState == kExtractStateBegin)
	{
		FFseek(m_fpRomFsBin, m_nLevel3Offset + m_RomFsLevel3Header.Section[kSectionTypeFile].Offset + current.EntryOffset, SEEK_SET);
		fread(&current.Entry.File, sizeof(current.Entry.File), 1, m_fpRomFsBin);
		char16_t* pEntryName = new char16_t[current.Entry.File.FileNameSize / 2 + 1];
		fread(pEntryName, 2, current.Entry.File.FileNameSize / 2, m_fpRomFsBin);
		pEntryName[current.Entry.File.FileNameSize / 2] = 0;
#if _3DSTOOL_COMPILER == COMPILER_MSC
		current.EntryName = FSU16ToW(pEntryName);
		delete[] pEntryName;
		wstring sPath = m_sRomFsDirName + current.Prefix + current.EntryName;
		FILE* fp = _wfopen(sPath.c_str(), L"wb");
		if (fp == nullptr)
		{
			wprintf(L"ERROR: open file %s failed\n\n", sPath.c_str());
			bResult = false;
		}
#else
		current.EntryName = FSU16ToU8(pEntryName);
		delete[] pEntryName;
		string sPath = m_sRomFsDirName + current.Prefix + current.EntryName;
		FILE* fp = fopen(sPath.c_str(), "wb");
		if (fp == nullptr)
		{
			wprintf(L"ERROR: open file %s failed\n\n", sPath.c_str());
			bResult = false;
		}
#endif
		else
		{
			if (m_bVerbose)
			{
#if _3DSTOOL_COMPILER == COMPILER_MSC
				wprintf(L"create: %s\n", sPath.c_str());
#else
				printf("create: %s\n", sPath.c_str());
#endif
			}
			FCopyFile(fp, m_fpRomFsBin, m_nLevel3Offset + m_RomFsLevel3Header.DataOffset + current.Entry.File.FileOffset, current.Entry.File.FileSize);
			fclose(fp);
		}
		if (current.Entry.File.SiblingFileOffset != s_nInvalidOffset)
		{
			m_sExtractStack.push(SStackElement());
			SStackElement& siblingFile = m_sExtractStack.top();
			siblingFile.IsDir = false;
			siblingFile.EntryOffset = current.Entry.File.SiblingFileOffset;
			siblingFile.Prefix = current.Prefix;
			siblingFile.ExtractState = kExtractStateBegin;
		}
		current.ExtractState = kExtractStateEnd;
	}
	else if (current.ExtractState == kExtractStateEnd)
	{
		m_sExtractStack.pop();
	}
	return bResult;
}

bool CRomFs::IsRomFsFile(const char* a_pFileName)
{
	FILE* fp = fopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pFileName);
		return false;
	}
	SRomFsHeader romFsHeader;
	fread(&romFsHeader, sizeof(romFsHeader), 1, fp);
	fclose(fp);
	return romFsHeader.Signature == s_uSignature;
}
