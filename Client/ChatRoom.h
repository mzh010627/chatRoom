#ifndef __CHAT_ROOM_H__
#define __CHAT_ROOM_H__


/* 聊天室初始化 */
int ChatRoomOLInit();

/* 聊天室退出 */
int ChatRoomOLExit();

/* 聊天室注册 */
int ChatRoomOLRegister(int sockfd);

/* 聊天室登录 */
int ChatRoomOLLogin(int sockfd);

/* 添加好友 */
int ChatRoomOLAddFriend(int sockfd, const char *name);

/* 显示好友 */
int ChatRoomOLShowFriend(int sockfd);

/* 删除好友 */
int ChatRoomOLDelFriend(int sockfd, const char *name);

/* 私聊 */
int ChatRoomOLPrivateChat(int sockfd, const char *name);

/* 发起群聊 */
int ChatRoomOLAddGroupChat(int sockfd, const char *name);

/* 群聊 */
int ChatRoomOLGroupChat(int sockfd, const char *name);

/* 退出群聊 */
int ChatRoomOLExitGroupChat(int sockfd, const char *name);


#endif // __CHAT_ROOM_H__