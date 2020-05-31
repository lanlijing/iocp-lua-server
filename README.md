# iocp-lua-server
一个基于iocp,lua的游戏服务器雏形
1、使用内存池,避免在网络处理时频繁分配释放内存
2、网络封装了iocp,有粘包处理
3、支持lua调试及lua热更新
