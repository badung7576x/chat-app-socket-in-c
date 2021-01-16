#include <arpa/inet.h>
#include <crypt.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "constant.h"

typedef struct {
    struct sockaddr_in addr;
    int connfd;
    int uid;
    char name[32];
    int currentRoomId;
} client_t;

typedef struct {
    int roomId;
    char name[32];
    client_t *clients[10];
    int userCount;
    char *publicStream;
} room_t;

int sendResponse(int connfd);
char *splitMessage(char *message);
int checkRoomName(char *roomName);
int getRoomIndexByRoomId(int roomId);
void messageToRoom(int roomIndex);
void messageToRoomExceptSender(client_t *client);
int checkUserInRoom(int roomId, char *name);
void getOnlineUsers(int roomIndex, char *list);
void handleGetOnlineMember(client_t *client);
void updateOnlineUserToAll(int roomIndex);
void handleUserExitRoom(client_t *client);
void handleNewUserJoinMessage(client_t *client);
void handleCreateRoom(client_t *client, char *message);
int handleJoinRoom(client_t *client, char *message);
void handlePublicMessage(client_t *client, char *message);
void handleGetStream(client_t *client);
void handlePrivateMessage(client_t *client, char *message);
int handleMessage(void *arg);
client_t *initNewClient(struct sockaddr_in clientAdd, int connfd);
void createRoom(client_t *client, char *roomName);
void addUserToRoom(int roomId, client_t *client);
void removeUserToRoom(int roomIndex, client_t *client);
void updateRoomId(client_t *client, int roomId);
void removeRoom(int roomIndex);
int createServer();