#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#include "constant.h"
#include "data.h"


char *push(char *str);
int pop(char *str);
char *splitMessage(char *message);
void clearBuffer(char *buff);
char * getStream(char *channel);
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
int findUserMessageStream(char * sender);
char * saveToUserMessageStream(char * sender, char * message);
int handleJoinRoomResponse(char *_message);
int handleCreateRoomResponse(char *_message);
