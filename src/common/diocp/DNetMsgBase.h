#ifndef _DNETMSGBASE_H_
#define _DNETMSGBASE_H_

#pragma once

/*
������Ϣ��ͷ��ʽ��
1��4���ֽڵ���Ϣ��ͷ,uint32��
2��4���ֽڵ�������Ϣ������ uint32�� ��������Ϣͷ��16���ֽ����ڣ�
3��4���ֽڵĿͻ��������ʶID��������������Դ�ڿͻ��˵ģ����ĸ��ֽں��ԣ�uint32��
4��4���ֽڵ�������ϢID uint32��
��16���ֽ�
*/
#define MAX_NETMSG_LEN			131072					//������Ϣ����󳤶ȡ�128K
#define NETMSG_HEADER_LEN		16						//������Ϣ��ͷ����
#define NET_MSG_TAG				0xDF698999				//��Ϣ��ͷ



#endif