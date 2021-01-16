#include "server.h"

char buffer[MAXLINE];
int uid = 0;
int roomid = 0;
int room_count = 0;

room_t *rooms[MAX_ROOMS];

pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) { createServer(); }

int sendResponse(int connfd) {
    if (strstr(buffer, ACK) == NULL) strcat(buffer, ACK);
    return send(connfd, buffer, strlen(buffer), 0);
}

char *splitMessage(char *message) {
    char *split = strchr(message, '#');
    *split = '\0';
    return split + 1;
}

void debug(char *funcName, char *str) {
    printf("%s debug: %s \n", funcName, str);
}

void showRoom() {
    for (int i = 0; i < room_count; i++) {
        printf("%d. Room %s (%d)\n", i + 1, rooms[i]->name,
               rooms[i]->userCount);
        for (int j = 0; j < rooms[i]->userCount; j++) {
            printf("\t%d. Client %s\n", j + 1, rooms[i]->clients[j]->name);
        }
        printf("Room chat: %s\n", rooms[i]->publicStream);
    }
}

int checkRoomName(char *roomName) {
    for (int i = 0; i < room_count; i++) {
        if (!strcmp(roomName, rooms[i]->name)) {
            return rooms[i]->roomId;
        }
    }
    return NOT_EXIST;
}

int getRoomIndexByRoomId(int roomId) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i]->roomId == roomId) return i;
    }
    return NOT_EXIST;
}

void messageToRoom(int roomIndex) {
    for (int i = 0; i < rooms[roomIndex]->userCount; i++) {
        sendResponse(rooms[roomIndex]->clients[i]->connfd);
    }
}

void messageToRoomExceptSender(client_t *client) {
    int roomIndex = getRoomIndexByRoomId(client->currentRoomId);
    if (roomIndex == NOT_EXIST) return;
    for (int i = 0; i < rooms[roomIndex]->userCount; i++) {
        if (rooms[roomIndex]->clients[i]->uid != client->uid)
            sendResponse(rooms[roomIndex]->clients[i]->connfd);
    }
}

int checkUserInRoom(int roomId, char *name) {
    int roomIndex = getRoomIndexByRoomId(roomId);
    if (roomIndex == NOT_EXIST) return;
    for (int i = 0; i < rooms[roomIndex]->userCount; i++) {
        if (strcmp(rooms[roomIndex]->clients[i]->name, name) == 0) {
            return rooms[roomIndex]->clients[i]->connfd;
        }
    }
    return NOT_EXIST;
}

void getOnlineUsers(int roomIndex, char *list) {
    for (int i = 0; i < rooms[roomIndex]->userCount; i++) {
        strcat(list, rooms[roomIndex]->clients[i]->name);
        list[strlen(list)] = SEPARATOR;
    }
}

void handleGetOnlineMember(client_t *client) {
    memset(buffer, 0, MAXLINE);
    buffer[0] = GET_LIST_USER_ACTION;
    getOnlineUsers(client->currentRoomId, buffer + 1);
    sendResponse(client->connfd);
}

void updateOnlineUserToAll(int roomIndex) {
    memset(buffer, 0, MAXLINE);
    buffer[0] = GET_LIST_USER_ACTION;
    getOnlineUsers(roomIndex, buffer + 1);
    messageToRoom(roomIndex);
}

void handleUserExitRoom(client_t *client) {
    int roomId = client->currentRoomId;
    int roomIndex = getRoomIndexByRoomId(roomId);
    if (roomIndex == NOT_EXIST) return;
    char temp[200];
    removeUserToRoom(roomIndex, client);
    memset(buffer, 0, MAXLINE);
    sprintf(temp, ">>>> %s đã rời khỏi phòng chat! <<<<", client->name);
    sprintf(buffer, "%c%s#%s", CHANNEL_MESSAGE_ACTION, "-", temp);
    messageToRoom(roomIndex);
    strcat(rooms[roomIndex]->publicStream, temp);
    strcat(rooms[roomIndex]->publicStream, "\n");

    updateOnlineUserToAll(roomIndex);
}

void handleNewUserJoinMessage(client_t *client) {
    char temp[200];
    int roomIndex = getRoomIndexByRoomId(client->currentRoomId);
    if (roomIndex == NOT_EXIST) return;
    memset(buffer, 0, MAXLINE);
    sprintf(temp, ">>>> %s đã tham gia phòng chat! <<<<", client->name);
    sprintf(buffer, "%c%s#%s", CHANNEL_MESSAGE_ACTION, "-", temp);
    messageToRoomExceptSender(client);
    strcat(rooms[roomIndex]->publicStream, temp);
    strcat(rooms[roomIndex]->publicStream, "\n");
}

void handleCreateRoom(client_t *client, char *message) {
    if (DEBUG) {
        char debugMess[200];
        sprintf(debugMess, "Handler create room with ", message);
        debug("handleCreateRoom", debugMess);
    }
    char *username = message, temp[100];
    char *roomName = splitMessage(message);
    if (client->currentRoomId == -1) {
        if (checkRoomName(roomName) == NOT_EXIST) {
            strcpy(client->name, username);
            createRoom(client, roomName);
            sprintf(buffer, "%c%s#%s", CREATE_ROOM_ACTION, SUCCESS, OK);
        } else {
            sprintf(temp, ROOM_EXIST, roomName);
            sprintf(buffer, "%c%s#%s", CREATE_ROOM_ACTION, FAILED, temp);
        }
    } else {
        sprintf(buffer, "%c%s#%s", CREATE_ROOM_ACTION, FAILED,
                JOIN_ROOM_FAILED);
    }
    sendResponse(client->connfd);
}

int handleJoinRoom(client_t *client, char *message) {
    char *username = message;
    char *roomName = splitMessage(message);
    if (client->currentRoomId == -1) {
        int roomId = checkRoomName(roomName);
        if (roomId == NOT_EXIST) {
            sprintf(buffer, "%c%s#%s", JOIN_ROOM_ACTION, FAILED,
                    ROOM_NOT_EXIST);
            sendResponse(client->connfd);
            return 0;
        } else {
            strcpy(client->name, username);
            addUserToRoom(roomId, client);
            sprintf(buffer, "%c%s#%s", JOIN_ROOM_ACTION, SUCCESS, OK);
            sendResponse(client->connfd);
            return 1;
        }
    } else {
        sprintf(buffer, "%c%s#%s", JOIN_ROOM_ACTION, FAILED, JOIN_ROOM_FAILED);
        sendResponse(client->connfd);
        return 0;
    }
}

void handlePublicMessage(client_t *client, char *message) {
    char temp[MAXLINE];
    int roomIndex = getRoomIndexByRoomId(client->currentRoomId);
    if (roomIndex == NOT_EXIST) return;
    sprintf(temp, "%s:%s\n", client->name, message);
    sprintf(buffer, "%c%s#%s", CHANNEL_MESSAGE_ACTION, client->name, message);
    messageToRoom(roomIndex);
    strcat(rooms[roomIndex]->publicStream, temp);
}

void handleGetStream(client_t *client) {
    int roomIndex = getRoomIndexByRoomId(client->currentRoomId);
    if (roomIndex == NOT_EXIST) return;
    sprintf(buffer, "%c%s", GET_PUBLIC_STREAM, rooms[roomIndex]->publicStream);
    sendResponse(client->connfd);
}

void handlePrivateMessage(client_t *client, char *message) {
    char *sender = client->name;
    char *toUser = message;
    char *mess = splitMessage(message);
    int userConnfd = checkUserInRoom(client->currentRoomId, toUser);

    if (userConnfd != NOT_EXIST) {
        sprintf(buffer, "%c%s#%s", PRIVATE_MESSAGE_ACTION, sender, mess);
        sendResponse(userConnfd);
    } else {
        printf("Error: No user:{%s}\n", toUser);
    }
}

int handleMessage(void *arg) {
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
        memset(buffer, 0, MAXLINE);
        switch (action) {
            case CREATE_ROOM_ACTION:
                handleCreateRoom(client, message);
                break;
            case JOIN_ROOM_ACTION:
                if (handleJoinRoom(client, message) == 1) {
                    updateOnlineUserToAll(client->currentRoomId);
                    handleNewUserJoinMessage(client);
                }
                break;
            case GET_PUBLIC_STREAM:
                handleGetStream(client);
                break;
            case GET_LIST_USER_ACTION:
                handleGetOnlineMember(client);
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
        showRoom();
    }
    if (n == 0) {
        printf("Client %d hung up\n", client->connfd);
        handleUserExitRoom(client);
    }
    if (n < 0) perror("recv");
}

client_t *initNewClient(struct sockaddr_in clientAdd, int connfd) {
    client_t *client = (client_t *)malloc(sizeof(client_t));
    client->addr = clientAdd;
    client->connfd = connfd;
    client->uid = uid++;
    client->currentRoomId = -1;
    return client;
}

void createRoom(client_t *client, char *roomName) {
    if (DEBUG) {
        char debugMess[200];
        sprintf(debugMess, "Create Room by client socket: %d", client->connfd);
        debug("createRoom", debugMess);
    }

    room_t *rm = (room_t *)malloc(sizeof(room_t));
    strcpy(rm->name, roomName);
    rm->userCount = 0;
    rm->clients[rm->userCount++] = client;
    rm->roomId = roomid++;
    rm->publicStream = (char *)malloc(1024 * MAXLINE);

    pthread_mutex_lock(&room_mutex);
    rooms[room_count++] = rm;
    pthread_mutex_unlock(&room_mutex);

    updateRoomId(client, rm->roomId);
}

void addUserToRoom(int roomId, client_t *client) {
    int roomIndex = getRoomIndexByRoomId(roomId);
    if (roomIndex == NOT_EXIST) return;
    int clientIndex = rooms[roomIndex]->userCount;

    pthread_mutex_lock(&room_mutex);
    rooms[roomIndex]->clients[clientIndex] = client;
    rooms[roomIndex]->userCount++;
    pthread_mutex_unlock(&room_mutex);

    updateRoomId(client, roomId);
}

void removeUserToRoom(int roomIndex, client_t *client) {
    pthread_mutex_lock(&room_mutex);
    int deletePosition = -1;
    for (int i = 0; i < rooms[roomIndex]->userCount; i++) {
        if (rooms[roomIndex]->clients[i]->uid == client->uid) {
            deletePosition = i;
            break;
        }
    }
    if (deletePosition != -1) {
        for (int i = deletePosition; i < rooms[roomIndex]->userCount - 1; i++) {
            rooms[roomIndex]->clients[i] = rooms[roomIndex]->clients[i + 1];
        }
        rooms[roomIndex]->userCount--;
    }

    if (rooms[roomIndex]->userCount <= 0) {
        removeRoom(roomIndex);
    }
    updateRoomId(client, -1);
    pthread_mutex_unlock(&room_mutex);
}

void updateRoomId(client_t *client, int roomId) {
    client->currentRoomId = roomId;
}

void removeRoom(int roomIndex) {
    for (int i = roomIndex; i < room_count - 1; i++) {
        rooms[i] = rooms[i + 1];
    }
    room_count--;
}

int createServer() {
    int connfd, listenfd;
    pthread_t tid;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) >= 0) {
        printf("Server is running at port %d\n", SERV_PORT);
    } else {
        perror("Bind failed");
        return 0;
    }

    listen(listenfd, MAX_CLIENTS);

    printf("%s\n", "Server running...waiting for connections.");

    fflush(stdout);
    while (1) {
        socklen_t clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd == -1) {
            perror("accept");
        } else {
            printf("New connection on socket %d\n", connfd);
            client_t *client = initNewClient(cliaddr, connfd);
            pthread_create(&tid, NULL, &handleMessage, (void *)client);
        }
    }
}