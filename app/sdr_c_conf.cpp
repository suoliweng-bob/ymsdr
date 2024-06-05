#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "sdr_func.h" 
#include "sdr_c_conf.h" 

//静态成员赋值
CConfig *CConfig::m_instance = NULL;

//构造函数
CConfig::CConfig()
{		
}

//析构函数
CConfig::~CConfig()
{    
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_configItemList.begin(); pos != m_configItemList.end(); ++pos)
	{		
		delete (*pos);
	}
	m_configItemList.clear(); 
    return;
}

//装载配置文件
bool CConfig::Load(const char *pconfName) 
{   
    FILE *fp;
    fp = fopen(pconfName,"r");
    if(fp == NULL)
        return false;

    //每一行配置文件,保持<500字符内
    char  linebuf[501]; 
    
    //文件打开成功 
    while(!feof(fp)) 
    {    
        if(fgets(linebuf,500,fp) == NULL) 
            continue;

        if(linebuf[0] == 0)
            continue;

        //处理注释行
        if(*linebuf==';' || *linebuf==' ' || *linebuf=='#' || *linebuf=='\t'|| *linebuf=='\n')
			continue;
        
    lblprocstring:
        //若有换行，回车，空格等都截取掉
		if(strlen(linebuf) > 0)
		{
			if(linebuf[strlen(linebuf)-1] == 10 || linebuf[strlen(linebuf)-1] == 13 || linebuf[strlen(linebuf)-1] == 32) 
			{
				linebuf[strlen(linebuf)-1] = 0;
				goto lblprocstring;
			}		
		}
        if(linebuf[0] == 0)
            continue;
        if(*linebuf=='[') 
			continue;

        char *ptmp = strchr(linebuf,'=');
        if(ptmp != NULL)
        {
            LPCConfItem p_confitem = new CConfItem;                    
            memset(p_confitem,0,sizeof(CConfItem));
            strncpy(p_confitem->itemname,linebuf,(int)(ptmp-linebuf)); 
            strcpy(p_confitem->itemcontent,ptmp+1);             

            Rtrim(p_confitem->itemname);
			Ltrim(p_confitem->itemname);
			Rtrim(p_confitem->itemcontent);
			Ltrim(p_confitem->itemcontent);
     
            m_configItemList.push_back(p_confitem);  
        } 
    } 

    fclose(fp); 
    return true;
}

//根据ItemName获取配置信息字符串
const char *CConfig::GetString(const char *pitemName)
{
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_configItemList.begin(); pos != m_configItemList.end(); ++pos)
	{	
		if(strcasecmp( (*pos)->itemname,pitemName) == 0)
			return (*pos)->itemcontent;
	}//end for
	return NULL;
}
//根据ItemName获取数字类型配置信息
int CConfig::GetIntDefault(const char *pitemName,const int def)
{
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_configItemList.begin(); pos !=m_configItemList.end(); ++pos)
	{	
		if(strcasecmp( (*pos)->itemname,pitemName) == 0)
			return atoi((*pos)->itemcontent);
	}//end for
	return def;
}
