#ifndef DISCORD_WINDOW_H
#define DISCORD_WINDOW_H

#include <gtk/gtk.h>
#include "auth.h"

// Structure to hold Discord window data
typedef struct
{
  GtkWidget *window;
  GtkWidget *messages_text_view;
  GtkWidget *channels_list;
  GtkWidget *users_list;
  GtkWidget *message_entry;
  GtkTextBuffer *messages_buffer;
  int user_id;
  char *username;
  int current_channel_id;
} DiscordWindow;

// Function to create and show the main Discord window
DiscordWindow *create_discord_window(int user_id, const char *username);

// Function to destroy the Discord window and cleanup resources
void destroy_discord_window(DiscordWindow *discord);

// Function to add a new message to the chat
void add_message_to_chat(DiscordWindow *discord_window, const char *sender, const char *message);

// Function to update the users list
void update_users_list(DiscordWindow *discord_window, const char **users, int user_count);

// Function to update the channels list
void update_channels_list(DiscordWindow *discord_window, const char **channels, int channel_count);

#endif // DISCORD_WINDOW_H