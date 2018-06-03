#ifndef _3DSTOOL_H_
#define _3DSTOOL_H_

#include "utility.h"
#include "bignum.h"

class C3dsTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionExtract,
		kActionCreate,
		kActionEncrypt,
		kActionUncompress,
		kActionCompress,
		kActionTrim,
		kActionPad,
		kActionDiff,
		kActionPatch,
		kActionLock,
		kActionDownload,
		kActionSample,
		kActionHelp
	};
	enum EFileType
	{
		kFileTypeUnknown,
		kFileTypeCci,
		kFileTypeCxi,
		kFileTypeCfa,
		kFileTypeExeFs,
		kFileTypeRomFs,
		kFileTypeBanner
	};
	enum ECompressType
	{
		kCompressTypeNone,
		kCompressTypeBlz,
		kCompressTypeLz,
		kCompressTypeLzEx,
		kCompressTypeH4,
		kCompressTypeH8,
		kCompressTypeRl,
		kCompressTypeYaz0
	};
	struct SOption
	{
		const UChar* Name;
		int Key;
		const UChar* Doc;
	};
	C3dsTool();
	~C3dsTool();
	int ParseOptions(int a_nArgc, UChar* a_pArgv[]);
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool checkFileType();
	bool extractFile();
	bool createFile();
	bool encryptFile();
	bool uncompressFile();
	bool compressFile();
	bool trimFile();
	bool padFile();
	bool diffFile();
	bool patchFile();
	bool lock();
	bool download();
	int sample();
	EAction m_eAction;
	EFileType m_eFileType;
	UString m_sFileName;
	bool m_bVerbose;
	UString m_sHeaderFileName;
	int m_nEncryptMode;
	CBigNum m_Key;
	CBigNum m_Counter;
	UString m_sXorFileName;
	bool m_bRemoveExtKey;
	bool m_bDev;
	n32 m_nCompressAlign;
	ECompressType m_eCompressType;
	UString m_sCompressOutFileName;
	n32 m_nYaz0Align;
	UString m_sOldFileName;
	UString m_sNewFileName;
	UString m_sPatchFileName;
	n32 m_nRegionCode;
	n32 m_nLanguageCode;
	n32 m_nDownloadBegin;
	n32 m_nDownloadEnd;
	map<int, UString> m_mNcchFileName;
	bool m_bNotPad;
	int m_nLastPartitionIndex;
	UString m_sExtendedHeaderFileName;
	UString m_sLogoRegionFileName;
	UString m_sPlainRegionFileName;
	UString m_sExeFsFileName;
	UString m_sRomFsFileName;
	UString m_sExeFsDirName;
	UString m_sRomFsDirName;
	UString m_sBannerDirName;
	bool m_bKeyValid;
	bool m_bCounterValid;
	bool m_bUncompress;
	bool m_bCompress;
	UString m_sMessage;
};

#endif	// _3DSTOOL_H_
