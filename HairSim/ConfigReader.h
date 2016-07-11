/****************************************************************************
*   ԭ��:  ����
*   ����:  2013-4-14
*   Ŀ��:  ��ȡ�����ļ�����Ϣ����string����ʽ����
*   Ҫ��:  �����ļ��ĸ�ʽ����#��Ϊ��ע�ͣ����õ���ʽ��key = value���м���пո�Ҳ��û�пո�
*****************************************************************************/
#pragma once
#include <map>
#include <string>
#define COMMENT_CHAR '#'//ע�ͷ�

class ParamDict :
	public std::map<std::string, std::string>
{
public:
};

class ConfigReader
{
	typedef std::string string;
private:
    std::ifstream *infile = nullptr;
	string fileName;

public:
    //����filename�������ļ�������
    ConfigReader(const string & filename);
    ~ConfigReader(void){ if (infile) delete infile; }

    //����name�������������
    //����ֵ����Ӧ������name��valueֵ
    string getValue(const string & name);
    void getParamDict(ParamDict& dict);

    void close();
};

