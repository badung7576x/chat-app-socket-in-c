#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <crypt.h>
#include "constant.h"

char outputBuffer[MAXLINE];
char *publicStream;
int uid = 0;
int roomid = 0;
int client_count = 0;
int room_count = 0;

typedef struct {
    struct sockaddr_in addr;    
    int connfd;                 
    int uid;                    
    char name[32];              
    int currentRoomId;          
} client_t;

typedef struct {
    int roomid;                 
    char name[32];             
    int userids[20];             
    int actualUsers;            
    int roomRemoved;            
} room_t;

client_t *clients[MAX_CLIENTS];
room_t *rooms[MAX_ROOMS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER;

int sendResponse(int connfd) {
    if (strstr(outputBuffer, ACK) == NULL) {
        strcat(outputBuffer, ACK);
    }
    return send(connfd, outputBuffer, strlen(outputBuffer), 0);
}

char *splitMessage(char *message){
    char *split = strchr(message, '#');
    *split = '\0';
    return split + 1;
}

int checkRoomName(char *roomName){
    for (int i = 0; i < room_count; i++) {
        if(!strcmp(roomName, rooms[i]->name) && rooms[i]->roomRemoved != 1){
            return rooms[i]->roomid;
        }
    }
    return NOT_EXIST;
}

void messageToRoom(int roomId){
    for(int i = 0; i < rooms[roomid]->actualUsers; i++){
        if(clients[rooms[roomid]->userids[i]]->currentRoomId == roomid){
            sendResponse(clients[rooms[roomid]->userids[i]]->connfd);
        }
    }
}

int checkUserInRoom(int roomId, char *name) {
    for(int i = 0; i < rooms[roomid]->actualUsers; i++){
        if(strcmp(clients[rooms[roomid]->userids[i]]->name, name) == 0){
            return clients[rooms[roomid]->userids[i]]->connfd;
        }
    }
    return -1;
}

void getOnlineUsers(int roomId, char *list){
    for(int i = 0; i < rooms[roomid]->actualUsers; i++){
        if(clients[rooms[roomid]->userids[i]]->currentRoomId == roomid){
            strcat(list, clients[rooms[roomid]->userids[i]]->name);
            list[strlen(list)] = SEPARATOR;
        }
    }
}

void handleGetOnlineMember(int roomId, int sendTo) {
    memset(outputBuffer, 0, MAXLINE);
    outputBuffer[0] = GET_LIST_USER_ACTION;
    getOnlineUsers(roomId, outputBuffer + 1);
    sendResponse(sendTo);
}

void updateOnlineUserToAll(int roomId) {
    memset(outputBuffer, 0, MAXLINE);
    outputBuffer[0] = GET_LIST_USER_ACTION;
    getOnlineUsers(roomId, outputBuffer + 1);
    messageToRoom(roomId);
}

void handleUserExitRoom(client_t *client) {
    int roomId = client->currentRoomId;
    char temp[200];
    pthread_mutex_lock(&clients_mutex);
    pthread_mutex_lock(&room_mutex);

    rooms[client->currentRoomId]->actualUsers--;
    if(rooms[client->currentRoomId]->actualUsers == 0){
        rooms[client->currentRoomId]->roomRemoved = 1;
    }
    client->currentRoomId =-1;  

    pthread_mutex_unlock(&clients_mutex);
    pthread_mutex_unlock(&room_mutex);
    
    memset(outputBuffer, 0, MAXLINE);
    sprintf(temp, "%s đã rời khỏi phòng chat!", client->name);
    sprintf(outputBuffer, "%c%s#%s", CHANNEL_MESSAGE_ACTION, ">>", temp);
    messageToRoom(client);   
    sprintf(temp, "%s:%s\n", ">>", temp);
    strcat(publicStream, temp);

    updateOnlineUserToAll(roomId);
}

void handleNewUserJoinMessage(client_t *client){
    sleep(1);
    char temp[200];
    memset(outputBuffer, 0, MAXLINE);
    sprintf(temp, "%s đã tham gia phòng chat!", client->name);
    sprintf(outputBuffer, "%c%s#%s", CHANNEL_MESSAGE_ACTION, ">>", temp);
    messageToRoom(client);   
    sprintf(temp, "%s:%s\n", ">>", temp);
    strcat(publicStream, temp);
}

void handleCreateRoom(client_t *client, char *message){
    char *username = message;
    char *roomName = splitMessage(message);
    if (checkRoomName(roomName) == NOT_EXIST) {
        strcpy(client->name, username);
        createRoom(client->uid, roomName);
        sprintf(outputBuffer, "%c%s#%s", CREATE_ROOM_ACTION, SUCCESS, OK);
    } else {
        sprintf(outputBuffer, "%c%s#%s", CREATE_ROOM_ACTION, FAILED, ROOM_EXIST);
    }
    sendResponse(client->connfd);
}

int handleJoinRoom(client_t *client, char *message){
    char *username = message;
    char *roomName = splitMessage(message);
    if(client->currentRoomId == -1){
        printf("CheckRoom\n");
        int roomId = checkRoomName(roomName);
        if(roomId == -1) {
            printf("Fail\n");
            sprintf(outputBuffer, "%c%s#%s", JOIN_ROOM_ACTION, FAILED, JOIN_ROOM_FAILED);
            sendResponse(client->connfd);
            return 0;
        } else {
            printf("Addroom\n");
            addUserToRoom(roomId, client->uid);
            updateRoomId(client, roomId);
            strcpy(client->name, username);
            sprintf(outputBuffer, "%c%s#%s", JOIN_ROOM_ACTION, SUCCESS, OK);
            sendResponse(client->connfd);
            return 1;
        }
    }
    return 0;
}


void handlePublicMessage(client_t *client, char *message){
    char temp[MAXLINE];
    sprintf(temp, "%s:%s\n", client->name , message);
    sprintf(outputBuffer, "%c%s#%s", CHANNEL_MESSAGE_ACTION, client->name, message);
    messageToRoom(client);    
    strcat(publicStream, temp);
}

void handleGetStream(int sendTo) {
    sprintf(outputBuffer, "%c%s", GET_PUBLIC_STREAM, publicStream);
    sendResponse(sendTo);
}

void handlePrivateMessage(client_t *client, char *message) {
    char *sender = client->name;
    char *toUser = message;
    char *mess = splitMessage(message);
    int userConnfd = checkUserInRoom(client, toUser);
    
    if (userConnfd != -1) {
        sprintf(outputBuffer, "%c%s#%s", PRIVATE_MESSAGE_ACTION, sender, mess);
        sendResponse(userConnfd);
    } else {
        printf("Error: No user:{%s}\n", toUser);
    }
}

int handleMessage(void *arg){
    client_t *client = (client_t *)arg;
    int connfd = client->connfd, n;
    char action, *message;
    char readBuffer[MAXLINE];
    while ((n = recv(connfd, readBuffer, MAXLINE, 0)) > 0) {
        readBuffer[n] = '\0';
        if (readBuffer[strlen(readBuffer) - 1] == '\n')
            readBuffer[strlen(readBuffer) - 1] = 0;
        printf("String received from client: %s\n", readBuffer);
        action = readBuffer[0];
        message = readBuffer + 1;
        memset(outputBuffer, 0, MAXLINE);
        switch(action) {
            case CREATE_ROOM_ACTION:
                handleCreateRoom(client, message);
                break;
            case JOIN_ROOM_ACTION:
                if(handleJoinRoom(client, message) == 1){
                    memset(outputBuffer, 0, MAXLINE);
                    updateOnlineUserToAll(client->currentRoomId);
                    handleNewUserJoinMessage(client);
                }
                break;
            case GET_PUBLIC_STREAM:
                handleGetStream(client->connfd);
                break;
            case GET_LIST_USER_ACTION:
                handleGetOnlineMember(client->currentRoomId, client->connfd);
                break;
            case CHANNEL_MESSAGE_ACTION:
                handlePublicMessage(client, message);
                break;
            case PRIVATE_MESSAGE_ACTION:
                handlePrivateMessage(client, message);
                break;
            case EXIT_ROOM_ACTION:
                handleUserExitRoom(client);
                break;
            default:
                puts("\n--------------------UNKNOW ACTION-----------------");
                printf("Send to socket:%d\n", connfd);
        }
    }
}

client_t *initNewClient(struct sockaddr_in clientAdd, int connfd) {
    client_t *client = (client_t *)malloc(sizeof(client_t));
    client->addr = clientAdd;
    client->connfd = connfd;
    client->uid = uid++;
    client->currentRoomId = -1;
    addToClients(client);
    return client;
}

void addToClients(client_t *client){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


void createRoom(int userId, char *roomName){
    pthread_mutex_lock(&room_mutex);
    room_t *rm = (room_t *)malloc(sizeof(room_t));
    strcpy(rm->name, roomName);
    rm->actualUsers = 0;
    rm->actualUsers = 1;
    rm->roomRemoved = 0;
    rm->userids[rm->actualUsers++] = userId;
    rm->roomid = room_count; 
    rooms[room_count++] = rm;
    clients[rm->userids[0]]->currentRoomId = rm->roomid;
    pthread_mutex_unlock(&room_mutex);
}

void addUserToRoom(int roomId, int userId) {
    pthread_mutex_lock(&room_mutex);
    rooms[roomId]->userids[rooms[roomId]->actualUsers] =  userId;
    rooms[roomId]->actualUsers++;
    rooms[roomId]->actualUsers++;
    pthread_mutex_unlock(&room_mutex);
}

void updateRoomId(client_t *client, int roomId) {
    pthread_mutex_lock(&clients_mutex);
    client->currentRoomId = roomId; 
    pthread_mutex_unlock(&clients_mutex);
}

void removeToClients(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}


int createServer(){
    int connfd, listenfd;
    socklen_t clilen;
    pthread_t tid;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) >= 0){
        printf("Server is running at port %d\n", SERV_PORT);
    } else {
        perror("Bind failed");
        return 0;
    }

    listen(listenfd, MAX_CLIENTS);

    printf("%s\n", "Server running...waiting for connections.");

    fflush(stdout);
    while (1){
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd == -1){
            perror("accept");
        } else {
            printf("New connection on socket %d\n", connfd);
            client_t *client  = initNewClient(cliaddr, connfd);
            pthread_create(&tid, NULL, &handleMessage, (void*)client);
        }
    }
}

int main(int argc, char **argv) {
    publicStream = (char *)malloc(1024 * MAXLINE);
    createServer();
}