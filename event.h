#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "constant.h"
#include "client.h"

#define runInUIThread(x) \
	gdk_threads_enter(); \
	x;                   \
	gdk_threads_leave();


void clearStreamList();
void onExitRoomButtonClicked(GtkWidget *widget, gpointer *data);
void onCreateRoomSuccess(char *message);
void onJoinRoomSuccess(char *message);
void *showBubbleNotify(void *notify);
void onUserTagged(char * sender);
void onCreateOrJoinFailed(char *message);
void onCreateButtonClicked(GtkWidget *widget, gpointer gp);
void onJoinButtonClicked(GtkWidget *widget, gpointer gp);
void onChannelButtonClicked(GtkWidget *widget, gpointer data);
void onExit(GtkWidget *widget, gpointer data);
void onSendButtonClicked(GtkWidget *widget, gpointer data);