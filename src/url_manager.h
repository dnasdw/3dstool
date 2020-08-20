#ifndef URL_MANAGER_H_
#define URL_MANAGER_H_

#include "utility.h"
#include <curl/curl.h>

class CUrl
{
public:
	friend class CUrlManager;
	const string& GetData() const;
	static size_t OnWrite(char* a_pData, size_t a_uSize, size_t a_uNmemb, void* a_pUserData);
protected:
	CUrl(void* a_pUserData);
	virtual ~CUrl();
	virtual void OnWriteOver() SDW_PURE;
	CURLMcode m_eMultiCode;
	CURLM* m_pCurlm;
	CURL* m_pCurl;
	n32 m_nStillRunning;
	string m_sData;
	void* m_pUserData;
};

class CFunctionUrl : public CUrl
{
public:
	CFunctionUrl(void(*a_pFunction)(CUrl*, void*), void* a_pUserData)
		: CUrl(a_pUserData)
		, m_pFunction(a_pFunction)
	{
	}
	virtual void OnWriteOver()
	{
		if (m_pFunction != nullptr)
		{
			(*m_pFunction)(this, m_pUserData);
		}
	}
private:
	void (*m_pFunction)(CUrl*, void*);
};

template<typename T>
class CMemberFunctionUrl : public CUrl
{
public:
	CMemberFunctionUrl(T& a_Obj, void (T::*a_pMemberFunction)(CUrl*, void*), void* a_pUserData)
		: CUrl(a_pUserData)
		, m_Obj(a_Obj)
		, m_pMemberFunction(a_pMemberFunction)
	{
	}
	virtual void OnWriteOver()
	{
		if (m_pMemberFunction != nullptr)
		{
			(m_Obj.*m_pMemberFunction)(this, m_pUserData);
		}
	}
private:
	T& m_Obj;
	void (T::*m_pMemberFunction)(CUrl* a_pUrl, void* a_pUserData);
};

class CUrlManager
{
public:
	CUrlManager();
	~CUrlManager();
	CUrl* HttpsGet(const string& a_sUrl, void(*a_pFunction)(CUrl*, void*), void* a_pUserData)
	{
		if (!s_bInitialized && !Initialize())
		{
			return nullptr;
		}
		CUrl* pUrl = new CFunctionUrl(a_pFunction, a_pUserData);
		if (pUrl == nullptr)
		{
			return nullptr;
		}
		pUrl->m_pCurlm = curl_multi_init();
		if (pUrl->m_pCurlm == nullptr)
		{
			delete pUrl;
			return nullptr;
		}
		pUrl->m_pCurl = curl_easy_init();
		if (pUrl->m_pCurl == nullptr)
		{
			delete pUrl;
			return nullptr;
		}
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_URL, a_sUrl.c_str());
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_WRITEFUNCTION, &CUrl::OnWrite);
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_WRITEDATA, pUrl);
		curl_multi_add_handle(pUrl->m_pCurlm, pUrl->m_pCurl);
		pUrl->m_eMultiCode = curl_multi_perform(pUrl->m_pCurlm, &pUrl->m_nStillRunning);
		if (!m_sUrl.insert(pUrl).second)
		{
			delete pUrl;
			return nullptr;
		}
		return pUrl;
	}
	template<typename T>
	CUrl* HttpsGet(const string& a_sUrl, T& a_Obj, void (T::*a_pMemberFunction)(CUrl*, void*), void* a_pUserData)
	{
		if (!s_bInitialized && !Initialize())
		{
			return nullptr;
		}
		CUrl* pUrl = new CMemberFunctionUrl<T>(a_Obj, a_pMemberFunction, a_pUserData);
		if (pUrl == nullptr)
		{
			return nullptr;
		}
		pUrl->m_pCurlm = curl_multi_init();
		if (pUrl->m_pCurlm == nullptr)
		{
			delete pUrl;
			return nullptr;
		}
		pUrl->m_pCurl = curl_easy_init();
		if (pUrl->m_pCurl == nullptr)
		{
			delete pUrl;
			return nullptr;
		}
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_URL, a_sUrl.c_str());
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_WRITEFUNCTION, &CUrl::OnWrite);
		curl_easy_setopt(pUrl->m_pCurl, CURLOPT_WRITEDATA, pUrl);
		curl_multi_add_handle(pUrl->m_pCurlm, pUrl->m_pCurl);
		pUrl->m_eMultiCode = curl_multi_perform(pUrl->m_pCurlm, &pUrl->m_nStillRunning);
		if (!m_sUrl.insert(pUrl).second)
		{
			delete pUrl;
			return nullptr;
		}
		return pUrl;
	}
	void Cleanup();
	u32 GetCount() const;
	void Perform();
	static bool Initialize();
	static void Finalize();
private:
	unordered_set<CUrl*> m_sUrl;
	static bool s_bInitialized;
};

#endif	// URL_MANAGER_H_
