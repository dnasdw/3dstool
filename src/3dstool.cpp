#include "3dstool.h"
#include "3dscrypt.h"
#include "backwardlz77.h"
#include "banner.h"
#include "exefs.h"
#include "huffman.h"
#include "lz77.h"
#include "ncch.h"
#include "ncsd.h"
#include "patch.h"
#include "romfs.h"
#include "runlength.h"
#include "yaz0.h"

C3dsTool::SOption C3dsTool::s_Option[] =
{
	{ nullptr, 0, USTR("action:") },
	{ USTR("extract"), USTR('x'), USTR("extract the target file") },
	{ USTR("create"), USTR('c'), USTR("create the target file") },
	{ USTR("encrypt"), USTR('e'), USTR("encrypt the target file") },
	{ USTR("uncompress"), USTR('u'), USTR("uncompress the target file") },
	{ USTR("compress"), USTR('z'), USTR("compress the target file") },
	{ USTR("trim"), USTR('r'), USTR("trim the cci file") },
	{ USTR("pad"), USTR('p'), USTR("pad the cci file") },
	{ USTR("diff"), 0, USTR("create the patch file from the old file and the new file") },
	{ USTR("patch"), 0, USTR("apply the patch file to the target file") },
	{ USTR("sample"), 0, USTR("show the samples") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, USTR("\ncommon:") },
	{ USTR("type"), USTR('t'), USTR("[[card|cci|3ds]|[exec|cxi]|[data|cfa]|exefs|romfs|banner]\n\t\tthe type of the file, optional") },
	{ USTR("file"), USTR('f'), USTR("the target file, required") },
	{ USTR("verbose"), USTR('v'), USTR("show the info") },
	{ nullptr, 0, USTR(" extract/create:") },
	{ nullptr, 0, USTR("  cci/cxi/cfa/exefs:") },
	{ USTR("header"), 0, USTR("the header file of the target file") },
	{ nullptr, 0, USTR(" encrypt:") },
	{ USTR("key0"), 0, USTR("short for --key 00000000000000000000000000000000") },
	{ USTR("key"), 0, USTR("the hex string of the key used by the AES-CTR encryption") },
	{ USTR("counter"), 0, USTR("the hex string of the counter used by the AES-CTR encryption") },
	{ USTR("xor"), 0, USTR("the xor data file used by the xor encryption") },
	{ nullptr, 0, USTR(" compress:") },
	{ USTR("compress-align"), 0, USTR("[1|4|8|16|32]\n\t\tthe alignment of the compressed filesize, optional") },
	{ nullptr, 0, USTR("  uncompress:") },
	{ USTR("compress-type"), 0, USTR("[blz|lz(ex)|h4|h8|rl|yaz0]\n\t\tthe type of the compress") },
	{ USTR("compress-out"), 0, USTR("the output file of uncompressed or compressed") },
	{ nullptr, 0, USTR("  yaz0:") },
	{ USTR("yaz0-align"), 0, USTR("[0|128]\n\t\tthe alignment property of the yaz0 compressed file, optional") },
	{ nullptr, 0, USTR(" diff:") },
	{ USTR("old"), 0, USTR("the old file") },
	{ USTR("new"), 0, USTR("the new file") },
	{ nullptr, 0, USTR("  patch:") },
	{ USTR("patch-file"), 0, USTR("the patch file") },
	{ nullptr, 0, USTR("\ncci:") },
	{ nullptr, 0, USTR(" create:") },
	{ USTR("not-pad"), 0, USTR("do not add the pad data") },
	{ nullptr, 0, USTR("  extract:") },
	{ USTR("partition0"), USTR('0'), USTR("the cxi file of the cci file at partition 0") },
	{ USTR("partition1"), USTR('1'), USTR("the cfa file of the cci file at partition 1") },
	{ USTR("partition2"), USTR('2'), USTR("the cfa file of the cci file at partition 2") },
	{ USTR("partition3"), USTR('3'), USTR("the cfa file of the cci file at partition 3") },
	{ USTR("partition4"), USTR('4'), USTR("the cfa file of the cci file at partition 4") },
	{ USTR("partition5"), USTR('5'), USTR("the cfa file of the cci file at partition 5") },
	{ USTR("partition6"), USTR('6'), USTR("the cfa file of the cci file at partition 6") },
	{ USTR("partition7"), USTR('7'), USTR("the cfa file of the cci file at partition 7") },
	{ nullptr, 0, USTR(" trim:") },
	{ USTR("trim-after-partition"), 0, USTR("[0~7], the index of the last reserve partition, optional") },
	{ nullptr, 0, USTR("\ncxi:") },
	{ nullptr, 0, USTR(" create:") },
	{ USTR("not-update-exh-hash"), 0, nullptr },
	{ USTR("not-update-extendedheader-hash"), 0, USTR("do not update the extendedheader hash") },
	{ nullptr, 0, USTR("  extract:") },
	{ USTR("exh"), 0, nullptr },
	{ USTR("extendedheader"), 0, USTR("the extendedheader file of the cxi file") },
	{ USTR("logo"), 0, nullptr },
	{ USTR("logoregion"), 0, USTR("the logoregion file of the cxi file") },
	{ USTR("plain"), 0, nullptr },
	{ USTR("plainregion"), 0, USTR("the plainregion file of the cxi file") },
	{ nullptr, 0, USTR("   encrypt:") },
	{ USTR("exh-xor"), 0, nullptr },
	{ USTR("extendedheader-xor"), 0, USTR("the xor data file used by encrypt the extendedheader of the cxi file") },
	{ USTR("exefs-top-xor"), 0, USTR("the xor data file used by encrypt the top section of the exefs of the cxi file") },
	{ USTR("exefs-top-auto-key"), 0, USTR("use the known key to encrypt the top section of the exefs of the cxi file") },
	{ nullptr, 0, USTR(" cfa:") },
	{ nullptr, 0, USTR("  create:") },
	{ USTR("not-update-exefs-hash"), 0, USTR("do not update the exefs super block hash") },
	{ USTR("not-update-romfs-hash"), 0, USTR("do not update the romfs super block hash") },
	{ nullptr, 0, USTR("   extract:") },
	{ USTR("exefs"), 0, USTR("the exefs file of the cxi/cfa file") },
	{ USTR("romfs"), 0, USTR("the romfs file of the cxi/cfa file") },
	{ nullptr, 0, USTR("    encrypt:") },
	{ USTR("exefs-xor"), 0, USTR("the xor data file used by encrypt the exefs of the cxi/cfa file") },
	{ USTR("romfs-xor"), 0, USTR("the xor data file used by encrypt the romfs of the cxi/cfa file") },
	{ USTR("romfs-auto-key"), 0, USTR("use the known key to encrypt the romfs of the cxi/cfa file") },
	{ nullptr, 0, USTR("\nexefs:") },
	{ nullptr, 0, USTR(" extract/create:") },
	{ USTR("exefs-dir"), 0, USTR("the exefs dir for the exefs file") },
	{ nullptr, 0, USTR("\nromfs:") },
	{ nullptr, 0, USTR(" extract/create:") },
	{ USTR("romfs-dir"), 0, USTR("the romfs dir for the romfs file") },
	{ nullptr, 0, USTR("\nbanner:") },
	{ nullptr, 0, USTR(" extract/create:") },
	{ USTR("banner-dir"), 0, USTR("the banner dir for the banner file") },
	{ nullptr, 0, nullptr }
};

C3dsTool::C3dsTool()
	: m_eAction(kActionNone)
	, m_eFileType(kFileTypeUnknown)
	, m_bVerbose(false)
	, m_nEncryptMode(CNcch::kEncryptModeNone)
	, m_nCompressAlign(1)
	, m_eCompressType(kCompressTypeNone)
	, m_nYaz0Align(0)
	, m_bNotPad(false)
	, m_nLastPartitionIndex(7)
	, m_bNotUpdateExtendedHeaderHash(false)
	, m_bNotUpdateExeFsHash(false)
	, m_bNotUpdateRomFsHash(false)
	, m_bExeFsTopAutoKey(false)
	, m_bRomFsAutoKey(false)
	, m_bCounterValid(false)
	, m_bUncompress(false)
	, m_bCompress(false)
{
}

C3dsTool::~C3dsTool()
{
}

int C3dsTool::ParseOptions(int a_nArgc, UChar* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(UCslen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != USTR('-'))
		{
			UPrintf(USTR("ERROR: illegal option\n\n"));
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != USTR('-'))
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					UPrintf(USTR("ERROR: illegal option\n\n"));
					return 1;
				case kParseOptionReturnNoArgument:
					UPrintf(USTR("ERROR: no argument\n\n"));
					return 1;
				case kParseOptionReturnUnknownArgument:
					UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
					return 1;
				case kParseOptionReturnOptionConflict:
					UPrintf(USTR("ERROR: option conflict\n\n"));
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == USTR('-'))
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				UPrintf(USTR("ERROR: illegal option\n\n"));
				return 1;
			case kParseOptionReturnNoArgument:
				UPrintf(USTR("ERROR: no argument\n\n"));
				return 1;
			case kParseOptionReturnUnknownArgument:
				UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
				return 1;
			case kParseOptionReturnOptionConflict:
				UPrintf(USTR("ERROR: option conflict\n\n"));
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int C3dsTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction != kActionDiff && m_eAction != kActionSample && m_eAction != kActionHelp && m_sFileName.empty())
	{
		UPrintf(USTR("ERROR: no --file option\n\n"));
		return 1;
	}
	if (m_eAction == kActionExtract)
	{
		if (!checkFileType())
		{
			UPrintf(USTR("ERROR: %") PRIUS USTR("\n\n"), m_sMessage.c_str());
			return 1;
		}
		switch (m_eFileType)
		{
		case kFileTypeCci:
			if (m_sHeaderFileName.empty())
			{
				bool bEmpty = true;
				for (int i = 0; i < 8; i++)
				{
					if (!m_mNcchFileName[i].empty())
					{
						bEmpty = false;
						break;
					}
				}
				if (bEmpty)
				{
					UPrintf(USTR("ERROR: nothing to be extract\n\n"));
					return 1;
				}
			}
			break;
		case kFileTypeCxi:
			if (m_sHeaderFileName.empty() && m_sExtendedHeaderFileName.empty() && m_sLogoRegionFileName.empty() && m_sPlainRegionFileName.empty() && m_sExeFsFileName.empty() && m_sRomFsFileName.empty())
			{
				UPrintf(USTR("ERROR: nothing to be extract\n\n"));
				return 1;
			}
			break;
		case kFileTypeCfa:
			if (m_sHeaderFileName.empty() && m_sRomFsFileName.empty())
			{
				UPrintf(USTR("ERROR: nothing to be extract\n\n"));
				return 1;
			}
			break;
		case kFileTypeExeFs:
			if (m_sHeaderFileName.empty() && m_sExeFsDirName.empty())
			{
				UPrintf(USTR("ERROR: nothing to be extract\n\n"));
				return 1;
			}
			break;
		case kFileTypeRomFs:
			if (m_sRomFsDirName.empty())
			{
				UPrintf(USTR("ERROR: no --romfs-dir option\n\n"));
				return 1;
			}
			break;
		case kFileTypeBanner:
			if (m_sBannerDirName.empty())
			{
				UPrintf(USTR("ERROR: no --banner-dir option\n\n"));
				return 1;
			}
			break;
		default:
			break;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (m_eFileType == kFileTypeUnknown)
		{
			UPrintf(USTR("ERROR: no --type option\n\n"));
			return 1;
		}
		else
		{
			if (m_eFileType == kFileTypeCci || m_eFileType == kFileTypeCxi || m_eFileType == kFileTypeCfa || m_eFileType == kFileTypeExeFs)
			{
				if (m_sHeaderFileName.empty())
				{
					UPrintf(USTR("ERROR: no --header option\n\n"));
					return 1;
				}
			}
			if (m_eFileType == kFileTypeCci)
			{
				if (m_mNcchFileName[0].empty())
				{
					UPrintf(USTR("ERROR: no --partition0 option\n\n"));
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeCxi)
			{
				if (m_sExtendedHeaderFileName.empty() || m_sExeFsFileName.empty())
				{
					UPrintf(USTR("ERROR: no --extendedheader or --exefs option\n\n"));
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeCfa)
			{
				if (m_sRomFsFileName.empty())
				{
					UPrintf(USTR("ERROR: no --romfs option\n\n"));
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeExeFs)
			{
				if (m_sExeFsDirName.empty())
				{
					UPrintf(USTR("ERROR: no --exefs-dir option\n\n"));
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeRomFs)
			{
				if (m_sRomFsDirName.empty())
				{
					UPrintf(USTR("ERROR: no --romfs-dir option\n\n"));
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeBanner)
			{
				if (m_sBannerDirName.empty())
				{
					UPrintf(USTR("ERROR: no --banner-dir option\n\n"));
					return 1;
				}
			}
		}
	}
	if (m_eAction == kActionEncrypt)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			UPrintf(USTR("ERROR: no key or xor data file\n\n"));
			return 1;
		}
		else if (m_nEncryptMode == CNcch::kEncryptModeXor)
		{
			if (!m_sExtendedHeaderXorFileName.empty() || !m_sExeFsXorFileName.empty() || !m_sRomFsXorFileName.empty())
			{
				if (!m_sXorFileName.empty())
				{
					UPrintf(USTR("ERROR: --xor can not with --extendedheader-xor or --exefs-xor or --romfs-xor\n\n"));
					return 1;
				}
				if (CNcch::IsCxiFile(m_sFileName))
				{
					if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCxi && m_bVerbose)
					{
						UPrintf(USTR("INFO: ignore --type option\n"));
					}
				}
				else if (CNcch::IsCfaFile(m_sFileName))
				{
					if (m_sRomFsXorFileName.empty())
					{
						UPrintf(USTR("ERROR: no --romfs-xor option\n\n"));
						return 1;
					}
					if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCfa && m_bVerbose)
					{
						UPrintf(USTR("INFO: ignore --type option\n"));
					}
				}
				else
				{
					UPrintf(USTR("ERROR: %") PRIUS USTR(" is not a ncch file\n\n"), m_sFileName.c_str());
					return 1;
				}
			}
		}
	}
	if (m_eAction == kActionUncompress || m_eAction == kActionCompress)
	{
		if (m_eCompressType == kCompressTypeNone)
		{
			UPrintf(USTR("ERROR: no --compress-type option\n\n"));
			return 1;
		}
		if (m_sCompressOutFileName.empty())
		{
			m_sCompressOutFileName = m_sFileName;
		}
	}
	if (m_eAction == kActionTrim || m_eAction == kActionPad)
	{
		if (!CNcsd::IsNcsdFile(m_sFileName))
		{
			UPrintf(USTR("ERROR: %") PRIUS USTR(" is not a ncsd file\n\n"), m_sFileName.c_str());
			return 1;
		}
		else if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCci && m_bVerbose)
		{
			UPrintf(USTR("INFO: ignore --type option\n"));
		}
	}
	if (m_eAction == kActionDiff)
	{
		if (m_sOldFileName.empty())
		{
			UPrintf(USTR("ERROR: no --old option\n\n"));
			return 1;
		}
		if (m_sNewFileName.empty())
		{
			UPrintf(USTR("ERROR: no --new option\n\n"));
			return 1;
		}
		if (m_sPatchFileName.empty())
		{
			UPrintf(USTR("ERROR: no --patch-file option\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionPatch)
	{
		if (m_sPatchFileName.empty())
		{
			UPrintf(USTR("ERROR: no --patch-file option\n\n"));
			return 1;
		}
	}
	return 0;
}

int C3dsTool::Help()
{
	UPrintf(USTR("3dstool %") PRIUS USTR(" by dnasdw\n\n"), AToU(_3DSTOOL_VERSION).c_str());
	UPrintf(USTR("usage: 3dstool [option...] [option]...\n\n"));
	UPrintf(USTR("option:\n"));
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			UPrintf(USTR("  "));
			if (pOption->Key != 0)
			{
				UPrintf(USTR("-%c,"), pOption->Key);
			}
			else
			{
				UPrintf(USTR("   "));
			}
			UPrintf(USTR(" --%-8") PRIUS, pOption->Name);
			if (UCslen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				UPrintf(USTR("\n%16") PRIUS, USTR(""));
			}
		}
		if (pOption->Doc != nullptr)
		{
			UPrintf(USTR("%") PRIUS, pOption->Doc);
		}
		UPrintf(USTR("\n"));
		pOption++;
	}
	return 0;
}

int C3dsTool::Action()
{
	if (m_eAction == kActionExtract)
	{
		if (!extractFile())
		{
			UPrintf(USTR("ERROR: extract file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (!createFile())
		{
			UPrintf(USTR("ERROR: create file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionEncrypt)
	{
		if (!encryptFile())
		{
			UPrintf(USTR("ERROR: encrypt file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionUncompress)
	{
		if (!uncompressFile())
		{
			UPrintf(USTR("ERROR: uncompress file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionCompress)
	{
		if (!compressFile())
		{
			UPrintf(USTR("ERROR: compress file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionTrim)
	{
		if (!trimFile())
		{
			UPrintf(USTR("ERROR: trim file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionPad)
	{
		if (!padFile())
		{
			UPrintf(USTR("ERROR: pad file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionDiff)
	{
		if (!diffFile())
		{
			UPrintf(USTR("ERROR: create patch file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionPatch)
	{
		if (!patchFile())
		{
			UPrintf(USTR("ERROR: apply patch file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionSample)
	{
		return sample();
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

C3dsTool::EParseOptionReturn C3dsTool::parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	if (UCscmp(a_pName, USTR("extract")) == 0)
	{
		if (m_eAction == kActionNone || m_eAction == kActionUncompress)
		{
			m_eAction = kActionExtract;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("create")) == 0)
	{
		if (m_eAction == kActionNone || m_eAction == kActionCompress)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("encrypt")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionEncrypt;
		}
		else if (m_eAction != kActionEncrypt && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("uncompress")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionUncompress;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionUncompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bUncompress = true;
	}
	else if (UCscmp(a_pName, USTR("compress")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCompress;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionCompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bCompress = true;
	}
	else if (UCscmp(a_pName, USTR("trim")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionTrim;
		}
		else if (m_eAction != kActionTrim && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("pad")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionPad;
		}
		else if (m_eAction != kActionPad && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("diff")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionDiff;
		}
		else if (m_eAction != kActionDiff && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("patch")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionPatch;
		}
		else if (m_eAction != kActionPatch && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("sample")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionSample;
		}
		else if (m_eAction != kActionSample && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("help")) == 0)
	{
		m_eAction = kActionHelp;
	}
	else if (UCscmp(a_pName, USTR("type")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UChar* pType = a_pArgv[++a_nIndex];
		if (UCscmp(pType, USTR("card")) == 0 || UCscmp(pType, USTR("cci")) == 0 || UCscmp(pType, USTR("3ds")) == 0)
		{
			m_eFileType = kFileTypeCci;
		}
		else if (UCscmp(pType, USTR("exec")) == 0 || UCscmp(pType, USTR("cxi")) == 0)
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (UCscmp(pType, USTR("data")) == 0 || UCscmp(pType, USTR("cfa")) == 0)
		{
			m_eFileType = kFileTypeCfa;
		}
		else if (UCscmp(pType, USTR("exefs")) == 0)
		{
			m_eFileType = kFileTypeExeFs;
		}
		else if (UCscmp(pType, USTR("romfs")) == 0)
		{
			m_eFileType = kFileTypeRomFs;
		}
		else if (UCscmp(pType, USTR("banner")) == 0)
		{
			m_eFileType = kFileTypeBanner;
		}
		else
		{
			m_sMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("verbose")) == 0)
	{
		m_bVerbose = true;
	}
	else if (UCscmp(a_pName, USTR("header")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sHeaderFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("key0")) == 0)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_Key = 0;
	}
	else if (UCscmp(a_pName, USTR("key")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		UString sKey = a_pArgv[++a_nIndex];
		if (sKey.size() != 32 || sKey.find_first_not_of(USTR("0123456789ABCDEFabcdef")) != string::npos)
		{
			return kParseOptionReturnUnknownArgument;
		}
		m_Key = UToA(sKey).c_str();
	}
	else if (UCscmp(a_pName, USTR("counter")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		UString sCounter = a_pArgv[++a_nIndex];
		if (sCounter.size() != 32 || sCounter.find_first_not_of(USTR("0123456789ABCDEFabcdef")) != string::npos)
		{
			return kParseOptionReturnUnknownArgument;
		}
		m_Counter = UToA(sCounter).c_str();
		m_bCounterValid = true;
	}
	else if (UCscmp(a_pName, USTR("xor")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sXorFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("compress-align")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sCompressAlign = a_pArgv[++a_nIndex];
		n32 nCompressAlign = SToN32(sCompressAlign);
		if (nCompressAlign != 1 && nCompressAlign != 4 && nCompressAlign != 8 && nCompressAlign != 16 && nCompressAlign != 32)
		{
			m_sMessage = sCompressAlign;
			return kParseOptionReturnUnknownArgument;
		}
		m_nCompressAlign = nCompressAlign;
	}
	else if (UCscmp(a_pName, USTR("compress-type")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UChar* pType = a_pArgv[++a_nIndex];
		if (UCscmp(pType, USTR("blz")) == 0)
		{
			m_eCompressType = kCompressTypeBlz;
		}
		else if (UCscmp(pType, USTR("lz")) == 0)
		{
			m_eCompressType = kCompressTypeLz;
		}
		else if (UCscmp(pType, USTR("lzex")) == 0)
		{
			m_eCompressType = kCompressTypeLzEx;
		}
		else if (UCscmp(pType, USTR("h4")) == 0)
		{
			m_eCompressType = kCompressTypeH4;
		}
		else if (UCscmp(pType, USTR("h8")) == 0)
		{
			m_eCompressType = kCompressTypeH8;
		}
		else if (UCscmp(pType, USTR("rl")) == 0)
		{
			m_eCompressType = kCompressTypeRl;
		}
		else if (UCscmp(pType, USTR("yaz0")) == 0)
		{
			m_eCompressType = kCompressTypeYaz0;
		}
		else
		{
			m_sMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("compress-out")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sCompressOutFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("yaz0-align")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sYaz0Align = a_pArgv[++a_nIndex];
		n32 nYaz0Align = SToN32(sYaz0Align);
		if (nYaz0Align != 0 && nYaz0Align != 128)
		{
			m_sMessage = sYaz0Align;
			return kParseOptionReturnUnknownArgument;
		}
		m_nYaz0Align = nYaz0Align;
	}
	else if (UCscmp(a_pName, USTR("old")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sOldFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("new")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sNewFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("patch-file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sPatchFileName = a_pArgv[++a_nIndex];
	}
	else if (StartWith<UString>(a_pName, USTR("partition")))
	{
		int nIndex = SToN32(a_pName + UCslen(USTR("partition")));
		if (nIndex < 0 || nIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_mNcchFileName[nIndex] = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("not-pad")) == 0)
	{
		m_bNotPad = true;
	}
	else if (UCscmp(a_pName, USTR("trim-after-partition")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_nLastPartitionIndex = SToN32(a_pArgv[++a_nIndex]);
		if (m_nLastPartitionIndex < 0 || m_nLastPartitionIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
	}
	else if (UCscmp(a_pName, USTR("not-update-extendedheader-hash")) == 0 || UCscmp(a_pName, USTR("not-update-exh-hash")) == 0)
	{
		m_bNotUpdateExtendedHeaderHash = true;
	}
	else if (UCscmp(a_pName, USTR("not-update-exefs-hash")) == 0)
	{
		m_bNotUpdateExeFsHash = true;
	}
	else if (UCscmp(a_pName, USTR("not-update-romfs-hash")) == 0)
	{
		m_bNotUpdateRomFsHash = true;
	}
	else if (UCscmp(a_pName, USTR("extendedheader")) == 0 || UCscmp(a_pName, USTR("exh")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sExtendedHeaderFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("logoregion")) == 0 || UCscmp(a_pName, USTR("logo")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sLogoRegionFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("plainregion")) == 0 || UCscmp(a_pName, USTR("plain")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sPlainRegionFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("exefs")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sExeFsFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("romfs")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sRomFsFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("extendedheader-xor")) == 0 || UCscmp(a_pName, USTR("exh-xor")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sExtendedHeaderXorFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("exefs-xor")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sExeFsXorFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("exefs-top-xor")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (m_bExeFsTopAutoKey)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sExeFsTopXorFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("romfs-xor")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (m_bRomFsAutoKey)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sRomFsXorFileName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("exefs-top-auto-key")) == 0)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (!m_sExeFsTopXorFileName.empty())
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bExeFsTopAutoKey = true;
	}
	else if (UCscmp(a_pName, USTR("romfs-auto-key")) == 0)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (!m_sRomFsXorFileName.empty())
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bRomFsAutoKey = true;
	}
	else if (UCscmp(a_pName, USTR("exefs-dir")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sExeFsDirName = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("romfs-dir")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sRomFsDirName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("banner-dir")) == 0)
	{
		if (a_nIndex + 1 > a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sBannerDirName = a_pArgv[++a_nIndex];
	}
	return kParseOptionReturnSuccess;
}

C3dsTool::EParseOptionReturn C3dsTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, UChar* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool C3dsTool::checkFileType()
{
	if (m_eFileType == kFileTypeUnknown)
	{
		if (CNcsd::IsNcsdFile(m_sFileName))
		{
			m_eFileType = kFileTypeCci;
		}
		else if (CNcch::IsCxiFile(m_sFileName))
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (CNcch::IsCfaFile(m_sFileName))
		{
			m_eFileType = kFileTypeCfa;
		}
		else if (CExeFs::IsExeFsFile(m_sFileName, 0))
		{
			m_eFileType = kFileTypeExeFs;
		}
		else if (CRomFs::IsRomFsFile(m_sFileName))
		{
			m_eFileType = kFileTypeRomFs;
		}
		else if (CBanner::IsBannerFile(m_sFileName))
		{
			m_eFileType = kFileTypeBanner;
		}
		else
		{
			m_sMessage = USTR("unknown file type");
			return false;
		}
	}
	else
	{
		bool bMatch = false;
		switch (m_eFileType)
		{
		case kFileTypeCci:
			bMatch = CNcsd::IsNcsdFile(m_sFileName);
			break;
		case kFileTypeCxi:
			bMatch = CNcch::IsCxiFile(m_sFileName);
			break;
		case kFileTypeCfa:
			bMatch = CNcch::IsCfaFile(m_sFileName);
			break;
		case kFileTypeExeFs:
			bMatch = CExeFs::IsExeFsFile(m_sFileName, 0);
			break;
		case kFileTypeRomFs:
			bMatch = CRomFs::IsRomFsFile(m_sFileName);
			break;
		case kFileTypeBanner:
			bMatch = CBanner::IsBannerFile(m_sFileName);
			break;
		default:
			break;
		}
		if (!bMatch)
		{
			m_sMessage = USTR("the file type is mismatch");
			return false;
		}
	}
	return true;
}

bool C3dsTool::extractFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case kFileTypeCci:
		{
			CNcsd ncsd;
			ncsd.SetFileName(m_sFileName);
			ncsd.SetVerbose(m_bVerbose);
			ncsd.SetHeaderFileName(m_sHeaderFileName);
			ncsd.SetNcchFileName(m_mNcchFileName);
			bResult = ncsd.ExtractFile();
		}
		break;
	case kFileTypeCxi:
		{
			CNcch ncch;
			ncch.SetFileName(m_sFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_sHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetExtendedHeaderFileName(m_sExtendedHeaderFileName);
			ncch.SetLogoRegionFileName(m_sLogoRegionFileName);
			ncch.SetPlainRegionFileName(m_sPlainRegionFileName);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExtendedHeaderXorFileName(m_sExtendedHeaderXorFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetExeFsTopXorFileName(m_sExeFsTopXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetExeFsTopAutoKey(m_bExeFsTopAutoKey);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.ExtractFile();
		}
		break;
	case kFileTypeCfa:
		{
			CNcch ncch;
			ncch.SetFileName(m_sFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_sHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.ExtractFile();
		}
		break;
	case kFileTypeExeFs:
		{
			CExeFs exeFs;
			exeFs.SetFileName(m_sFileName);
			exeFs.SetVerbose(m_bVerbose);
			exeFs.SetHeaderFileName(m_sHeaderFileName);
			exeFs.SetExeFsDirName(m_sExeFsDirName);
			exeFs.SetUncompress(m_bUncompress);
			bResult = exeFs.ExtractFile();
		}
		break;
	case kFileTypeRomFs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_sFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.SetRomFsDirName(m_sRomFsDirName);
			bResult = romFs.ExtractFile();
		}
		break;
	case kFileTypeBanner:
		{
			CBanner banner;
			banner.SetFileName(m_sFileName);
			banner.SetVerbose(m_bVerbose);
			banner.SetBannerDirName(m_sBannerDirName);
			bResult = banner.ExtractFile();
		}
		break;
	default:
		break;
	}
	return bResult;
}

bool C3dsTool::createFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case kFileTypeCci:
		{
			CNcsd ncsd;
			ncsd.SetFileName(m_sFileName);
			ncsd.SetVerbose(m_bVerbose);
			ncsd.SetHeaderFileName(m_sHeaderFileName);
			ncsd.SetNcchFileName(m_mNcchFileName);
			ncsd.SetNotPad(m_bNotPad);
			bResult = ncsd.CreateFile();
		}
		break;
	case kFileTypeCxi:
		{
			CNcch ncch;
			ncch.SetFileName(m_sFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_sHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetNotUpdateExtendedHeaderHash(m_bNotUpdateExtendedHeaderHash);
			ncch.SetNotUpdateExeFsHash(m_bNotUpdateExeFsHash);
			ncch.SetNotUpdateRomFsHash(m_bNotUpdateRomFsHash);
			ncch.SetExtendedHeaderFileName(m_sExtendedHeaderFileName);
			ncch.SetLogoRegionFileName(m_sLogoRegionFileName);
			ncch.SetPlainRegionFileName(m_sPlainRegionFileName);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExtendedHeaderXorFileName(m_sExtendedHeaderXorFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetExeFsTopXorFileName(m_sExeFsTopXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetExeFsTopAutoKey(m_bExeFsTopAutoKey);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.CreateFile();
		}
		break;
	case kFileTypeCfa:
		{
			CNcch ncch;
			ncch.SetFileName(m_sFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_sHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetNotUpdateExeFsHash(m_bNotUpdateExeFsHash);
			ncch.SetNotUpdateRomFsHash(m_bNotUpdateRomFsHash);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.CreateFile();
		}
		break;
	case kFileTypeExeFs:
		{
			CExeFs exeFs;
			exeFs.SetFileName(m_sFileName);
			exeFs.SetVerbose(m_bVerbose);
			exeFs.SetHeaderFileName(m_sHeaderFileName);
			exeFs.SetExeFsDirName(m_sExeFsDirName);
			exeFs.SetCompress(m_bCompress);
			bResult = exeFs.CreateFile();
		}
		break;
	case kFileTypeRomFs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_sFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.SetRomFsDirName(m_sRomFsDirName);
			romFs.SetRomFsFileName(m_sRomFsFileName);
			bResult = romFs.CreateFile();
		}
		break;
	case kFileTypeBanner:
		{
			CBanner banner;
			banner.SetFileName(m_sFileName);
			banner.SetVerbose(m_bVerbose);
			banner.SetBannerDirName(m_sBannerDirName);
			bResult = banner.CreateFile();
		}
		break;
	default:
		break;
	}
	return bResult;
}

bool C3dsTool::encryptFile()
{
	bool bResult = false;
	if (m_nEncryptMode == CNcch::kEncryptModeAesCtr && m_bCounterValid)
	{
		bResult = FEncryptAesCtrFile(m_sFileName, m_Key, m_Counter, 0, 0, true, 0);
	}
	else if (m_nEncryptMode == CNcch::kEncryptModeXor && !m_sXorFileName.empty())
	{
		bResult = FEncryptXorFile(m_sFileName, m_sXorFileName, 0, 0, true, 0);
	}
	else if (CNcch::IsCxiFile(m_sFileName))
	{
		CNcch ncch;
		ncch.SetFileName(m_sFileName);
		ncch.SetVerbose(m_bVerbose);
		ncch.SetEncryptMode(m_nEncryptMode);
		ncch.SetKey(m_Key);
		ncch.SetExtendedHeaderXorFileName(m_sExtendedHeaderXorFileName);
		ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
		ncch.SetExeFsTopXorFileName(m_sExeFsTopXorFileName);
		ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
		ncch.SetExeFsTopAutoKey(m_bExeFsTopAutoKey);
		ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
		bResult = ncch.EncryptFile();
	}
	else if (CNcch::IsCfaFile(m_sFileName))
	{
		CNcch ncch;
		ncch.SetFileName(m_sFileName);
		ncch.SetVerbose(m_bVerbose);
		ncch.SetEncryptMode(m_nEncryptMode);
		ncch.SetKey(m_Key);
		ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
		ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
		ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
		bResult = ncch.EncryptFile();
	}
	return bResult;
}

bool C3dsTool::uncompressFile()
{
	FILE* fp = Fopen(m_sFileName.c_str(), "rb");
	bool bResult = fp != nullptr;
	if (bResult)
	{
		Fseek(fp, 0, SEEK_END);
		u32 uCompressedSize = static_cast<u32>(Ftell(fp));
		Fseek(fp, 0, SEEK_SET);
		u8* pCompressed = new u8[uCompressedSize];
		fread(pCompressed, 1, uCompressedSize, fp);
		fclose(fp);
		u32 uUncompressedSize = 0;
		switch (m_eCompressType)
		{
		case kCompressTypeBlz:
			bResult = CBackwardLz77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeLz:
		case kCompressTypeLzEx:
			bResult = CLz77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeH4:
		case kCompressTypeH8:
			bResult = CHuffman::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeRl:
			bResult = CRunLength::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeYaz0:
			bResult = CYaz0::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		default:
			break;
		}
		if (bResult)
		{
			u8* pUncompressed = new u8[uUncompressedSize];
			switch (m_eCompressType)
			{
			case kCompressTypeBlz:
				bResult = CBackwardLz77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeLz:
			case kCompressTypeLzEx:
				bResult = CLz77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeH4:
			case kCompressTypeH8:
				bResult = CHuffman::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeRl:
				bResult = CRunLength::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeYaz0:
				bResult = CYaz0::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			default:
				break;
			}
			if (bResult)
			{
				fp = Fopen(m_sCompressOutFileName.c_str(), "wb");
				bResult = fp != nullptr;
				if (bResult)
				{
					fwrite(pUncompressed, 1, uUncompressedSize, fp);
					fclose(fp);
				}
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
	return bResult;
}

bool C3dsTool::compressFile()
{
	FILE* fp = Fopen(m_sFileName.c_str(), "rb");
	bool bReuslt = fp != nullptr;
	if (bReuslt)
	{
		Fseek(fp, 0, SEEK_END);
		u32 uUncompressedSize = static_cast<u32>(Ftell(fp));
		Fseek(fp, 0, SEEK_SET);
		u8* pUncompressed = new u8[uUncompressedSize];
		fread(pUncompressed, 1, uUncompressedSize, fp);
		fclose(fp);
		u32 uCompressedSize = 0;
		switch (m_eCompressType)
		{
		case kCompressTypeBlz:
			uCompressedSize = uUncompressedSize;
			break;
		case kCompressTypeLz:
		case kCompressTypeLzEx:
			uCompressedSize = CLz77::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeH4:
		case kCompressTypeH8:
			uCompressedSize = CHuffman::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeRl:
			uCompressedSize = CRunLength::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeYaz0:
			uCompressedSize = CYaz0::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		default:
			break;
		}
		u8* pCompressed = new u8[uCompressedSize];
		switch (m_eCompressType)
		{
		case kCompressTypeBlz:
			bReuslt = CBackwardLz77::Compress(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize);
			break;
		case kCompressTypeLz:
			bReuslt = CLz77::CompressLz(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeLzEx:
			bReuslt = CLz77::CompressLzEx(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeH4:
			bReuslt = CHuffman::CompressH4(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeH8:
			bReuslt = CHuffman::CompressH8(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeRl:
			bReuslt = CRunLength::Compress(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeYaz0:
			bReuslt = CYaz0::Compress(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign, m_nYaz0Align);
			break;
		default:
			break;
		}
		if (bReuslt)
		{
			fp = Fopen(m_sCompressOutFileName.c_str(), "wb");
			bReuslt = fp != nullptr;
			if (bReuslt)
			{
				fwrite(pCompressed, 1, uCompressedSize, fp);
				fclose(fp);
			}
		}
		else
		{
			UPrintf(USTR("ERROR: compress error\n\n"));
		}
		delete[] pCompressed;
		delete[] pUncompressed;
	}
	return bReuslt;
}

bool C3dsTool::trimFile()
{
	CNcsd ncsd;
	ncsd.SetFileName(m_sFileName);
	ncsd.SetVerbose(m_bVerbose);
	ncsd.SetLastPartitionIndex(m_nLastPartitionIndex);
	bool bResult = ncsd.TrimFile();
	return bResult;
}

bool C3dsTool::padFile()
{
	CNcsd ncsd;
	ncsd.SetFileName(m_sFileName);
	ncsd.SetVerbose(m_bVerbose);
	bool bResult = ncsd.PadFile();
	return bResult;
}

bool C3dsTool::diffFile()
{
	CPatch patch;
	patch.SetFileType(m_eFileType);
	patch.SetVerbose(m_bVerbose);
	patch.SetOldFileName(m_sOldFileName);
	patch.SetNewFileName(m_sNewFileName);
	patch.SetPatchFileName(m_sPatchFileName);
	return patch.CreatePatchFile();
}

bool C3dsTool::patchFile()
{
	CPatch patch;
	patch.SetFileName(m_sFileName);
	patch.SetVerbose(m_bVerbose);
	patch.SetPatchFileName(m_sPatchFileName);
	return patch.ApplyPatchFile();
}

int C3dsTool::sample()
{
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("# extract cci\n"));
	UPrintf(USTR("3dstool -xvt017f cci 0.cxi 1.cfa 7.cfa input.3ds --header ncsdheader.bin\n\n"));
	UPrintf(USTR("# extract cxi without encryption\n"));
	UPrintf(USTR("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin\n\n"));
	UPrintf(USTR("# extract cxi with AES-CTR encryption\n"));
	UPrintf(USTR("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n"));
	UPrintf(USTR("# extract cxi with xor encryption\n"));
	UPrintf(USTR("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n"));
	UPrintf(USTR("# extract cxi with 7.x xor encryption\n"));
	UPrintf(USTR("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-xor 000400000XXXXX00.0.exefs_top.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n"));
	UPrintf(USTR("# extract cxi with 7.x auto encryption\n"));
	UPrintf(USTR("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-auto-key --romfs-auto-key\n\n"));
	UPrintf(USTR("# extract cfa without encryption\n"));
	UPrintf(USTR("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin\n\n"));
	UPrintf(USTR("# extract cfa with AES-CTR encryption\n"));
	UPrintf(USTR("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n"));
	UPrintf(USTR("# extract cfa with xor encryption\n"));
	UPrintf(USTR("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --romfs-xor 000400000XXXXX00.1.romfs.xor\n\n"));
	UPrintf(USTR("# extract exefs without Backward LZ77 uncompress\n"));
	UPrintf(USTR("3dstool -xvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n"));
	UPrintf(USTR("# extract exefs with Backward LZ77 uncompress\n"));
	UPrintf(USTR("3dstool -xuvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n"));
	UPrintf(USTR("# extract romfs\n"));
	UPrintf(USTR("3dstool -xvtf romfs romfs.bin --romfs-dir romfs\n\n"));
	UPrintf(USTR("# extract banner\n"));
	UPrintf(USTR("3dstool -xvtf banner banner.bnr --banner-dir banner\n\n"));
	UPrintf(USTR("# create cci with pad 0xFF\n"));
	UPrintf(USTR("3dstool -cvt017f cci 0.cxi 1.cfa 7.cfa output.3ds --header ncsdheader.bin\n\n"));
	UPrintf(USTR("# create cci without pad\n"));
	UPrintf(USTR("3dstool -cvt017f cci 0.cxi 1.cfa 7.cfa output.3ds --header ncsdheader.bin --not-pad\n\n"));
	UPrintf(USTR("# create cxi without encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --not-update-exh-hash --not-update-exefs-hash --not-update-romfs-hash\n\n"));
	UPrintf(USTR("# create cxi with AES-CTR encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n"));
	UPrintf(USTR("# create cxi with xor encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n"));
	UPrintf(USTR("# create cxi with 7.x xor encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-xor 000400000XXXXX00.0.exefs_top.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n"));
	UPrintf(USTR("# create cxi with 7.x auto encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-auto-key --romfs-auto-key\n\n"));
	UPrintf(USTR("# create cfa without encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --not-update-romfs-hash\n\n"));
	UPrintf(USTR("# create cfa with AES-CTR encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n"));
	UPrintf(USTR("# create cfa with xor encryption and calculate hash\n"));
	UPrintf(USTR("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n"));
	UPrintf(USTR("# create exefs without Backward LZ77 compress\n"));
	UPrintf(USTR("3dstool -cvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n"));
	UPrintf(USTR("# create exefs with Backward LZ77 compress\n"));
	UPrintf(USTR("3dstool -czvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n"));
	UPrintf(USTR("# create romfs without reference\n"));
	UPrintf(USTR("3dstool -cvtf romfs romfs.bin --romfs-dir romfs\n\n"));
	UPrintf(USTR("# create romfs with reference\n"));
	UPrintf(USTR("3dstool -cvtf romfs romfs.bin --romfs-dir romfs --romfs original_romfs.bin\n\n"));
	UPrintf(USTR("# create banner\n"));
	UPrintf(USTR("3dstool -cvtf banner banner.bnr --banner-dir banner\n\n"));
	UPrintf(USTR("# encrypt file with AES-CTR encryption, standalone\n"));
	UPrintf(USTR("3dstool -evf file.bin --key 00000000000000000000000000000000 --counter 00000000000000000000000000000000\n\n"));
	UPrintf(USTR("# encrypt file with xor encryption, standalone\n"));
	UPrintf(USTR("3dstool -evf file.bin --xor xor.bin\n\n"));
	UPrintf(USTR("# uncompress file with Backward LZ77, standalone\n"));
	UPrintf(USTR("3dstool -uvf code.bin --compress-type blz --compress-out code.bin\n\n"));
	UPrintf(USTR("# compress file with Backward LZ77, standalone\n"));
	UPrintf(USTR("3dstool -zvf code.bin --compress-type blz --compress-out code.bin\n\n"));
	UPrintf(USTR("# uncompress file with LZ77, standalone\n"));
	UPrintf(USTR("3dstool -uvf input.lz --compress-type lz --compress-out output.bin\n\n"));
	UPrintf(USTR("# compress file with LZ77, standalone\n"));
	UPrintf(USTR("3dstool -zvf input.bin --compress-type lz --compress-out output.lz\n\n"));
	UPrintf(USTR("# compress file with LZ77 and align to 4 bytes, standalone\n"));
	UPrintf(USTR("3dstool -zvf input.bin --compress-type lz --compress-out output.lz --compress-align 4\n\n"));
	UPrintf(USTR("# uncompress file with LZ77Ex, standalone\n"));
	UPrintf(USTR("3dstool -uvf logo.darc.lz --compress-type lzex --compress-out logo.darc\n\n"));
	UPrintf(USTR("# compress file with LZ77Ex, standalone\n"));
	UPrintf(USTR("3dstool -zvf logo.darc --compress-type lzex --compress-out logo.darc.lz\n\n"));
	UPrintf(USTR("# uncompress file with Huffman 4bits, standalone\n"));
	UPrintf(USTR("3dstool -uvf input.bin --compress-type h4 --compress-out output.bin\n\n"));
	UPrintf(USTR("# compress file with Huffman 4bits, standalone\n"));
	UPrintf(USTR("3dstool -zvf input.bin --compress-type h4 --compress-out output.bin\n\n"));
	UPrintf(USTR("# uncompress file with Huffman 8bits, standalone\n"));
	UPrintf(USTR("3dstool -uvf input.bin --compress-type h8 --compress-out output.bin\n\n"));
	UPrintf(USTR("# compress file with Huffman 8bits, standalone\n"));
	UPrintf(USTR("3dstool -zvf input.bin --compress-type h8 --compress-out output.bin\n\n"));
	UPrintf(USTR("# uncompress file with RunLength, standalone\n"));
	UPrintf(USTR("3dstool -uvf input.bin --compress-type rl --compress-out output.bin\n\n"));
	UPrintf(USTR("# compress file with RunLength, standalone\n"));
	UPrintf(USTR("3dstool -zvf input.bin --compress-type rl --compress-out output.bin\n\n"));
	UPrintf(USTR("# uncompress file with Yaz0, standalone\n"));
	UPrintf(USTR("3dstool -uvf input.szs --compress-type yaz0 --compress-out output.sarc\n\n"));
	UPrintf(USTR("# compress file with Yaz0, standalone\n"));
	UPrintf(USTR("3dstool -zvf input.sarc --compress-type yaz0 --compress-out output.szs\n\n"));
	UPrintf(USTR("# compress file with Yaz0 and set the alignment property, standalone\n"));
	UPrintf(USTR("3dstool -zvf input.sarc --compress-type yaz0 --compress-out output.szs --yaz0-align 128\n\n"));
	UPrintf(USTR("# trim cci without pad\n"));
	UPrintf(USTR("3dstool --trim -vtf cci input.3ds\n\n"));
	UPrintf(USTR("# trim cci reserve partition 0~2\n"));
	UPrintf(USTR("3dstool --trim -vtf cci input.3ds --trim-after-partition 2\n\n"));
	UPrintf(USTR("# pad cci with 0xFF\n"));
	UPrintf(USTR("3dstool --pad -vtf cci input.3ds\n\n"));
	UPrintf(USTR("# create patch file without optimization\n"));
	UPrintf(USTR("3dstool --diff -v --old old.bin --new new.bin --patch-file patch.3ps\n\n"));
	UPrintf(USTR("# create patch file with cci optimization\n"));
	UPrintf(USTR("3dstool --diff -vt cci --old old.3ds --new new.3ds --patch-file patch.3ps\n\n"));
	UPrintf(USTR("# create patch file with cxi optimization\n"));
	UPrintf(USTR("3dstool --diff -vt cxi --old old.cxi --new new.cxi --patch-file patch.3ps\n\n"));
	UPrintf(USTR("# create patch file with cfa optimization\n"));
	UPrintf(USTR("3dstool --diff -vt cfa --old old.cfa --new new.cfa --patch-file patch.3ps\n\n"));
	UPrintf(USTR("# apply patch file\n"));
	UPrintf(USTR("3dstool --patch -vf input.bin --patch-file patch.3ps\n\n"));
	return 0;
}

int UMain(int argc, UChar* argv[])
{
	SetLocale();
	C3dsTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
