#include "space.h"

SBuffer::SBuffer(n64 a_nTop, n64 a_nBottom)
	: Top(a_nTop)
	, Bottom(a_nBottom)
{
}

CSpace::CSpace()
{
}

CSpace::~CSpace()
{
}

bool CSpace::AddSpace(n64 a_nOffset, n64 a_nSize)
{
	if (a_nSize == 0)
	{
		return true;
	}
	n64 nTop = a_nOffset;
	n64 nBottom = a_nOffset + a_nSize;
	for (list<SBuffer>::iterator it = m_lBuffer.begin(); it != m_lBuffer.end(); ++it)
	{
		SBuffer& buffer = *it;
		if ((nTop >= buffer.Top && nTop < buffer.Bottom) || (nBottom > buffer.Top && nBottom <= buffer.Bottom))
		{
			UPrintf(USTR("ERROR: [%") PRIUS USTR(", %") PRIUS USTR(") [%") PRIUS USTR(", %") PRIUS USTR(") overlap\n\n"), AToU(Format("0x%" PRIX64, nTop)).c_str(), AToU(Format("0x%" PRIX64, nBottom)).c_str(), AToU(Format("0x%" PRIX64, buffer.Top)).c_str(), AToU(Format("0x%" PRIX64, buffer.Bottom)).c_str());
			return false;
		}
		if (nBottom < buffer.Top)
		{
			m_lBuffer.insert(it, SBuffer(nTop, nBottom));
			return true;
		}
		else if (nBottom == buffer.Top)
		{
			buffer.Top = nTop;
			return true;
		}
		else if (nTop == buffer.Bottom)
		{
			list<SBuffer>::iterator itNext = it;
			++itNext;
			if (itNext == m_lBuffer.end() || nBottom < itNext->Top)
			{
				buffer.Bottom = nBottom;
				return true;
			}
			else if (nBottom == itNext->Top)
			{
				buffer.Bottom = itNext->Bottom;
				m_lBuffer.erase(itNext);
				return true;
			}
		}
	}
	m_lBuffer.push_back(SBuffer(nTop, nBottom));
	return true;
}

bool CSpace::SubSpace(n64 a_nOffset, n64 a_nSize)
{
	if (a_nSize == 0)
	{
		return true;
	}
	n64 nTop = a_nOffset;
	n64 nBottom = a_nOffset + a_nSize;
	for (list<SBuffer>::iterator it = m_lBuffer.begin(); it != m_lBuffer.end(); ++it)
	{
		SBuffer& buffer = *it;
		if (nTop == buffer.Top && nBottom == buffer.Bottom)
		{
			m_lBuffer.erase(it);
			return true;
		}
		else if (nTop == buffer.Top && nBottom < buffer.Bottom)
		{
			buffer.Top = nBottom;
			return true;
		}
		else if (nTop > buffer.Top && nBottom == buffer.Bottom)
		{
			buffer.Bottom = nTop;
			return true;
		}
		else if (nTop > buffer.Top && nBottom < buffer.Bottom)
		{
			list<SBuffer>::iterator itNext = it;
			++itNext;
			m_lBuffer.insert(itNext, SBuffer(nBottom, buffer.Bottom));
			buffer.Bottom = nTop;
			return true;
		}
	}
	return false;
}

void CSpace::Clear()
{
	m_lBuffer.clear();
}

n64 CSpace::GetSpace(n64 a_nSize) const
{
	for (list<SBuffer>::const_iterator it = m_lBuffer.begin(); it != m_lBuffer.end(); ++it)
	{
		const SBuffer& buffer = *it;
		if (buffer.Bottom - buffer.Top >= a_nSize)
		{
			return buffer.Top;
		}
	}
	return -1;
}
