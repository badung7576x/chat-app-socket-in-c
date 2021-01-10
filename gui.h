#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "event.h"


void set_size(GtkWidget *gw, int width, int height);
void set_position(GtkWidget *gw, int x, int y);
void showMainWindow();
void initMainWindow();
void showMessage(GtkWidget *parent, GtkMessageType type, char *mms, char *content);
void showCreateOrJoinDialog();
void initCreateOrJoinDialog();
GtkWidget *initMessageInput(int x, int y);
void textViewSetText(GtkWidget *textView, char *text);
GtkWidget *initChatArea(int x, int y);
void initCurrentUserBox(int x, int y);
GtkWidget *initPublicChannelBox(int x, int y);
void delFromUserBox(gpointer child, gpointer user_data);
int setButtonFocus(GtkWidget *button, char *s);
void addButtonToUserListBox(char n[][32], int count);
void updateUserList(char n[][32], int count);
GtkWidget *initUserList(int x, int y, char names[][32], int amount);
