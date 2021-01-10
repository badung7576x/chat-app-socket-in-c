#include "client.h"

int clientSocket;
char *inputBuffer;
GMutex queueMutex;
char *bufferQueue[MAX_CLIENTS];

int queueSize = 0;

extern char *you;                                       // from main.c
extern char *currentChannel;                            // from main.c
extern char *publicStream;                              // from main.c
extern int onlineUserCount;                             // from main.c
extern char onlineUsers[MAX_CLIENTS][32];               // from main.c
extern User onlineUsersStream[MAX_CLIENTS];

extern GtkWidget *chatArea;                             // from gui.c
extern void updateUserList(char n[][32], int);
extern void textViewSetText(GtkWidget *, char *);
extern void *showBubbleNotify(void *);

char *push(char *str) {
    g_mutex_lock(&queueMutex);
    bufferQueue[queueSize] = strdup(str);
    printf("PUSH ==> %s\n", str);
    queueSize += 1;
    g_mutex_unlock(&queueMutex);
    return bufferQueue[queueSize];
}

int pop(char *str) {
    g_mutex_lock(&queueMutex);
    int current = queueSize;
    if (queueSize > 0) {
        queueSize--;
        strcpy(str, bufferQueue[queueSize]);
        printf("POP ==> %s\n", str);
        free(bufferQueue[queueSize]);
    }
    g_mutex_unlock(&queueMutex);
    return current;
}

char * getStream(char *channel) {
    int i = 0;
    for(;i < MAX_CLIENTS; i++){
        if(strcmp(channel, onlineUsersStream[i].username) == 0){
            return onlineUsersStream[i].stream;
        }
    }
    return NULL;
}

void clearBuffer(char *buff) {
    memset(buff, 0, strlen(buff));
}

int sendRequest() {
    int n = strlen(inputBuffer);
    puts("\n|");
    printf("\n+--->Send to server :{%s}\n", inputBuffer);
    printf("\n--------------->Sent %d bytes:<-----------------\n\n", n);
    return send(clientSocket, inputBuffer, n, 0);
}

char *splitMessage(char *message) {
    char *split = strchr(message, '#');
    *split = '\0';
    return split + 1;
}

int handleCreateRoomResponse(char *_message) {
    char *value = _message;
    _message = splitMessage(_message);
    printf("Value:%s\nmessage:%s\n", value, _message);
    if (strcmp(value, SUCCESS) == 0) {
        onCreateRoomSuccess(_message);
    } else if (strcmp(value, FAILED) == 0) {
        onCreateOrJoinFailed(_message);
    }
    return 0;
}

int handleJoinRoomResponse(char *_message) {
    char *value = _message;
    _message = splitMessage(_message);
    printf("Value:%s\nmessage:%s\n", value, _message);
    if (strcmp(value, SUCCESS) == 0) {
        onJoinRoomSuccess(_message);
    } else if (strcmp(value, FAILED) == 0) {
        onCreateOrJoinFailed(_message);
    }
    return 0;
}

char * saveToUserMessageStream(char * sender, char * message) {
    int i, found = 0;
    char temp[MAXLINE];
    for (i = 0; i< MAX_CLIENTS; i++){
        if(strcmp(onlineUsersStream[i].username, sender) == 0){ 
            printf("Find user at %d\n", i);
            printf("Current stream: %s\n", onlineUsersStream[i].stream);
            found = 1;
        }else if(onlineUsersStream[i].username[0] == '\0'){
            printf("User not found created at %d\n", i);
            strcpy(onlineUsersStream[i].username, sender);
            onlineUsersStream[i].stream = (char *)calloc(69 * MAXLINE , sizeof(char));
            found = 1;
        }
        if (found){
            sprintf(temp, "%s\n", message);
            strcat(onlineUsersStream[i].stream, temp);
            return onlineUsersStream[i].stream;
        }
    }
    return sender;
}

int findUserMessageStream(char * sender) {
    int i, found = 0;
    for (i = 0; i< MAX_CLIENTS; i++){
        if(strcmp(onlineUsersStream[i].username, sender) == 0){ 
            printf("Find user at %d\n", i);
            printf("Current stream: %s\n", onlineUsersStream[i].stream);
            found = 1;
        }else if(onlineUsersStream[i].username[0] == '\0'){
            printf("User not found created at %d\n", i);
            strcpy(onlineUsersStream[i].username, sender);
            onlineUsersStream[i].stream = (char *)calloc(69 * MAXLINE, sizeof (char));
            found = 1;
        }

        if (found) {
            return i;
        }
    }
    return -1;
}

int notifyMessageCount(char *sender) {
    int i, count = 0;
    char name[32];
        
    for(i = 0; i< onlineUserCount; i++){
        count = 0;
        sscanf(onlineUsers[i], "%[^(](%d", name, &count);
        if (strcmp(name, sender) == 0){
            sprintf(onlineUsers[i], "%s(%d)", name, count+1);
            puts(onlineUsers[i]);
            int id = findUserMessageStream(name);
            if (id != -1){
                onlineUsersStream[i].newMessage = count+1;
            }
            break;
        }
    }
    updateUserList(onlineUsers, onlineUserCount);
}

int handlePrivateMessage(char *message) {
    printf("Private message: %s\n", message);
    char *sender = message;
    message = splitMessage(message);
    char temp[MAXLINE];
    
    sprintf(temp, "%s:%s", sender, message);
    char * xstream =  saveToUserMessageStream(sender, temp);
    if (strcmp(currentChannel, sender) == 0){
        textViewSetText(chatArea, xstream);
    }else{
        notifyMessageCount(sender);
    }
    return 0;
}


int handlePublicMessage(char *message) {
    char *sender = message;
    message = splitMessage(message);
    char temp[MAXLINE];
    sprintf(temp, "%s:%s\n", sender, message);
    strcat(publicStream, temp);
    if (strcmp(currentChannel, PUBLIC) == 0) {
        textViewSetText(chatArea, publicStream);
    }
    sprintf(temp, "@%s", you);
    if (strstr(message, temp)){
        onUserTagged(sender);
    }
    return 0;
}

int handleOnlineUsersList(char *message) {
    int i, j = 0;
    int messageLength = strlen(message);
    onlineUserCount = 0;
    printf("User lists: %s\n", message);
    for (i = 0; i < messageLength; i++) {
        if (message[i] == SEPARATOR) {
            message[i] = '\0';
            strcpy(onlineUsers[onlineUserCount], message + j);
            int id = findUserMessageStream(onlineUsers[onlineUserCount]);
            if (id != -1) {
                if (onlineUsersStream[id].newMessage) {
                    char temp[10];
                    sprintf(temp, "(%d)", onlineUsersStream[id].newMessage);
                    strcat(onlineUsers[onlineUserCount], temp);
                }
            }
            j = i + 1;
            onlineUserCount++;
        }
    }
    updateUserList(onlineUsers, onlineUserCount);
    return messageLength;
}

void handleGetStream(char *message) {
    strcpy(publicStream, message);
    strcat(publicStream, "\n");
    textViewSetText(chatArea, publicStream);
}
void handleReponse(char *buff, int n){
    buff[n] = '\0';
    printf("\n------------->Received %d bytes:<--------------\n\n", n);
    char action, *message;
    if (buff[strlen(buff) - 1] == '\n')
        buff[strlen(buff) - 1] = '\0';
    action = buff[0];
    message = buff + 1;
    
    switch (action) {
        case CREATE_ROOM_ACTION:
            handleCreateRoomResponse(message);
            break;
        case JOIN_ROOM_ACTION:
            handleJoinRoomResponse(message);
            break;
        case GET_LIST_USER_ACTION:
            handleOnlineUsersList(message);
            break;
        case PRIVATE_MESSAGE_ACTION:
            handlePrivateMessage(message);
            break;
        case CHANNEL_MESSAGE_ACTION:
            handlePublicMessage(message);
            break;
        case GET_PUBLIC_STREAM:
            if (strlen(message) > 1) {
                handleGetStream(message);
            }
            break;
    }
}
void signio_handler(int signo){
    char buffer[MAXLINE];
    clearBuffer(buffer);
    char * ackIndex, * bufferStart;
    int n = recv(clientSocket, buffer, MAXLINE, 0);
    if (n > 0) {
        bufferStart = buffer;
        char *end = bufferStart + strlen(bufferStart);
        do{
            ackIndex = strstr(bufferStart, ACK);
            if (ackIndex != NULL) {   
                *ackIndex = '\0';
                push(bufferStart);
                bufferStart = ackIndex + 1;
            }
        }while(ackIndex != NULL && bufferStart < end);  
    } else {
        puts("Error when handle receive data!");
    }
}

gboolean loopHandeResponse() {
    char newBuffer[MAXLINE]={};
    if (pop(newBuffer) > 0) handleReponse(newBuffer, strlen(newBuffer));
    return TRUE;
}

int createClient(int argc, char *argv[]) {

    inputBuffer = (char *)malloc(MAXLINE * sizeof(char));
    
    struct sockaddr_in server_socket;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    server_socket.sin_family = AF_INET;
    server_socket.sin_port = htons(SERV_PORT);
    server_socket.sin_addr.s_addr = inet_addr(argc > 1?argv[1] : "127.0.0.1");
    printf("Server IP = %s ", inet_ntoa(server_socket.sin_addr));

    if (connect(clientSocket, (struct sockaddr *) &server_socket, sizeof(server_socket)) < 0){
        char * errorMessage = "Error in connecting to server\n";
        showBubbleNotify(errorMessage);
    }

    if (fcntl(clientSocket, F_SETFL, O_NONBLOCK | O_ASYNC))
        printf("Error in setting socket to async, nonblock mode");

    signal(SIGIO, signio_handler);

    if (fcntl(clientSocket, F_SETOWN, getpid()) < 0)
        printf("Error in setting own to socket");

    return 0;
}