#ifndef PATCH_H_
#define PATCH_H_

#include "utility.h"
#include "3dstool.h"

class CPatch
{
public:
	enum EPatchCommand
	{
		kPatchCommandOver,
		kPatchCommandCheck,
		kPatchCommandMove,
		kPatchCommandSet,
		kPatchCommandChangeSize,
		kPatchCommandSeekWrite = 0x10
	};
	struct S3dsPatchSystemHeader
	{
		u32 Signature;
		u8 VersionMajor;
		u8 VersionMinor;
		u8 VersionPatchLevel;
		u8 Reserved;
		n64 ExtDataOffset;
	};
	CPatch();
	~CPatch();
	void SetFileType(C3dsTool::EFileType a_eFileType);
	void SetFileName(const UString& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetOldFileName(const UString& a_sOldFileName);
	void SetNewFileName(const UString& a_sNewFileName);
	void SetPatchFileName(const UString& a_sPatchFileName);
	bool CreatePatchFile();
	bool ApplyPatchFile();
	static const u32 s_uSignature;
	static const u8 s_uCurrentVersionMajor;
	static const u8 s_uCurrentVersionMinor;
	static const u8 s_uCurrentVersionPatchLevel;
private:
	bool createNcsdPatchFile();
	bool createNcchPatchFile(C3dsTool::EFileType a_eFileType, n64 a_nOffsetOld, n64 a_nOffsetNew, bool a_bCreateCheck);
	bool createPatchFile(n64 a_nOffsetOld, n64 a_nSizeOld, n64 a_nOffsetNew, n64 a_nSizeNew);
	void writeOver();
	void writeCheck(n64 a_nOffset, n64 a_nSize, u8* a_pSHA256);
	void writeMove(n64 a_nFromOffset, n64 a_nToOffset, n64 a_nSize);
	void writeSet(n64 a_nStartOffset, n64 a_nSize, u8 a_uData);
	void writeChangeSize(n64 a_nSize);
	void writeSeekWrite(bool a_bSeekSet, n64 a_nOffset, size_t a_nSize, u8* a_pData);
	void writePatch(u8 a_uPatchCommand, n64* a_pArg);
	void writePatch(u8 a_uPatchCommand);
	void calculateVersion();
	void executeMove(n64 a_nFromOffset, n64 a_nToOffset, n64 a_nSize);
	void executeSet(n64 a_nStartOffset, n64 a_nSize, u8 a_uData);
	void executeChangeSize(n64 a_nSize);
	void executeSeekWrite(bool a_bSeekSet, n64 a_nOffset, size_t a_nSize, u8* a_pData);
	C3dsTool::EFileType m_eFileType;
	UString m_sFileName;
	bool m_bVerbose;
	UString m_sOldFileName;
	UString m_sNewFileName;
	UString m_sPatchFileName;
	FILE* m_fpOld;
	FILE* m_fpNew;
	FILE* m_fpPatch;
	S3dsPatchSystemHeader m_3dsPatchSystemHeader;
	u32 m_uVersion;
};

#endif	// PATCH_H_
