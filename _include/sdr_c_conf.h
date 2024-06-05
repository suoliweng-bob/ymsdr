
#ifndef __SDR_CONF_H__
#define __SDR_CONF_H__

#include <vector>

#include "sdr_global.h"

class CConfig
{
private:
	CConfig();
public:
	~CConfig();
private:
	static CConfig *m_instance;
	
public:
	static CConfig* GetInstance()
	{
		if(m_instance == NULL)
		{
			if(m_instance == NULL)
			{
				m_instance = new CConfig();
				static CGarhuishou cl;
			}
		}
		return m_instance;
	}
	class CGarhuishou	// 用于释放对象
	{
	public:
		~CGarhuishou()
		{
			if(CConfig::m_instance)
			{
				delete CConfig::m_instance;
				CConfig::m_instance = NULL;
			}
		}

	};

public:
	//装载配置文件
	bool Load(const char *pconfName);
	const char *GetString(const char *pitemName);
	int GetIntDefault(const char *pitemName, const int idef);

public:
	std::vector<LPCConfItem> m_configItemList;
};

#endif