#ifndef _DNETMSGBASE_H_
#define _DNETMSGBASE_H_

#pragma once

/*
网络消息包头格式：
1、4个字节的消息包头,uint32型
2、4个字节的网络消息包长度 uint32型 （包括消息头的16个字节在内）
3、4个字节的客户端网络标识ID（如果网络包不是源于客户端的，这四个字节忽略）uint32型
4、4个字节的网络消息ID uint32型
共16个字节
*/
#define MAX_NETMSG_LEN			131072					//网络消息的最大长度。128K
#define NETMSG_HEADER_LEN		16						//网络消息包头长度
#define NET_MSG_TAG				0xDF698999				//消息包头



#endif