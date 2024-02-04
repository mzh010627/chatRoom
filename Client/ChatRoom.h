#ifndef __CHAT_ROOM_H__
#define __CHAT_ROOM_H__


/* 聊天室初始化 */
int ChatRoomInit();

/* 聊天室退出 */
int ChatRoomExit();

/* 聊天室注册 */
int ChatRoomRegister(int sockfd);

/* 聊天室登录 */
int ChatRoomLogin(int sockfd);

/* 添加好友 */
int ChatRoomAddFriend(int sockfd, const char *name);

/* 显示好友 */
int ChatRoomShowFriend(int sockfd, const char *name);

/* 删除好友 */
int ChatRoomDelFriend(int sockfd, const char *name);

/* 私聊 */
int ChatRoomPrivateChat(int sockfd, const char *name);

/* 发起群聊 */
int ChatRoomAddGroupChat(int sockfd, const char *name);

/* 群聊 */
int ChatRoomGroupChat(int sockfd, const char *name);

/* 退出群聊 */
int ChatRoomExitGroupChat(int sockfd, const char *name);


#endif // __CHAT_ROOM_H__