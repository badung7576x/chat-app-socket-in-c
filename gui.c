#include "gui.h"

extern GtkWidget *window;
extern GtkWidget *frame;
extern GtkWidget *chatArea;
extern GtkWidget *messageInput;

extern char *you;
extern char *currentChannel;
extern char *publicStream;
extern int onlineUserCount;
extern char onlineUsers[MAX_CLIENTS][32];

GtkWidget *userListBox;
GtkWidget *createOrJoinDialog = NULL;
GtkWidget *inputUsername;
GtkWidget *inputRoomName;
GtkWidget *yourNameLabel;
GtkWidget *publicChannelButton;
GtkWidget *userListScroller;
GtkWidget *chatOutputScroller;

GMutex mutex_interface;

void set_size(GtkWidget *gw, int width, int height) {
	gtk_widget_set_size_request(gw, width, height);
}

void set_position(GtkWidget *gw, int x, int y) {
	gtk_fixed_put(GTK_FIXED(frame), gw, x, y);
}

void showMessage(GtkWidget *parent, GtkMessageType type, char *mms, char *content) {
	GtkWidget *mdialog;
	mdialog = gtk_message_dialog_new(
        GTK_WINDOW(parent),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        type,
        GTK_BUTTONS_OK,
        "%s", mms);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(mdialog), "%s", content);
	gtk_widget_show_all(mdialog);
	gtk_dialog_run(GTK_DIALOG(mdialog));
	gtk_widget_destroy(mdialog);
}

void showCreateOrJoinDialog() {
	if (createOrJoinDialog == NULL)
		initCreateOrJoinDialog();
	gtk_widget_show_all(createOrJoinDialog);
	gtk_dialog_run(GTK_DIALOG(createOrJoinDialog));
	return;
}

void initCreateOrJoinDialog() {
	createOrJoinDialog = gtk_dialog_new_with_buttons(APP_TITLE, GTK_WINDOW(window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL, NULL);

	GtkWidget *dialog_ground = gtk_fixed_new();
	GtkWidget *uframe = gtk_frame_new(USERNAME);
	GtkWidget *rframe = gtk_frame_new(ROOMNAME);
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *createButton = gtk_button_new_with_label(CREATE_ROOM);
	GtkWidget *joinButton = gtk_button_new_with_label(JOIN_ROOM);
	inputUsername = gtk_entry_new();
	inputRoomName = gtk_entry_new();

	gtk_box_pack_start(GTK_BOX(box), createButton, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), joinButton, TRUE, TRUE, 2);

	set_size(uframe, 300, 50);
	set_size(rframe, 300, 50);
	set_size(box, 300, 50);
	set_size(createButton, 100, 30);
	set_size(joinButton, 100, 30);

	gtk_widget_set_margin_start(inputUsername, 2);
	gtk_widget_set_margin_end(inputUsername, 2);
	gtk_widget_set_margin_start(inputRoomName, 2);
	gtk_widget_set_margin_end(inputRoomName, 2);

	gtk_fixed_put(GTK_FIXED(dialog_ground), uframe, 0, 20);
	gtk_fixed_put(GTK_FIXED(dialog_ground), rframe, 0, 80);
	gtk_fixed_put(GTK_FIXED(dialog_ground), box, 0, 220);

	gtk_container_add(GTK_CONTAINER(uframe), inputUsername);
	gtk_container_add(GTK_CONTAINER(rframe), inputRoomName);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(createOrJoinDialog))), dialog_ground, TRUE, TRUE, 0);

	GtkWidget *data_array[3];
	data_array[0] = inputUsername;
	data_array[1] = inputRoomName;
	data_array[2] = createOrJoinDialog;
	g_signal_connect(createButton, "clicked", G_CALLBACK(onCreateButtonClicked), data_array);
	g_signal_connect(joinButton, "clicked", G_CALLBACK(onJoinButtonClicked), data_array);
	g_signal_connect(createOrJoinDialog, "destroy", G_CALLBACK(onExit), NULL);
}

GtkWidget *initMessageInput(int x, int y) {
	GtkWidget *inputGroupBox;
	GtkWidget *inputBox;
	GtkWidget *sendButton;

	inputGroupBox = gtk_frame_new(TEXT_INPUT_LABEL);
	set_size(inputGroupBox, 450, 60);
	set_position(inputGroupBox, x, y);

	inputBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(inputGroupBox), inputBox);

	messageInput = gtk_entry_new();
	set_size(messageInput, 388, 20);

	sendButton = gtk_button_new_with_label(SEND_LABEL);
	gtk_widget_set_margin_bottom(sendButton, 5);
	gtk_widget_set_margin_top(sendButton, 0);

	gtk_box_pack_start(GTK_BOX(inputBox), messageInput, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(inputBox), sendButton, TRUE, TRUE, 5);

	g_signal_connect(sendButton, "clicked", G_CALLBACK(onSendButtonClicked), NULL);
	// g_signal_connect(messageInput, "activate", G_CALLBACK(onSendButtonClicked), NULL);
	return messageInput;
}

void textViewSetText(GtkWidget *textView, char *text) {
	char *x, *q;
	GtkTextBuffer *t_buffer;
	GtkTextIter start;
	GtkTextIter end;
	t_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
	if (t_buffer == NULL){
		t_buffer = gtk_text_buffer_new(NULL);
	}
	gtk_text_buffer_set_text(t_buffer, text, -1);
	int i, lineCount = gtk_text_buffer_get_line_count(t_buffer);
	GtkTextTag *tag = gtk_text_buffer_create_tag(t_buffer, NULL,
        "foreground", "blue",
        "weight", PANGO_WEIGHT_BOLD, NULL);
	for (i = 0; i < lineCount - 1; ++i){
		gtk_text_buffer_get_iter_at_line(t_buffer, &start, i);
		gtk_text_buffer_get_end_iter(t_buffer, &end);
		x = gtk_text_buffer_get_text(t_buffer, &start, &end, TRUE);
		q = strchr(x, ':');
		if (q > x &&  q < x + strlen(x)){
			gtk_text_buffer_get_iter_at_line_index(t_buffer, &end, i, q - x);
			gtk_text_buffer_apply_tag(t_buffer, tag, &start, &end);
		}
	}

	gtk_text_view_set_buffer(GTK_TEXT_VIEW(textView), t_buffer);
	GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(chatOutputScroller));
	gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
	gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(chatOutputScroller), adj);
}

GtkWidget *initChatArea(int x, int y) {
	GtkWidget *outputBox;

	outputBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	set_size(outputBox, 450, 280);
	set_position(outputBox, x, y);

	chatArea = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(chatArea), GTK_WRAP_WORD_CHAR);

	chatOutputScroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(chatOutputScroller), chatArea);
	gtk_box_pack_start(GTK_BOX(outputBox), chatOutputScroller, TRUE, TRUE, 2);
	return chatArea;
}

void initCurrentUserBox(int x, int y) {
	GtkWidget *currentUserGroupBox;
	GtkWidget *exitRoomButton;
	GtkWidget *containBox;
	currentUserGroupBox = gtk_frame_new(YOU);
	containBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(currentUserGroupBox), containBox);

	set_size(currentUserGroupBox, 115, 64);
	set_position(currentUserGroupBox, x, y);
	yourNameLabel = gtk_label_new(you);

	exitRoomButton = gtk_button_new_with_label(EXIT_ROOM);
	set_size(exitRoomButton, 110, 34);

	gtk_box_pack_start(GTK_BOX(containBox), yourNameLabel, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(containBox), exitRoomButton, TRUE, TRUE, 0);

	g_signal_connect(exitRoomButton, "clicked", G_CALLBACK(onExitRoomButtonClicked), NULL);
}

GtkWidget *initPublicChannelBox(int x, int y) {
	GtkWidget *publicChannelGroupBox;
	publicChannelGroupBox = gtk_frame_new(PUBLIC);
	set_size(publicChannelGroupBox, 115, 54);
	set_position(publicChannelGroupBox, x, y);

	publicChannelButton = gtk_button_new_with_label(PUBLIC);
	gtk_container_add(GTK_CONTAINER(publicChannelGroupBox), publicChannelButton);

	g_signal_connect(publicChannelButton, "clicked", G_CALLBACK(onChannelButtonClicked), PUBLIC);
	return publicChannelButton;
}

void delFromUserBox(gpointer child, gpointer user_data) {
	gtk_container_remove(GTK_CONTAINER(userListBox), (GtkWidget *)child);
}

int setButtonFocus(GtkWidget *button, char *s) {
	GdkRGBA color;
	gdk_rgba_parse(&color, s);
	gtk_widget_override_background_color(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
	return 0;
}

void addButtonToUserListBox(char n[][32], int count) {
	int i = 0;
	if (strcmp(PUBLIC, currentChannel) == 0) {
		setButtonFocus(publicChannelButton, DOWN);
	} else {
		setButtonFocus(publicChannelButton, UP);
	}
	for (i = 0; i < count; ++i) {
		if (strcmp(n[i], you) != 0){
			GtkWidget *userIndex = gtk_button_new_with_label(n[i]);
			if (strcmp(n[i], currentChannel) == 0) {
				setButtonFocus(userIndex, DOWN);
			}
			gtk_box_pack_end(GTK_BOX(userListBox), userIndex, TRUE, TRUE, 0);
			g_signal_connect(userIndex, "clicked", G_CALLBACK(onChannelButtonClicked), n[i]);
		}
	}
	gtk_widget_show_all(userListBox);
}

void updateUserList(char n[][32], int count) {
	GList *childs = gtk_container_get_children(GTK_CONTAINER(userListBox));
	g_list_foreach(childs, delFromUserBox, NULL);
	addButtonToUserListBox(n, count);
}
GtkWidget *initUserList(int x, int y, char names[][32], int amount) {
	GtkWidget *userListGroupBox;

	userListGroupBox = gtk_frame_new(ONLINE_LIST_LABEL);
	set_size(userListGroupBox, 115, 186);
	set_position(userListGroupBox, x, y);

	userListBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	userListScroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(userListScroller), userListBox);

	gtk_container_add(GTK_CONTAINER(userListGroupBox), userListScroller);
	addButtonToUserListBox(names, amount);

	return userListGroupBox;
}

void initMainWindow(){
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(window), APP_TITLE);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	frame = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(window), frame);
	gtk_widget_set_margin_bottom(frame, 5);
	gtk_widget_set_margin_end(frame, 5);

	initCurrentUserBox(5, 4);
	initPublicChannelBox(5, 94);
	initChatArea(120, 10);

	initUserList(5, 154, onlineUsers, 0);
	initMessageInput(120, 280);

	g_signal_connect(window, "destroy", G_CALLBACK(onExit), NULL);
}

void showMainWindow() {
	if (window == NULL) {
		initMainWindow();
	}
	gtk_widget_show_all(window);
	gtk_entry_grab_focus_without_selecting(GTK_ENTRY(messageInput));
}