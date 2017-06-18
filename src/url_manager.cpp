#include "url_manager.h"

bool CUrlManager::s_bInitialized = false;

const string& CUrl::GetData() const
{
	return m_sData;
}

size_t CUrl::OnWrite(char* a_pData, size_t a_uSize, size_t a_uNmemb, void* a_pUserData)
{
	size_t uSize = a_uSize * a_uNmemb;
	CUrl* pUrl = reinterpret_cast<CUrl*>(a_pUserData);
	if (pUrl != nullptr)
	{
		pUrl->m_sData.append(a_pData, uSize);
	}
	return uSize;
}

CUrl::CUrl(void* a_pUserData)
	: m_eMultiCode(CURLM_OK)
	, m_pCurlm(nullptr)
	, m_pCurl(nullptr)
	, m_nStillRunning(0)
	, m_pUserData(a_pUserData)
{
}

CUrl::~CUrl()
{
	if (m_pCurlm != nullptr)
	{
		if (m_pCurl != nullptr)
		{
			curl_multi_remove_handle(m_pCurlm, m_pCurl);
			curl_easy_cleanup(m_pCurl);
		}
		curl_multi_cleanup(m_pCurlm);
	}
}

CUrlManager::CUrlManager()
{
}

CUrlManager::~CUrlManager()
{
}

void CUrlManager::Cleanup()
{
	for (unordered_set<CUrl*>::iterator it = m_sUrl.begin(); it != m_sUrl.end(); )
	{
		CUrl* pUrl = *it;
		it = m_sUrl.erase(it);
		delete pUrl;
	}
}

u32 CUrlManager::GetCount() const
{
	return static_cast<u32>(m_sUrl.size());
}

void CUrlManager::Perform()
{
	for (unordered_set<CUrl*>::iterator it = m_sUrl.begin(); it != m_sUrl.end(); )
	{
		CUrl* pUrl = *it;
		pUrl->m_eMultiCode = curl_multi_perform(pUrl->m_pCurlm, &pUrl->m_nStillRunning);
		if (pUrl->m_eMultiCode > CURLM_OK || pUrl->m_nStillRunning == 0)
		{
			pUrl->OnWriteOver();
			it = m_sUrl.erase(it);
			delete pUrl;
		}
		else
		{
			++it;
		}
	}
}

bool CUrlManager::Initialize()
{
	if (!s_bInitialized)
	{
		CURLcode eError = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (eError == CURLE_OK)
		{
			s_bInitialized = true;
		}
	}
	return s_bInitialized;
}

void CUrlManager::Finalize()
{
	if (s_bInitialized)
	{
		curl_global_cleanup();
		s_bInitialized = false;
	}
}
