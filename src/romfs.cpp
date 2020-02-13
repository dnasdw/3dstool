#include "romfs.h"
#include "space.h"
#include <openssl/sha.h>

bool RemapIgnoreLevelCompare(const CRomFs::SCommonFileEntry* lhs, const CRomFs::SCommonFileEntry* rhs)
{
	return lhs->RemapIgnoreLevel < rhs->RemapIgnoreLevel;
}

const u32 CRomFs::s_uSignature = SDW_CONVERT_ENDIAN32('IVFC');
const int CRomFs::s_nBlockSizePower = 0xC;
const int CRomFs::s_nBlockSize = 1 << s_nBlockSizePower;
const int CRomFs::s_nSHA256BlockSize = 0x20;
const n32 CRomFs::s_nInvalidOffset = -1;
const int CRomFs::s_nEntryNameAlignment = 4;
const n64 CRomFs::s_nFileSizeAlignment = 0x10;

CRomFs::CRomFs()
	: m_bVerbose(false)
	, m_fpRomFs(nullptr)
	, m_nLevel3Offset(s_nBlockSize)
	, m_bRemapped(false)
{
	memset(&m_RomFsHeader, 0, sizeof(m_RomFsHeader));
	memset(&m_RomFsMetaInfo, 0, sizeof(m_RomFsMetaInfo));
}

CRomFs::~CRomFs()
{
}

void CRomFs::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CRomFs::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CRomFs::SetRomFsDirName(const UString& a_sRomFsDirName)
{
	m_sRomFsDirName = a_sRomFsDirName;
}

void CRomFs::SetRomFsFileName(const UString& a_sRomFsFileName)
{
	m_sRomFsFileName = a_sRomFsFileName;
}

bool CRomFs::ExtractFile()
{
	bool bResult = true;
	m_fpRomFs = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (m_fpRomFs == nullptr)
	{
		return false;
	}
	fread(&m_RomFsHeader, sizeof(m_RomFsHeader), 1, m_fpRomFs);
	m_nLevel3Offset = Align(Align(m_RomFsHeader.Size, s_nSHA256BlockSize) + m_RomFsHeader.Level0Size, s_nBlockSize);
	Fseek(m_fpRomFs, m_nLevel3Offset, SEEK_SET);
	fread(&m_RomFsMetaInfo, sizeof(m_RomFsMetaInfo), 1, m_fpRomFs);
	pushExtractStackElement(true, 0, USTR("/"));
	while (!m_sExtractStack.empty())
	{
		SExtractStackElement& current = m_sExtractStack.top();
		if (current.IsDir)
		{
			if (!extractDirEntry())
			{
				bResult = false;
			}
		}
		else if (!extractFileEntry())
		{
			bResult = false;
		}
	}
	fclose(m_fpRomFs);
	return bResult;
}

bool CRomFs::CreateFile()
{
	bool bResult = true;
	setupCreate();
	buildIgnoreList();
	pushDirEntry(USTR(""), 0);
	pushCreateStackElement(0);
	while (!m_sCreateStack.empty())
	{
		if (!createEntryList())
		{
			bResult = false;
		}
	}
	removeEmptyDirEntry();
	createHash();
	redirectOffset();
	createMetaInfo();
	remap();
	createHeader();
	initLevelBuffer();
	n64 nFileSize = Align(m_LevelBuffer[2].FilePos + m_RomFsHeader.Level2.Size, s_nBlockSize);
	m_fpRomFs = UFopen(m_sFileName.c_str(), USTR("wb"));
	if (m_fpRomFs == nullptr)
	{
		return false;
	}
	Seek(m_fpRomFs, nFileSize);
	if (!updateLevelBuffer())
	{
		bResult = false;
	}
	fclose(m_fpRomFs);
	return bResult;
}

bool CRomFs::IsRomFsFile(const UString& a_sFileName)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	SRomFsHeader romFsHeader;
	fread(&romFsHeader, sizeof(romFsHeader), 1, fp);
	fclose(fp);
	return romFsHeader.Signature == s_uSignature;
}

void CRomFs::pushExtractStackElement(bool a_bIsDir, n32 a_nEntryOffset, const UString& a_sPrefix)
{
	if (a_nEntryOffset != s_nInvalidOffset)
	{
		m_sExtractStack.push(SExtractStackElement());
		SExtractStackElement& current = m_sExtractStack.top();
		current.IsDir = a_bIsDir;
		current.EntryOffset = a_nEntryOffset;
		current.Prefix = a_sPrefix;
		current.ExtractState = kExtractStateBegin;
	}
}

bool CRomFs::extractDirEntry()
{
	SExtractStackElement& current = m_sExtractStack.top();
	if (current.ExtractState == kExtractStateBegin)
	{
		readEntry(current);
		UString sPrefix = current.Prefix;
		UString sDirName = m_sRomFsDirName + sPrefix;
		if (current.Entry.Dir.NameSize != 0)
		{
			sPrefix += current.EntryName + USTR("/");
			sDirName += current.EntryName;
		}
		else
		{
			sDirName.erase(sDirName.end() - 1);
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("save: %") PRIUS USTR("\n"), sDirName.c_str());
		}
		if (!UMakeDir(sDirName.c_str()))
		{
			m_sExtractStack.pop();
			return false;
		}
		pushExtractStackElement(false, current.Entry.Dir.ChildFileOffset, sPrefix);
		current.ExtractState = kExtractStateChildDir;
	}
	else if (current.ExtractState == kExtractStateChildDir)
	{
		UString sPrefix = current.Prefix;
		if (current.Entry.Dir.NameSize != 0)
		{
			sPrefix += current.EntryName + USTR("/");
		}
		pushExtractStackElement(true, current.Entry.Dir.ChildDirOffset, sPrefix);
		current.ExtractState = kExtractStateSiblingDir;
	}
	else if (current.ExtractState == kExtractStateSiblingDir)
	{
		pushExtractStackElement(true, current.Entry.Dir.SiblingDirOffset, current.Prefix);
		current.ExtractState = kExtractStateEnd;
	}
	else if (current.ExtractState == kExtractStateEnd)
	{
		m_sExtractStack.pop();
	}
	return true;
}

bool CRomFs::extractFileEntry()
{
	bool bResult = true;
	SExtractStackElement& current = m_sExtractStack.top();
	if (current.ExtractState == kExtractStateBegin)
	{
		readEntry(current);
		UString sPath = m_sRomFsDirName + current.Prefix + current.EntryName;
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
			CopyFile(fp, m_fpRomFs, m_nLevel3Offset + m_RomFsMetaInfo.DataOffset + current.Entry.File.FileOffset, current.Entry.File.FileSize);
			fclose(fp);
		}
		pushExtractStackElement(false, current.Entry.File.SiblingFileOffset, current.Prefix);
		current.ExtractState = kExtractStateEnd;
	}
	else if (current.ExtractState == kExtractStateEnd)
	{
		m_sExtractStack.pop();
	}
	return bResult;
}

void CRomFs::readEntry(SExtractStackElement& a_Element)
{
	if (a_Element.IsDir)
	{
		Fseek(m_fpRomFs, m_nLevel3Offset + m_RomFsMetaInfo.Section[kSectionTypeDir].Offset + a_Element.EntryOffset, SEEK_SET);
		fread(&a_Element.Entry.Dir, sizeof(a_Element.Entry.Dir), 1, m_fpRomFs);
		Char16_t* pEntryName = new Char16_t[a_Element.Entry.Dir.NameSize / 2 + 1];
		fread(pEntryName, 2, a_Element.Entry.Dir.NameSize / 2, m_fpRomFs);
		pEntryName[a_Element.Entry.Dir.NameSize / 2] = 0;
		a_Element.EntryName = U16ToU(pEntryName);
		delete[] pEntryName;
	}
	else
	{
		Fseek(m_fpRomFs, m_nLevel3Offset + m_RomFsMetaInfo.Section[kSectionTypeFile].Offset + a_Element.EntryOffset, SEEK_SET);
		fread(&a_Element.Entry.File, sizeof(a_Element.Entry.File), 1, m_fpRomFs);
		Char16_t* pEntryName = new Char16_t[a_Element.Entry.File.NameSize / 2 + 1];
		fread(pEntryName, 2, a_Element.Entry.File.NameSize / 2, m_fpRomFs);
		pEntryName[a_Element.Entry.File.NameSize / 2] = 0;
		a_Element.EntryName = U16ToU(pEntryName);
		delete[] pEntryName;
	}
}

void CRomFs::setupCreate()
{
	m_RomFsHeader.Signature = s_uSignature;
	m_RomFsHeader.Id = 0x10000;
	m_RomFsHeader.Level1.BlockSize = s_nBlockSizePower;
	m_RomFsHeader.Level2.BlockSize = s_nBlockSizePower;
	m_RomFsHeader.Level3.BlockSize = s_nBlockSizePower;
	m_RomFsHeader.Size = sizeof(m_RomFsHeader);
	m_RomFsMetaInfo.Size = sizeof(m_RomFsMetaInfo);
	m_RomFsMetaInfo.Section[kSectionTypeDirHash].Offset = static_cast<u32>(Align(m_RomFsMetaInfo.Size, s_nEntryNameAlignment));
}

void CRomFs::buildIgnoreList()
{
	m_vIgnoreList.clear();
	m_vRemapIgnoreList.clear();
	UString sIgnorePath = UGetModuleDirName() + USTR("/ignore_3dstool.txt");
	FILE* fp = UFopen(sIgnorePath.c_str(), USTR("rb"));
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
		vector<URegex>* pIgnoreList = &m_vIgnoreList;
		for (vector<string>::const_iterator it = vTxt.begin(); it != vTxt.end(); ++it)
		{
			sTxt = Trim(*it);
			if (!sTxt.empty())
			{
				if (StartWith(sTxt, "//"))
				{
					vector<string> vIngoreTag = Split<string>(sTxt.c_str() + strlen("//"), ":");
					if (vIngoreTag.size() == 2 && vIngoreTag[1].empty())
					{
						vIngoreTag[0] = Trim(vIngoreTag[0]);
						if (vIngoreTag[0] == "ignore")
						{
							pIgnoreList = &m_vIgnoreList;
						}
						else if (vIngoreTag[0] == "remap ignore")
						{
							pIgnoreList = &m_vRemapIgnoreList;
						}
					}
				}
				else
				{
					try
					{
						URegex black(AToU(sTxt), regex_constants::ECMAScript | regex_constants::icase);
						pIgnoreList->push_back(black);
					}
					catch (regex_error& e)
					{
						UPrintf(USTR("ERROR: %") PRIUS USTR("\n\n"), AToU(e.what()).c_str());
					}
				}
			}
		}
	}
}

void CRomFs::pushDirEntry(const UString& a_sEntryName, n32 a_nParentDirOffset)
{
	m_vCreateDir.push_back(SEntry());
	SEntry& currentEntry = m_vCreateDir.back();
	if (m_vCreateDir.size() == 1)
	{
		currentEntry.Path = m_sRomFsDirName;
	}
	else
	{
		currentEntry.Path = m_vCreateDir[a_nParentDirOffset].Path + USTR("/") + a_sEntryName;
	}
	currentEntry.EntryName = UToU16(a_sEntryName);
	currentEntry.Entry.Dir.ParentDirOffset = a_nParentDirOffset;
	currentEntry.Entry.Dir.SiblingDirOffset = s_nInvalidOffset;
	currentEntry.Entry.Dir.ChildDirOffset = s_nInvalidOffset;
	currentEntry.Entry.Dir.ChildFileOffset = s_nInvalidOffset;
	currentEntry.Entry.Dir.PrevDirOffset = s_nInvalidOffset;
	currentEntry.Entry.Dir.NameSize = static_cast<n32>(currentEntry.EntryName.size() * 2);
	currentEntry.EntryNameSize = static_cast<int>(Align(currentEntry.Entry.Dir.NameSize, s_nEntryNameAlignment));
	if (m_vCreateDir[a_nParentDirOffset].Entry.Dir.ChildDirOffset != s_nInvalidOffset && m_vCreateDir.size() - 1 != m_vCreateDir[a_nParentDirOffset].Entry.Dir.ChildDirOffset)
	{
		m_vCreateDir[m_vCreateDir.size() - 2].Entry.Dir.SiblingDirOffset = static_cast<n32>(m_vCreateDir.size() - 1);
	}
}

bool CRomFs::pushFileEntry(const UString& a_sEntryName, n32 a_nParentDirOffset)
{
	bool bResult = true;
	m_vCreateFile.push_back(SEntry());
	SEntry& currentEntry = m_vCreateFile.back();
	currentEntry.Path = m_vCreateDir[a_nParentDirOffset].Path + USTR("/") + a_sEntryName;
	currentEntry.EntryName = UToU16(a_sEntryName);
	currentEntry.EntryOffset = static_cast<n32>(Align(m_RomFsMetaInfo.Section[kSectionTypeFile].Size, s_nEntryNameAlignment));
	currentEntry.Entry.File.ParentDirOffset = a_nParentDirOffset;
	currentEntry.Entry.File.SiblingFileOffset = s_nInvalidOffset;
	currentEntry.Entry.File.FileOffset = Align(m_RomFsHeader.Level3.Size, s_nFileSizeAlignment);
	if (!UGetFileSize(currentEntry.Path.c_str(), currentEntry.Entry.File.FileSize))
	{
		bResult = false;
		UPrintf(USTR("ERROR: %") PRIUS USTR(" stat error\n\n"), currentEntry.Path.c_str());
	}
	currentEntry.Entry.File.PrevFileOffset = s_nInvalidOffset;
	currentEntry.Entry.File.NameSize = static_cast<n32>(currentEntry.EntryName.size() * 2);
	currentEntry.EntryNameSize = static_cast<int>(Align(currentEntry.Entry.File.NameSize, s_nEntryNameAlignment));
	if (m_vCreateDir[a_nParentDirOffset].Entry.Dir.ChildFileOffset != s_nInvalidOffset && m_vCreateFile.size() - 1 != m_vCreateDir[a_nParentDirOffset].Entry.Dir.ChildFileOffset)
	{
		m_vCreateFile[m_vCreateFile.size() - 2].Entry.File.SiblingFileOffset = static_cast<n32>(m_vCreateFile.size() - 1);
	}
	m_RomFsMetaInfo.Section[kSectionTypeFile].Size = currentEntry.EntryOffset + sizeof(currentEntry.Entry.File) + currentEntry.EntryNameSize;
	m_RomFsHeader.Level3.Size = currentEntry.Entry.File.FileOffset + currentEntry.Entry.File.FileSize;
	return bResult;
}

void CRomFs::pushCreateStackElement(int a_nEntryOffset)
{
	m_sCreateStack.push(SCreateStackElement());
	SCreateStackElement& current = m_sCreateStack.top();
	current.EntryOffset = a_nEntryOffset;
	current.ChildIndex = -1;
}

bool CRomFs::createEntryList()
{
	bool bResult = true;
	SCreateStackElement& current = m_sCreateStack.top();
	if (current.ChildIndex == -1)
	{
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
		WIN32_FIND_DATAW ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		wstring sPattern = m_vCreateDir[current.EntryOffset].Path + L"/*";
		hFind = FindFirstFileW(sPattern.c_str(), &ffd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (matchInIgnoreList(m_vCreateDir[current.EntryOffset].Path.substr(m_sRomFsDirName.size()) + L"/" + ffd.cFileName))
				{
					continue;
				}
				if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					if (m_vCreateDir[current.EntryOffset].Entry.Dir.ChildFileOffset == s_nInvalidOffset)
					{
						m_vCreateDir[current.EntryOffset].Entry.Dir.ChildFileOffset = static_cast<n32>(m_vCreateFile.size());
					}
					if (!pushFileEntry(ffd.cFileName, current.EntryOffset))
					{
						bResult = false;
					}
				}
				else if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
				{
					if (m_vCreateDir[current.EntryOffset].Entry.Dir.ChildDirOffset == s_nInvalidOffset)
					{
						m_vCreateDir[current.EntryOffset].Entry.Dir.ChildDirOffset = static_cast<n32>(m_vCreateDir.size());
					}
					current.ChildOffset.push_back(static_cast<int>(m_vCreateDir.size()));
					pushDirEntry(ffd.cFileName, current.EntryOffset);
				}
			} while (FindNextFileW(hFind, &ffd) != 0);
		}
#else
		DIR* pDir = opendir(m_vCreateDir[current.EntryOffset].Path.c_str());
		if (pDir != nullptr)
		{
			map<string, string> mDir;
			map<string, string> mFile;
			dirent* pDirent = nullptr;
			while ((pDirent = readdir(pDir)) != nullptr)
			{
				string sName = pDirent->d_name;
#if SDW_PLATFORM == SDW_PLATFORM_MACOS
				sName = TSToS<string, string>(sName, "UTF-8-MAC", "UTF-8");
#endif
				if (matchInIgnoreList(m_vCreateDir[current.EntryOffset].Path.substr(m_sRomFsDirName.size()) + "/" + sName))
				{
					continue;
				}
				string sNameUpper = sName;
				transform(sNameUpper.begin(), sNameUpper.end(), sNameUpper.begin(), ::toupper);
				// handle cases where d_type is DT_UNKNOWN
				if (pDirent->d_type == DT_UNKNOWN)
				{
					string sPath = m_vCreateDir[current.EntryOffset].Path + "/" + sName;
					Stat st;
					if (UStat(sPath.c_str(), &st) == 0)
					{
						if (S_ISREG(st.st_mode))
						{
							pDirent->d_type = DT_REG;
						}
						else if (S_ISDIR(st.st_mode))
						{
							pDirent->d_type = DT_DIR;
						}
					}
				}
				if (pDirent->d_type == DT_REG)
				{
					mFile.insert(make_pair(sNameUpper, sName));
				}
				else if (pDirent->d_type == DT_DIR && strcmp(pDirent->d_name, ".") != 0 && strcmp(pDirent->d_name, "..") != 0)
				{
					mDir.insert(make_pair(sNameUpper, sName));
				}
			}
			closedir(pDir);
			for (map<string, string>::const_iterator it = mDir.begin(); it != mDir.end(); ++it)
			{
				if (m_vCreateDir[current.EntryOffset].Entry.Dir.ChildDirOffset == s_nInvalidOffset)
				{
					m_vCreateDir[current.EntryOffset].Entry.Dir.ChildDirOffset = static_cast<n32>(m_vCreateDir.size());
				}
				current.ChildOffset.push_back(static_cast<int>(m_vCreateDir.size()));
				pushDirEntry(it->second, current.EntryOffset);
			}
			for (map<string, string>::const_iterator it = mFile.begin(); it != mFile.end(); ++it)
			{
				if (m_vCreateDir[current.EntryOffset].Entry.Dir.ChildFileOffset == s_nInvalidOffset)
				{
					m_vCreateDir[current.EntryOffset].Entry.Dir.ChildFileOffset = static_cast<n32>(m_vCreateFile.size());
				}
				if (!pushFileEntry(it->second, current.EntryOffset))
				{
					bResult = false;
				}
			}
		}
#endif
		current.ChildIndex = 0;
	}
	else if (current.ChildIndex != current.ChildOffset.size())
	{
		pushCreateStackElement(current.ChildOffset[current.ChildIndex++]);
	}
	else
	{
		m_sCreateStack.pop();
	}
	return bResult;
}

bool CRomFs::matchInIgnoreList(const UString& a_sPath) const
{
	bool bMatch = false;
	for (vector<URegex>::const_iterator it = m_vIgnoreList.begin(); it != m_vIgnoreList.end(); ++it)
	{
		if (regex_search(a_sPath, *it))
		{
			bMatch = true;
			break;
		}
	}
	return bMatch;
}

u32 CRomFs::getRemapIgnoreLevel(const UString& a_sPath) const
{
	for (u32 i = 0; i < static_cast<u32>(m_vRemapIgnoreList.size()); i++)
	{
		const URegex& rgx = m_vRemapIgnoreList[i];
		if (regex_search(a_sPath, rgx))
		{
			return i;
		}
	}
	return UINT32_MAX;
}

void CRomFs::removeEmptyDirEntry()
{
	int nEmptyDirIndex;
	do
	{
		nEmptyDirIndex = 0;
		for (int i = static_cast<int>(m_vCreateDir.size()) - 1; i > 0; i--)
		{
			SEntry& currentEntry = m_vCreateDir[i];
			if (currentEntry.Entry.Dir.ChildDirOffset == s_nInvalidOffset && currentEntry.Entry.Dir.ChildFileOffset == s_nInvalidOffset)
			{
				nEmptyDirIndex = i;
				break;
			}
		}
		if (nEmptyDirIndex > 0)
		{
			removeDirEntry(nEmptyDirIndex);
		}
	} while (nEmptyDirIndex > 0);
	for (int i = 0; i < static_cast<int>(m_vCreateDir.size()); i++)
	{
		SEntry& currentEntry = m_vCreateDir[i];
		currentEntry.EntryOffset = static_cast<n32>(Align(m_RomFsMetaInfo.Section[kSectionTypeDir].Size, s_nEntryNameAlignment));
		m_RomFsMetaInfo.Section[kSectionTypeDir].Size = currentEntry.EntryOffset + sizeof(currentEntry.Entry.Dir) + currentEntry.EntryNameSize;
	}
}

void CRomFs::removeDirEntry(int a_nIndex)
{
	SEntry& removedEntry = m_vCreateDir[a_nIndex];
	SEntry& siblingEntry = m_vCreateDir[a_nIndex - 1];
	SEntry& parentEntry = m_vCreateDir[removedEntry.Entry.Dir.ParentDirOffset];
	if (siblingEntry.Entry.Dir.SiblingDirOffset == a_nIndex)
	{
		siblingEntry.Entry.Dir.SiblingDirOffset = removedEntry.Entry.Dir.SiblingDirOffset;
	}
	else if (parentEntry.Entry.Dir.ChildDirOffset == a_nIndex)
	{
		parentEntry.Entry.Dir.ChildDirOffset = removedEntry.Entry.Dir.SiblingDirOffset;
	}
	for (int i = 0; i < static_cast<int>(m_vCreateDir.size()); i++)
	{
		SEntry& currentEntry = m_vCreateDir[i];
		subDirOffset(currentEntry.Entry.Dir.ParentDirOffset, a_nIndex);
		subDirOffset(currentEntry.Entry.Dir.SiblingDirOffset, a_nIndex);
		subDirOffset(currentEntry.Entry.Dir.ChildDirOffset, a_nIndex);
	}
	for (int i = 0; i < static_cast<int>(m_vCreateFile.size()); i++)
	{
		SEntry& currentEntry = m_vCreateFile[i];
		subDirOffset(currentEntry.Entry.File.ParentDirOffset, a_nIndex);
	}
	m_vCreateDir.erase(m_vCreateDir.begin() + a_nIndex);
}

void CRomFs::subDirOffset(n32& a_nOffset, int a_nIndex)
{
	if (a_nOffset > a_nIndex)
	{
		a_nOffset--;
	}
}

void CRomFs::createHash()
{
	m_vDirBucket.resize(computeBucketCount(static_cast<u32>(m_vCreateDir.size())), s_nInvalidOffset);
	m_vFileBucket.resize(computeBucketCount(static_cast<u32>(m_vCreateFile.size())), s_nInvalidOffset);
	for (int i = 0; i < static_cast<int>(m_vCreateDir.size()); i++)
	{
		SEntry& currentEntry = m_vCreateDir[i];
		currentEntry.BucketIndex = hash(m_vCreateDir[currentEntry.Entry.Dir.ParentDirOffset].EntryOffset, currentEntry.EntryName) % m_vDirBucket.size();
		if (m_vDirBucket[currentEntry.BucketIndex] != s_nInvalidOffset)
		{
			currentEntry.Entry.Dir.PrevDirOffset = m_vDirBucket[currentEntry.BucketIndex];
		}
		m_vDirBucket[currentEntry.BucketIndex] = i;
	}
	for (int i = 0; i < static_cast<int>(m_vCreateFile.size()); i++)
	{
		SEntry& currentEntry = m_vCreateFile[i];
		currentEntry.BucketIndex = hash(m_vCreateDir[currentEntry.Entry.File.ParentDirOffset].EntryOffset, currentEntry.EntryName) % m_vFileBucket.size();
		if (m_vFileBucket[currentEntry.BucketIndex] != s_nInvalidOffset)
		{
			currentEntry.Entry.File.PrevFileOffset = m_vFileBucket[currentEntry.BucketIndex];
		}
		m_vFileBucket[currentEntry.BucketIndex] = i;
	}
}

u32 CRomFs::computeBucketCount(u32 a_uEntries)
{
	u32 uBucket = a_uEntries;
	if (uBucket < 3)
	{
		uBucket = 3;
	}
	else if (uBucket <= 19)
	{
		uBucket |= 1;
	}
	else
	{
		while (uBucket % 2 == 0 || uBucket % 3 == 0 || uBucket % 5 == 0 || uBucket % 7 == 0 || uBucket % 11 == 0 || uBucket % 13 == 0 || uBucket % 17 == 0)
		{
			uBucket += 1;
		}
	}
	return uBucket;
}

void CRomFs::redirectOffset()
{
	for (int i = 0; i < static_cast<int>(m_vDirBucket.size()); i++)
	{
		redirectOffset(m_vDirBucket[i], true);
	}
	for (int i = 0; i < static_cast<int>(m_vFileBucket.size()); i++)
	{
		redirectOffset(m_vFileBucket[i], false);
	}
	for (int i = 0; i < static_cast<int>(m_vCreateDir.size()); i++)
	{
		SEntry& currentEntry = m_vCreateDir[i];
		redirectOffset(currentEntry.Entry.Dir.ParentDirOffset, true);
		redirectOffset(currentEntry.Entry.Dir.SiblingDirOffset, true);
		redirectOffset(currentEntry.Entry.Dir.ChildDirOffset, true);
		redirectOffset(currentEntry.Entry.Dir.ChildFileOffset, false);
		redirectOffset(currentEntry.Entry.Dir.PrevDirOffset, true);
	}
	for (int i = 0; i < static_cast<int>(m_vCreateFile.size()); i++)
	{
		SEntry& currentEntry = m_vCreateFile[i];
		redirectOffset(currentEntry.Entry.File.ParentDirOffset, true);
		redirectOffset(currentEntry.Entry.File.SiblingFileOffset, false);
		redirectOffset(currentEntry.Entry.File.PrevFileOffset, false);
	}
}

void CRomFs::redirectOffset(n32& a_nOffset, bool a_bIsDir)
{
	if (a_nOffset != s_nInvalidOffset)
	{
		if (a_bIsDir)
		{
			a_nOffset = m_vCreateDir[a_nOffset].EntryOffset;
		}
		else
		{
			a_nOffset = m_vCreateFile[a_nOffset].EntryOffset;
		}
	}
}

void CRomFs::createMetaInfo()
{
	m_RomFsMetaInfo.Section[kSectionTypeDirHash].Size = static_cast<u32>(m_vDirBucket.size() * 4);
	m_RomFsMetaInfo.Section[kSectionTypeDir].Offset = static_cast<u32>(Align(m_RomFsMetaInfo.Section[kSectionTypeDirHash].Offset + m_RomFsMetaInfo.Section[kSectionTypeDirHash].Size, s_nEntryNameAlignment));
	m_RomFsMetaInfo.Section[kSectionTypeFileHash].Offset = static_cast<u32>(Align(m_RomFsMetaInfo.Section[kSectionTypeDir].Offset + m_RomFsMetaInfo.Section[kSectionTypeDir].Size, s_nEntryNameAlignment));
	m_RomFsMetaInfo.Section[kSectionTypeFileHash].Size = static_cast<u32>(m_vFileBucket.size() * 4);
	m_RomFsMetaInfo.Section[kSectionTypeFile].Offset = static_cast<u32>(Align(m_RomFsMetaInfo.Section[kSectionTypeFileHash].Offset + m_RomFsMetaInfo.Section[kSectionTypeFileHash].Size, s_nEntryNameAlignment));
	m_RomFsMetaInfo.DataOffset = static_cast<u32>(Align(m_RomFsMetaInfo.Section[kSectionTypeFile].Offset + m_RomFsMetaInfo.Section[kSectionTypeFile].Size, s_nFileSizeAlignment));
}

void CRomFs::remap()
{
	if (!m_sRomFsFileName.empty())
	{
		CRomFs romFs;
		romFs.m_sFileName = m_sRomFsFileName;
		romFs.m_sRomFsDirName = m_sRomFsDirName;
		if (!romFs.travelFile())
		{
			return;
		}
		for (vector<SEntry>::const_iterator it = m_vCreateFile.begin(); it != m_vCreateFile.end(); ++it)
		{
			const SEntry& currentEntry = *it;
			m_mTravelInfo[currentEntry.Path] = currentEntry.Entry.File;
			m_mTravelInfo[currentEntry.Path].RemapIgnoreLevel = getRemapIgnoreLevel(currentEntry.Path);
		}
		CSpace space;
		if (m_nLevel3Offset + m_RomFsMetaInfo.DataOffset > romFs.m_nLevel3Offset + romFs.m_RomFsMetaInfo.DataOffset)
		{
			for (map<UString, SCommonFileEntry>::iterator itRomFs = romFs.m_mTravelInfo.begin(); itRomFs != romFs.m_mTravelInfo.end(); ++itRomFs)
			{
				SCommonFileEntry& currentRomFsFileEntry = itRomFs->second;
				n64 nDelta = currentRomFsFileEntry.FileOffset - (m_nLevel3Offset + m_RomFsMetaInfo.DataOffset);
				if (nDelta < 0)
				{
					currentRomFsFileEntry.FileOffset = m_nLevel3Offset + m_RomFsMetaInfo.DataOffset;
					currentRomFsFileEntry.FileSize += nDelta;
					if (currentRomFsFileEntry.FileSize < 0)
					{
						currentRomFsFileEntry.FileSize = 0;
					}
				}
			}
		}
		else if (m_nLevel3Offset + m_RomFsMetaInfo.DataOffset < romFs.m_nLevel3Offset + romFs.m_RomFsMetaInfo.DataOffset)
		{
			space.AddSpace(m_nLevel3Offset + m_RomFsMetaInfo.DataOffset, romFs.m_nLevel3Offset + romFs.m_RomFsMetaInfo.DataOffset - m_nLevel3Offset - m_RomFsMetaInfo.DataOffset);
		}
		m_RomFsHeader.Level3.Size = 0;
		vector<SCommonFileEntry*> vRemapIgnore;
		for (map<UString, SCommonFileEntry>::iterator itRomFs = romFs.m_mTravelInfo.begin(); itRomFs != romFs.m_mTravelInfo.end(); ++itRomFs)
		{
			SCommonFileEntry& currentRomFsFileEntry = itRomFs->second;
			map<UString, SCommonFileEntry>::iterator it = m_mTravelInfo.find(itRomFs->first);
			if (it == m_mTravelInfo.end())
			{
				space.AddSpace(currentRomFsFileEntry.FileOffset, Align(currentRomFsFileEntry.FileSize, s_nFileSizeAlignment));
				currentRomFsFileEntry.FileSize = 0;
			}
			else
			{
				SCommonFileEntry& currentFileEntry = it->second;
				if (Align(currentFileEntry.FileSize, s_nFileSizeAlignment) > Align(currentRomFsFileEntry.FileSize, s_nFileSizeAlignment) || currentFileEntry.RemapIgnoreLevel != UINT32_MAX)
				{
					space.AddSpace(currentRomFsFileEntry.FileOffset, Align(currentRomFsFileEntry.FileSize, s_nFileSizeAlignment));
					currentRomFsFileEntry.FileSize = 0;
					vRemapIgnore.push_back(&currentFileEntry);
				}
				else
				{
					currentFileEntry.FileOffset = currentRomFsFileEntry.FileOffset - m_nLevel3Offset - m_RomFsMetaInfo.DataOffset;
					space.AddSpace(Align(currentRomFsFileEntry.FileOffset + currentFileEntry.FileSize, s_nFileSizeAlignment), Align(currentRomFsFileEntry.FileSize, s_nFileSizeAlignment) - Align(currentFileEntry.FileSize, s_nFileSizeAlignment));
					if (currentFileEntry.FileOffset + currentFileEntry.FileSize > m_RomFsHeader.Level3.Size)
					{
						m_RomFsHeader.Level3.Size = currentFileEntry.FileOffset + currentFileEntry.FileSize;
					}
				}
			}
		}
		if (m_RomFsHeader.Level3.Size == 0)
		{
			space.Clear();
		}
		else
		{
			space.SubSpace(Align(m_nLevel3Offset + m_RomFsMetaInfo.DataOffset + m_RomFsHeader.Level3.Size, s_nFileSizeAlignment), Align(romFs.m_nLevel3Offset + romFs.m_RomFsHeader.Level3.Size, s_nFileSizeAlignment) - Align(m_nLevel3Offset + m_RomFsMetaInfo.DataOffset + m_RomFsHeader.Level3.Size, s_nFileSizeAlignment));
		}
		for (map<UString, SCommonFileEntry>::iterator it = m_mTravelInfo.begin(); it != m_mTravelInfo.end(); ++it)
		{
			SCommonFileEntry& currentFileEntry = it->second;
			map<UString, SCommonFileEntry>::const_iterator itRomFs = romFs.m_mTravelInfo.find(it->first);
			if (itRomFs == romFs.m_mTravelInfo.end())
			{
				vRemapIgnore.push_back(&currentFileEntry);
			}
		}
		stable_sort(vRemapIgnore.begin(), vRemapIgnore.end(), RemapIgnoreLevelCompare);
		for (vector<SCommonFileEntry*>::iterator it = vRemapIgnore.begin(); it != vRemapIgnore.end(); ++it)
		{
			SCommonFileEntry& currentFileEntry = **it;
			n64 nOffset = space.GetSpace(Align(currentFileEntry.FileSize, s_nFileSizeAlignment));
			if (nOffset < 0)
			{
				currentFileEntry.FileOffset = Align(m_RomFsHeader.Level3.Size, s_nFileSizeAlignment);
				m_RomFsHeader.Level3.Size = currentFileEntry.FileOffset + currentFileEntry.FileSize;
			}
			else
			{
				currentFileEntry.FileOffset = nOffset - m_nLevel3Offset - m_RomFsMetaInfo.DataOffset;
				space.SubSpace(nOffset, Align(currentFileEntry.FileSize, s_nFileSizeAlignment));
			}
		}
		for (vector<SEntry>::iterator it = m_vCreateFile.begin(); it != m_vCreateFile.end(); ++it)
		{
			SEntry& currentEntry = *it;
			currentEntry.Entry.File.FileOffset = m_mTravelInfo[currentEntry.Path].FileOffset;
		}
		m_bRemapped = true;
	}
}

bool CRomFs::travelFile()
{
	m_fpRomFs = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (m_fpRomFs == nullptr)
	{
		return false;
	}
	fread(&m_RomFsHeader, sizeof(m_RomFsHeader), 1, m_fpRomFs);
	m_nLevel3Offset = Align(Align(m_RomFsHeader.Size, s_nSHA256BlockSize) + m_RomFsHeader.Level0Size, s_nBlockSize);
	Fseek(m_fpRomFs, m_nLevel3Offset, SEEK_SET);
	fread(&m_RomFsMetaInfo, sizeof(m_RomFsMetaInfo), 1, m_fpRomFs);
	pushExtractStackElement(true, 0, USTR("/"));
	while (!m_sExtractStack.empty())
	{
		SExtractStackElement& current = m_sExtractStack.top();
		if (current.IsDir)
		{
			travelDirEntry();
		}
		else
		{
			travelFileEntry();
		}
	}
	fclose(m_fpRomFs);
	return true;
}

void CRomFs::travelDirEntry()
{
	SExtractStackElement& current = m_sExtractStack.top();
	if (current.ExtractState == kExtractStateBegin)
	{
		readEntry(current);
		UString sPrefix = current.Prefix;
		UString sDirName = m_sRomFsDirName + sPrefix;
		if (current.Entry.Dir.NameSize != 0)
		{
			sPrefix += current.EntryName + USTR("/");
			sDirName += current.EntryName;
		}
		else
		{
			sDirName.erase(sDirName.end() - 1);
		}
		pushExtractStackElement(false, current.Entry.Dir.ChildFileOffset, sPrefix);
		current.ExtractState = kExtractStateChildDir;
	}
	else if (current.ExtractState == kExtractStateChildDir)
	{
		UString sPrefix = current.Prefix;
		if (current.Entry.Dir.NameSize != 0)
		{
			sPrefix += current.EntryName + USTR("/");
		}
		pushExtractStackElement(true, current.Entry.Dir.ChildDirOffset, sPrefix);
		current.ExtractState = kExtractStateSiblingDir;
	}
	else if (current.ExtractState == kExtractStateSiblingDir)
	{
		pushExtractStackElement(true, current.Entry.Dir.SiblingDirOffset, current.Prefix);
		current.ExtractState = kExtractStateEnd;
	}
	else if (current.ExtractState == kExtractStateEnd)
	{
		m_sExtractStack.pop();
	}
}

void CRomFs::travelFileEntry()
{
	SExtractStackElement& current = m_sExtractStack.top();
	if (current.ExtractState == kExtractStateBegin)
	{
		readEntry(current);
		UString sPath = m_sRomFsDirName + current.Prefix + current.EntryName;
		current.Entry.File.FileOffset += m_nLevel3Offset + m_RomFsMetaInfo.DataOffset;
		m_mTravelInfo[sPath] = current.Entry.File;
		pushExtractStackElement(false, current.Entry.File.SiblingFileOffset, current.Prefix);
		current.ExtractState = kExtractStateEnd;
	}
	else if (current.ExtractState == kExtractStateEnd)
	{
		m_sExtractStack.pop();
	}
}

void CRomFs::createHeader()
{
	m_RomFsHeader.Level3.Size += m_RomFsMetaInfo.DataOffset;
	m_RomFsHeader.Level2.Size = Align(m_RomFsHeader.Level3.Size, s_nBlockSize) / s_nBlockSize * s_nSHA256BlockSize;
	m_RomFsHeader.Level1.Size = Align(m_RomFsHeader.Level2.Size, s_nBlockSize) / s_nBlockSize * s_nSHA256BlockSize;
	m_RomFsHeader.Level0Size = static_cast<u32>(Align(m_RomFsHeader.Level1.Size, s_nBlockSize) / s_nBlockSize * s_nSHA256BlockSize);
	m_RomFsHeader.Level2.LogicOffset = Align(m_RomFsHeader.Level1.Size, s_nBlockSize);
	m_RomFsHeader.Level3.LogicOffset = Align(m_RomFsHeader.Level2.LogicOffset + m_RomFsHeader.Level2.Size, s_nBlockSize);
}

void CRomFs::initLevelBuffer()
{
	n64 nFileSize = 0;
	m_LevelBuffer[0].Data.resize(s_nBlockSize, 0);
	m_LevelBuffer[0].DataPos = 0;
	m_LevelBuffer[0].FilePos = nFileSize;
	m_nLevel3Offset = Align(Align(sizeof(m_RomFsHeader), s_nSHA256BlockSize) + m_RomFsHeader.Level0Size, s_nBlockSize);
	nFileSize = m_nLevel3Offset;
	m_LevelBuffer[3].Data.resize(s_nBlockSize, 0);
	m_LevelBuffer[3].DataPos = 0;
	m_LevelBuffer[3].FilePos = nFileSize;
	nFileSize += Align(m_RomFsHeader.Level3.Size, s_nBlockSize);
	m_LevelBuffer[1].Data.resize(s_nBlockSize, 0);
	m_LevelBuffer[1].DataPos = 0;
	m_LevelBuffer[1].FilePos = nFileSize;
	nFileSize += Align(m_RomFsHeader.Level1.Size, s_nBlockSize);
	m_LevelBuffer[2].Data.resize(s_nBlockSize, 0);
	m_LevelBuffer[2].DataPos = 0;
	m_LevelBuffer[2].FilePos = nFileSize;
}

bool CRomFs::updateLevelBuffer()
{
	bool bResult = true;
	writeBuffer(0, &m_RomFsHeader, sizeof(m_RomFsHeader));
	alignBuffer(0, s_nSHA256BlockSize);
	writeBuffer(3, &m_RomFsMetaInfo, sizeof(m_RomFsMetaInfo));
	writeBuffer(3, &*m_vDirBucket.begin(), m_RomFsMetaInfo.Section[kSectionTypeDirHash].Size);
	for (int i = 0; i < static_cast<int>(m_vCreateDir.size()); i++)
	{
		SEntry& currentEntry = m_vCreateDir[i];
		writeBuffer(3, &currentEntry.Entry.Dir, sizeof(currentEntry.Entry.Dir));
		writeBuffer(3, currentEntry.EntryName.c_str(), currentEntry.EntryNameSize);
	}
	writeBuffer(3, &*m_vFileBucket.begin(), m_RomFsMetaInfo.Section[kSectionTypeFileHash].Size);
	for (int i = 0; i < static_cast<int>(m_vCreateFile.size()); i++)
	{
		SEntry& currentEntry = m_vCreateFile[i];
		writeBuffer(3, &currentEntry.Entry.File, sizeof(currentEntry.Entry.File));
		writeBuffer(3, currentEntry.EntryName.c_str(), currentEntry.EntryNameSize);
	}
	if (!m_bRemapped)
	{
		for (int i = 0; i < static_cast<int>(m_vCreateFile.size()); i++)
		{
			alignBuffer(3, s_nFileSizeAlignment);
			SEntry& currentEntry = m_vCreateFile[i];
			if (!writeBufferFromFile(3, currentEntry.Path, currentEntry.Entry.File.FileSize))
			{
				bResult = false;
			}
		}
	}
	else
	{
		map<n64, SEntry*> mCreateFile;
		for (int i = 0; i < static_cast<int>(m_vCreateFile.size()); i++)
		{
			if (m_vCreateFile[i].Entry.File.FileSize != 0)
			{
				mCreateFile.insert(make_pair(m_vCreateFile[i].Entry.File.FileOffset, &m_vCreateFile[i]));
			}
		}
		for (map<n64, SEntry*>::const_iterator it = mCreateFile.begin(); it != mCreateFile.end(); ++it)
		{
			const SEntry& currentEntry = *it->second;
			writeBuffer(3, nullptr, m_nLevel3Offset + m_RomFsMetaInfo.DataOffset + currentEntry.Entry.File.FileOffset - (m_LevelBuffer[3].FilePos + m_LevelBuffer[3].DataPos));
			if (!writeBufferFromFile(3, currentEntry.Path, currentEntry.Entry.File.FileSize))
			{
				bResult = false;
			}
		}
	}
	alignBuffer(3, s_nBlockSize);
	alignBuffer(2, s_nBlockSize);
	alignBuffer(1, s_nBlockSize);
	alignBuffer(0, s_nBlockSize);
	return bResult;
}

void CRomFs::writeBuffer(int a_nLevel, const void* a_pSrc, n64 a_nSize)
{
	const u8* pSrc = static_cast<const u8*>(a_pSrc);
	do
	{
		n64 nRemainSize = s_nBlockSize - m_LevelBuffer[a_nLevel].DataPos;
		n64 nSize = a_nSize > nRemainSize ? nRemainSize : a_nSize;
		if (nSize > 0)
		{
			if (a_pSrc != nullptr)
			{
				memcpy(&*(m_LevelBuffer[a_nLevel].Data.begin() + m_LevelBuffer[a_nLevel].DataPos), pSrc, static_cast<size_t>(nSize));
				pSrc += nSize;
			}
			m_LevelBuffer[a_nLevel].DataPos += static_cast<int>(nSize);
		}
		if (m_LevelBuffer[a_nLevel].DataPos == s_nBlockSize)
		{
			if (a_nLevel != 0)
			{
				writeBuffer(a_nLevel - 1, SHA256(&*m_LevelBuffer[a_nLevel].Data.begin(), s_nBlockSize, nullptr), s_nSHA256BlockSize);
			}
			Fseek(m_fpRomFs, m_LevelBuffer[a_nLevel].FilePos, SEEK_SET);
			fwrite(&*m_LevelBuffer[a_nLevel].Data.begin(), 1, s_nBlockSize, m_fpRomFs);
			memset(&*m_LevelBuffer[a_nLevel].Data.begin(), 0, s_nBlockSize);
			m_LevelBuffer[a_nLevel].DataPos = 0;
			m_LevelBuffer[a_nLevel].FilePos += s_nBlockSize;
		}
		a_nSize -= nSize;
	} while (a_nSize > 0);
}

bool CRomFs::writeBufferFromFile(int a_nLevel, const UString& a_sPath, n64 a_nSize)
{
	FILE* fp = UFopen(a_sPath.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("load: %") PRIUS USTR("\n"), a_sPath.c_str());
	}
	const n64 nBufferSize = 0x100000;
	static u8 uBuffer[nBufferSize];
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(uBuffer, 1, static_cast<size_t>(nSize), fp);
		writeBuffer(a_nLevel, uBuffer, nSize);
		a_nSize -= nSize;
	}
	fclose(fp);
	return true;
}

void CRomFs::alignBuffer(int a_nLevel, int a_nAlignment)
{
	m_LevelBuffer[a_nLevel].DataPos = static_cast<int>(Align(m_LevelBuffer[a_nLevel].DataPos, a_nAlignment));
	writeBuffer(a_nLevel, nullptr, 0);
}

u32 CRomFs::hash(n32 a_nParentOffset, U16String& a_sEntryName)
{
	u32 uHash = a_nParentOffset ^ 123456789;
	for (int i = 0; i < static_cast<int>(a_sEntryName.size()); i++)
	{
		uHash = ((uHash >> 5) | (uHash << 27)) ^ a_sEntryName[i];
	}
	return uHash;
}
