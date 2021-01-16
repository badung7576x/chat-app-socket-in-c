#include "event.h"

extern GtkWidget *window;                    // from main.c
extern GtkWidget *frame;                     // from main.c
extern GtkWidget *messageInput;              // from main.c
extern char *you;                            // from main.c
extern char *currentChannel;                 // from main.c
extern char *publicStream;                   // from main.c
extern int onlineUserCount;                  // from main.c
extern char onlineUsers[MAX_CLIENTS][32];    // from main.c
extern User onlineUsersStream[MAX_CLIENTS];  // from main.c

extern GtkWidget *userListBox;          // from gui.c
extern GtkWidget *chatArea;             // from gui.c
extern GtkWidget *createOrJoinDialog;   // from gui.c
extern GtkWidget *inputUsername;        // from gui.c
extern GtkWidget *inputRoomName;        // from gui.c
extern GtkWidget *yourNameLabel;        // from gui.c
extern GtkWidget *publicChannelButton;  // from gui.c

extern char *inputBuffer;  // from client.c

char username[32];
char roomName[32];

void clearStreamList() {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (strlen(onlineUsersStream[i].username) > 0) {
            memset(onlineUsersStream[i].username, 0,
                   strlen(onlineUsersStream[i].username));
            free(onlineUsersStream[i].stream);
        }
    }
}

int checkStringHasSpace(char *str) {
    int i = 0, len = strlen(str);
    while (i < len) {
        if (isspace(str[i])) return i;
        i++;
    }
    return -1;
}

char *trimWhiteSpace(char *str) {
    char *end;

    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

void onExitRoomButtonClicked(GtkWidget *widget, gpointer *data) {
    gtk_widget_hide(window);
    clearBuffer(inputBuffer);
    clearStreamList();
    currentChannel = PUBLIC;
    sprintf(inputBuffer, "%c", EXIT_ROOM_ACTION);
    sendRequest();
    showCreateOrJoinDialog();
}

void onCreateRoomSuccess(char *message) {
    runInUIThread(strcpy(you, username));
    runInUIThread(showMessage(createOrJoinDialog, GTK_MESSAGE_WARNING,
                              CREATE_ROOM_SUCCESS, WELLCOME));
    runInUIThread(gtk_entry_set_text(GTK_ENTRY(inputRoomName), BLANK));
    runInUIThread(gtk_entry_set_text(GTK_ENTRY(inputUsername), BLANK));
    runInUIThread(gtk_window_set_title(GTK_WINDOW(window), roomName));
    runInUIThread(showMainWindow());
    runInUIThread(gtk_widget_set_visible(createOrJoinDialog, FALSE));
    runInUIThread(gtk_label_set_text(GTK_LABEL(yourNameLabel), username));
    clearBuffer(inputBuffer);
    sprintf(inputBuffer, "%c", GET_PUBLIC_STREAM);
    sendRequest();
}

void onJoinRoomSuccess(char *message) {
    runInUIThread(strcpy(you, username));
    runInUIThread(gtk_widget_set_visible(createOrJoinDialog, FALSE));
    runInUIThread(gtk_entry_set_text(GTK_ENTRY(inputRoomName), BLANK));
    runInUIThread(gtk_entry_set_text(GTK_ENTRY(inputUsername), BLANK));
    runInUIThread(gtk_window_set_title(GTK_WINDOW(window), roomName));
    runInUIThread(showMainWindow());
    runInUIThread(gtk_label_set_text(GTK_LABEL(yourNameLabel), username));
    clearBuffer(inputBuffer);
    sprintf(inputBuffer, "%c", GET_PUBLIC_STREAM);
    sendRequest();
    updateUserList(onlineUsers, onlineUserCount);
}

void *showBubbleNotify(void *notify) {
    char command[200];
    sprintf(command, "notify-send \"%s\"", notify);
    system(command);
}
void onUserTagged(char *sender) {
    char notify[200];
    sprintf(notify, TAGGED_NOTIFY, sender);
    showBubbleNotify(notify);
}

void onCreateOrJoinFailed(char *message) {
    runInUIThread(showMessage(createOrJoinDialog, GTK_MESSAGE_ERROR,
                              CREATE_ROOM_FAILED, message));
}

void onCreateButtonClicked(GtkWidget *widget, gpointer gp) {
    strcpy(username, trimWhiteSpace(
                         (char *)gtk_entry_get_text(GTK_ENTRY(inputUsername))));
    strcpy(roomName, trimWhiteSpace(
                         (char *)gtk_entry_get_text(GTK_ENTRY(inputRoomName))));

    if (strlen(username) < 1 || strlen(roomName) < 1) {
        showMessage(createOrJoinDialog, GTK_MESSAGE_WARNING, ERROR, NOT_EMPTY);
        return;
    }
    if (checkStringHasSpace(roomName) != -1) {
        showMessage(createOrJoinDialog, GTK_MESSAGE_WARNING, ERROR,
                    ROOM_NAME_INVALID);
        return;
    }
    clearBuffer(inputBuffer);
    sprintf(inputBuffer, "%c%s#%s", CREATE_ROOM_ACTION, username, roomName);
    sendRequest();
}

void onJoinButtonClicked(GtkWidget *widget, gpointer gp) {
    strcpy(username, trimWhiteSpace(
                         (char *)gtk_entry_get_text(GTK_ENTRY(inputUsername))));
    strcpy(roomName, trimWhiteSpace(
                         (char *)gtk_entry_get_text(GTK_ENTRY(inputRoomName))));

    if (strlen(username) < 1 || strlen(roomName) < 1) {
        showMessage(createOrJoinDialog, GTK_MESSAGE_WARNING, ERROR, NOT_EMPTY);
        return;
    }
    if (checkStringHasSpace(roomName) != -1) {
        showMessage(createOrJoinDialog, GTK_MESSAGE_WARNING, ERROR,
                    ROOM_NAME_INVALID);
        return;
    }
    strcpy(you, roomName);
    clearBuffer(inputBuffer);
    sprintf(inputBuffer, "%c%s#%s", JOIN_ROOM_ACTION, username, roomName);
    sendRequest();
}

void onChannelButtonClicked(GtkWidget *widget, gpointer data) {
    int i = 0, count;
    char name[50];
    currentChannel = (char *)data;
    char *x = strchr(currentChannel, BRACKET);
    if (x != NULL) {
        *x = '\0';
    }
    for (i = 0; i < onlineUserCount; i++) {
        sscanf(onlineUsers[i], "%[^(](%d", name, &count);
        if (strcmp(name, currentChannel) == 0) {
            strcpy(onlineUsers[i], currentChannel);
            break;
        }
    }
    if (strcmp(currentChannel, PUBLIC) == 0) {
        setButtonFocus(publicChannelButton, DOWN);
        textViewSetText(chatArea, publicStream);
    } else {
        int id = findUserMessageStream(currentChannel);
        if (id != -1) {
            onlineUsersStream[id].newMessage = 0;
            textViewSetText(chatArea, onlineUsersStream[id].stream);
        }
    }
    updateUserList(onlineUsers, onlineUserCount);
}

void onExit(GtkWidget *widget, gpointer data) { exit(0); }

void onSendButtonClicked(GtkWidget *widget, gpointer data) {
    clearBuffer(inputBuffer);
    char text[255], *entryText;
    entryText = (char *)gtk_entry_get_text(GTK_ENTRY(messageInput));
    strcpy(text, trimWhiteSpace(entryText));

    if (strlen(text) <= 0) return;
    gtk_entry_set_text(GTK_ENTRY(messageInput), BLANK);
    if (strcmp(currentChannel, PUBLIC) == 0)
        sprintf(inputBuffer, "%c%s", CHANNEL_MESSAGE_ACTION, text);
    else {
        char temp[MAXLINE];
        sprintf(temp, "%s:%s", you, text);
        char *xstream = saveToUserMessageStream(currentChannel, temp);
        printf("%s\n", xstream);
        textViewSetText(chatArea, xstream);
        sprintf(inputBuffer, "%c%s#%s", PRIVATE_MESSAGE_ACTION, currentChannel,
                text);
    }
    sendRequest();
}