#ifndef CODE_H_
#define CODE_H_

#include "utility.h"
#include <capstone.h>

class CCode
{
public:
	struct SFunction
	{
		n32 First;
		n32 Last;
	};
	CCode();
	~CCode();
	void SetFileName(const UString& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetRegionCode(n32 a_nRegionCode);
	void SetLanguageCode(n32 a_nLanguageCode);
	bool Lock();
private:
	bool lockArm();
	bool lockThumb();
	bool lockRegionArm();
	bool lockRegionThumb();
	bool lockLanguageArm();
	bool lockLanguageThumb();
	void findGetRegionFunctionArm(SFunction& a_Function);
	void findGetRegionFunctionThumb(SFunction& a_Function);
	void findGetLanguageFunctionArm(SFunction& a_Function);
	void findGetLanguageFunctionThumb(SFunction& a_Function);
	void findFunctionArm(SFunction& a_Function);
	void findFunctionThumb(SFunction& a_Function);
	bool patchGetRegionFunctionArm(SFunction& a_Function);
	bool patchGetRegionFunctionThumb(SFunction& a_Function);
	bool patchGetLanguageFunctionArm(SFunction& a_Function);
	bool patchGetLanguageFunctionThumb(SFunction& a_Function);
	UString m_sFileName;
	bool m_bVerbose;
	n32 m_nRegionCode;
	n32 m_nLanguageCode;
	vector<u8> m_vCode;
	u32* m_pArm;
	n32 m_nArmCount;
	u8* m_pThumb;
	n32 m_nThumbSize;
	csh m_uHandle;
	cs_insn* m_pInsn;
	size_t m_uDisasmCount;
};

#endif	// CODE_H_
