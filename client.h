#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "constant.h"
#include "data.h"

char *push(char *str);
int pop(char *str);
char *splitMessage(char *message);
void clearBuffer(char *buff);
char *getStream(char *channel);
int sendRequest();
int createClient(int argc, char *argv[]);
void signal_SIGABRT(int signal);
gboolean loopHandeResponse();
void signio_handler(int signo);
void handleReponse(char *buff, int n);
void handleGetStream(char *message);
int handleOnlineUsersList(char *message);
int handlePublicMessage(char *message);
int handlePrivateMessage(char *message);
int notifyMessageCount(char *sender);
int findUserMessageStream(char *sender);
char *saveToUserMessageStream(char *sender, char *message);
int handleJoinRoomResponse(char *_message);
int handleCreateRoomResponse(char *_message);
