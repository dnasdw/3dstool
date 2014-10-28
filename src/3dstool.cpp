#include "3dstool.h"
#include "ncch.h"
#include "ncsd.h"
#include "romfs.h"

C3DSTool::SOption C3DSTool::s_Option[] =
{
	{ "extract", 'x', "extract the FILE" },
	{ "create", 'c', "create the FILE" },
	{ "crypto", 'e', "decrypt/encrypt the FILE or decrypt/encrypt the part of the cxi file" },
	{ "type", 't', "[[cci|ncsd|3ds]|[cxi|ncch]|romfs]\n\t\tthe type of FILE, optional" },
	{ "file", 'f', "the source of extract or the destination of create FILE, required" },
	{ "header", 0, "the header file of FILE" },
	{ "partition0", '0', "the cxi file of cci file at partition 0" },
	{ "partition1", '1', "the cxi file of cci file at partition 1" },
	{ "partition2", '2', "the cxi file of cci file at partition 2" },
	{ "partition3", '3', "the cxi file of cci file at partition 3" },
	{ "partition4", '4', "the cxi file of cci file at partition 4" },
	{ "partition5", '5', "the cxi file of cci file at partition 5" },
	{ "partition6", '6', "the cxi file of cci file at partition 6" },
	{ "partition7", '7', "the cxi file of cci file at partition 7" },
	{ "xor", 0, "the xor data file for crypto the FILE" },
	{ "exhxor", 0, "short for extendedheaderxor" },
	{ "extendedheaderxor", 0, "the xor data file for crypto the extendedheader of the cxi file" },
	{ "exefsxor", 0, "the xor data file for crypto the exefs part of the cxi file" },
	{ "romfsxor", 0, "the xor data file for crypto the romfs part of the cxi file" },
	{ "exh", 0, "short for extendedheader" },
	{ "extendedheader", 0, "the extendedheader file of the cxi file" },
	{ "ace", 0, "short for accesscontrolextended file of the cxi file" },
	{ "accesscontrolextended", 0, "the accesscontrolextended file of the cxi file" },
	{ "plain", 0, "short for plainregion" },
	{ "plainregion", 0, "the plainregion file of the cxi file" },
	{ "exefs", 0, "the exefs file of the cxi file" },
	{ "romfs", 0, "the romfs file of the cxi file" },
	{ "romfsdir", 0, "the romfs dir for the romfs file" },
	{ "verbose", 'v', "show progress" },
	{ "help", 'h', "show this help" },
	{ nullptr, 0, nullptr }
};

C3DSTool::C3DSTool()
	: m_eAction(kActionNone)
	, m_eFileType(kFileTypeUnknown)
	, m_pFileName(nullptr)
	, m_pHeaderFileName(nullptr)
	, m_pXorFileName(nullptr)
	, m_pExtendedHeaderXorFileName(nullptr)
	, m_pExeFsXorFileName(nullptr)
	, m_pRomFsXorFileName(nullptr)
	, m_pExtendedHeaderFileName(nullptr)
	, m_pAccessControlExtendedFileName(nullptr)
	, m_pPlainRegionFileName(nullptr)
	, m_pExeFsFileName(nullptr)
	, m_pRomFsFileName(nullptr)
	, m_pRomFsDirName(nullptr)
	, m_bVerbose(false)
	, m_pMessage(nullptr)
{
	memset(m_pNcchFileName, 0, sizeof(m_pNcchFileName));
}

C3DSTool::~C3DSTool()
{
}

int C3DSTool::ParseOptions(int a_nArgc, char* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = strlen(a_pArgv[i]);
		int nIndex = i;
		if (a_pArgv[i][0] != '-')
		{
			for (int j = 0; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnUnknownArgument:
					printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
					return 1;
				case kParseOptionReturnActionConflict:
					printf("ERROR: action conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != '-')
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnUnknownArgument:
					printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
					return 1;
				case kParseOptionReturnActionConflict:
					printf("ERROR: action conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == '-')
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				printf("ERROR: illegal option\n\n");
				return 1;
			case kParseOptionReturnNoArgument:
				printf("ERROR: no argument\n\n");
				return 1;
			case kParseOptionReturnUnknownArgument:
				printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
				return 1;
			case kParseOptionReturnActionConflict:
				printf("ERROR: action conflict\n\n");
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int C3DSTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		printf("ERROR: nothing to do\n\n");
		return 1;
	}
	if (m_eAction != kActionHelp && m_pFileName == nullptr)
	{
		printf("ERROR: no --file option\n\n");
		return 1;
	}
	if (m_eAction == kActionExtract)
	{
		if (m_pRomFsDirName == nullptr)
		{
			printf("ERROR: no romfsdir\n\n");
			return 1;
		}
		if (!CRomFs::IsRomFsFile(m_pFileName))
		{
			printf("ERROR: %s is not a romfs file\n\n", m_pFileName);
			return 1;
		}
		else if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeRomfs && m_bVerbose)
		{
			printf("INFO: ignore --type option\n");
		}
	}
	if (m_eAction == kActionCreate)
	{
		printf("ERRO: not implement\n\n");
		return 1;
	}
	if (m_eAction == kActionCrypto)
	{
		if (m_pXorFileName == nullptr && m_pExtendedHeaderXorFileName == nullptr && m_pExeFsXorFileName == nullptr && m_pRomFsXorFileName == nullptr)
		{
			printf("ERROR: no xor data file\n\n");
			return 1;
		}
		if (m_pExtendedHeaderXorFileName != nullptr || m_pExeFsXorFileName != nullptr || m_pRomFsXorFileName != nullptr)
		{
			if (m_pXorFileName != nullptr)
			{
				printf("ERROR: --xor can not with --extendedheaderxor or --exefsxor or --romfsxor\n\n");
				return 1;
			}
			if (!CNcch::IsNcchFile(m_pFileName))
			{
				printf("ERROR: %s is not a ncch file\n\n", m_pFileName);
				return 1;
			}
			else if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCxi && m_bVerbose)
			{
				printf("INFO: ignore --type option\n");
			}
		}
	}
	return 0;
}

int C3DSTool::Help()
{
	printf("3dstool %s by dnasdw\n\n", _3DS_TOOL_VERSION);
	printf("usage: 3dstool [option...] [FILE]...\n");
	printf("example:\n");
	printf("  3dstool -xvtf 3ds input.3ds\n\n");
	printf("option:\n");
	SOption* pOption = s_Option;
	while (pOption->m_pName != nullptr && pOption->m_pDoc != nullptr)
	{
		printf("  ");
		if (pOption->m_nKey != 0)
		{
			printf("-%c,", pOption->m_nKey);
		}
		else
		{
			printf("   ");
		}
		printf(" --%s", pOption->m_pName);
		if (strlen(pOption->m_pName) >= 8)
		{
			printf("\n\t");
		}
		printf("\t%s\n", pOption->m_pDoc);
		pOption++;
	}
	return 0;
}

int C3DSTool::Action()
{
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	if (m_eAction == kActionExtract)
	{
		if (!checkFileType())
		{
			printf("ERROR: %s\n\n", m_pMessage);
			return 1;
		}
		if (!extractFile())
		{
			printf("ERROR: extract file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionCrypto)
	{
		if (!cryptoFile())
		{
			printf("ERROR: crypto file failed\n\n");
			return 1;
		}
	}
	return 0;
}

C3DSTool::EParseOptionReturn C3DSTool::parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[])
{
	if (strcmp(a_pName, "extract") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionExtract;
		}
		else if (m_eAction != kActionHelp)
		{
			return kParseOptionReturnActionConflict;
		}
	}
	else if (strcmp(a_pName, "create") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionHelp)
		{
			return kParseOptionReturnActionConflict;
		}
	}
	else if (strcmp(a_pName, "crypto") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCrypto;
		}
		else if (m_eAction != kActionHelp)
		{
			return kParseOptionReturnActionConflict;
		}
	}
	else if (strcmp(a_pName, "type") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		char* pType = a_pArgv[++a_nIndex];
		if (strcmp(pType, "cci") == 0 || strcmp(pType, "ncsd") == 0 || strcmp(pType, "3ds") == 0)
		{
			m_eFileType = kFileTypeCci;
		}
		else if (strcmp(pType, "cxi") == 0 || strcmp(pType, "ncch") == 0)
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (strcmp(pType, "romfs") == 0)
		{
			m_eFileType = kFileTypeRomfs;
		}
		else
		{
			m_pMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "header") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (FSStartsWith<string>(a_pName, "partition"))
	{
		int nIndex = FSToN32(a_pName + strlen("partition"));
		if (nIndex < 0 || nIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pNcchFileName[nIndex] = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "extendedheaderxor") == 0 || strcmp(a_pName, "exhxor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExtendedHeaderXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefsxor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExeFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfsxor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pRomFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "extendedheader") == 0 || strcmp(a_pName, "exh") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExtendedHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "accesscontrolextended") == 0 || strcmp(a_pName, "ace") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pAccessControlExtendedFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "plainregion") == 0 || strcmp(a_pName, "plain") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pPlainRegionFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExeFsFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pRomFsFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfsdir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pRomFsDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "verbose") == 0)
	{
		m_bVerbose = true;
	}
	else if (strcmp(a_pName, "help") == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

C3DSTool::EParseOptionReturn C3DSTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, char* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->m_pName != nullptr || pOption->m_nKey != 0 || pOption->m_pDoc != nullptr; pOption++)
	{
		if (pOption->m_nKey == a_nKey)
		{
			return parseOptions(pOption->m_pName, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool C3DSTool::checkFileType()
{
	if (m_eFileType == kFileTypeUnknown)
	{
		if (CNcsd::IsNcsdFile(m_pFileName))
		{
			m_eFileType = kFileTypeCci;
		}
		else if (CNcch::IsNcchFile(m_pFileName))
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (CRomFs::IsRomFsFile(m_pFileName))
		{
			m_eFileType = kFileTypeRomfs;
		}
		else
		{
			m_pMessage = "unknown file type";
			return false;
		}
	}
	else
	{
		bool bMatch = false;
		switch (m_eFileType)
		{
		case C3DSTool::kFileTypeCci:
			bMatch = CNcsd::IsNcsdFile(m_pFileName);
			break;
		case C3DSTool::kFileTypeCxi:
			bMatch = CNcch::IsNcchFile(m_pFileName);
			break;
		case C3DSTool::kFileTypeRomfs:
			bMatch = CRomFs::IsRomFsFile(m_pFileName);
			break;
		}
		if (!bMatch)
		{
			m_pMessage = "the file type is mismatch";
			return false;
		}
	}
	return true;
}

bool C3DSTool::extractFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case C3DSTool::kFileTypeCci:
	{
		CNcsd ncsd;
		ncsd.SetFileName(m_pFileName);
		ncsd.SetHeaderFileName(m_pHeaderFileName);
		ncsd.SetNcchFileName(m_pNcchFileName);
		ncsd.SetVerbose(m_bVerbose);
		bResult = ncsd.ExtractFile();
	}
		break;
	case C3DSTool::kFileTypeCxi:
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetHeaderFileName(m_pHeaderFileName);
		ncch.SetExtendedHeaderFileName(m_pExtendedHeaderFileName);
		ncch.SetAccessControlExtendedFileName(m_pAccessControlExtendedFileName);
		ncch.SetPlainRegionFileName(m_pPlainRegionFileName);
		ncch.SetExeFsFileName(m_pExeFsFileName);
		ncch.SetRomFsFileName(m_pRomFsFileName);
		ncch.SetVerbose(m_bVerbose);
		bResult = ncch.ExtractFile();
	}
		break;
	case C3DSTool::kFileTypeRomfs:
	{
		CRomFs romFs;
		romFs.SetFileName(m_pFileName);
		romFs.SetRomFsDirName(m_pRomFsDirName);
		romFs.SetVerbose(m_bVerbose);
		bResult = romFs.ExtractFile();
	}
		break;
	}
	return bResult;
}

bool C3DSTool::cryptoFile()
{
	bool bResult = false;
	if (m_pXorFileName != nullptr)
	{
		bResult = FCryptoFile(m_pFileName, m_pXorFileName, 0, 0, true, 0, m_bVerbose);
	}
	else
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetExtendedHeaderXorFileName(m_pExtendedHeaderXorFileName);
		ncch.SetExeFsXorFileName(m_pExeFsXorFileName);
		ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
		ncch.SetVerbose(m_bVerbose);
		bResult = ncch.CryptoFile();
	}
	return bResult;
}

int main(int argc, char* argv[])
{
	FSetLocale();
	C3DSTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
	}
	return tool.Action();
}
