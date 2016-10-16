#ifndef SPACE_H_
#define SPACE_H_

#include "utility.h"

struct SBuffer
{
	n64 Top;
	n64 Bottom;
	SBuffer(n64 a_nTop, n64 a_nBottom);
};

class CSpace
{
public:
	CSpace();
	~CSpace();
	bool AddSpace(n64 a_nOffset, n64 a_nSize);
	bool SubSpace(n64 a_nOffset, n64 a_nSize);
	void Clear();
	n64 GetSpace(n64 a_nSize) const;
private:
	list<SBuffer> m_lBuffer;
};

#endif	// SPACE_H_
