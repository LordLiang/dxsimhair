/****************************************************************************
*   ԭ��:  ����
*   ����:  2013-4-14
*   Ŀ��:  ��ȡ�����ļ�����Ϣ����string����ʽ����
*   Ҫ��:  �����ļ��ĸ�ʽ����#��Ϊ��ע�ͣ����õ���ʽ��key = value���м���пո�Ҳ��û�пո�
*****************************************************************************/
#pragma once
#include <string>
#define COMMENT_CHAR '#'//ע�ͷ�
using namespace std;

class ConfigReader
{
private:
    ifstream *infile;
public:
    ConfigReader(void){}
    ~ConfigReader(void){}

    //����filename�������ļ�������
    ConfigReader(const string & filename);

    //����name�������������
    //����ֵ����Ӧ������name��valueֵ
    string getValue(const string & name);
};