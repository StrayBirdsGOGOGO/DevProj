
#ifndef __MD5_INCLUDED__
#define __MD5_INCLUDED__

//MD5ժҪֵ�ṹ��
typedef struct MD5VAL_STRUCT
{
	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int d;
} MD5VAL;

//�����ַ�����MD5ֵ(����ָ���������ɺ�������)
MD5VAL md5(char * str, unsigned int size=0);
unsigned int conv(unsigned int a);
 char*  GetMD5(char * str, unsigned int size=0);

#endif

