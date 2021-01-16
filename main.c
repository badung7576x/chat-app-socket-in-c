#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "constant.h"
#include "gui.h"

GtkWidget *window = NULL;
GtkWidget *frame;
GtkWidget *chatArea;
GtkWidget *messageInput;

char *you;
char *currentChannel = PUBLIC;
char *publicStream;
int onlineUserCount = 0;
char onlineUsers[MAX_CLIENTS][32];
User onlineUsersStream[MAX_CLIENTS];

int main(int argc, char *argv[]) {
    publicStream = (char *)malloc(1024 * MAXLINE);
    you = (char *)malloc(32);
    memset(onlineUsersStream, 0, MAX_CLIENTS * sizeof(User));
    if (!g_thread_supported()) {
        g_thread_init(NULL);
    }
    gdk_threads_init();
    gdk_threads_enter();

    createClient(argc, argv);
    g_timeout_add(100, (GSourceFunc)loopHandeResponse, NULL);
    gtk_init(&argc, &argv);
    initMainWindow();
    showCreateOrJoinDialog();

    gtk_main();
    gdk_threads_leave();
    return 0;
}
