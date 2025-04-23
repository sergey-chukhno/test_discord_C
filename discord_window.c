#include "discord_window.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include "client_auth.h"
#include <json-c/json.h>

// Global variable for the message receiving thread
static pthread_t message_thread;
static bool thread_running = false;

// Structure to pass data to idle functions
typedef struct
{
  DiscordWindow *discord;
  char *sender;
  char *message;
} MessageData;

typedef struct
{
  DiscordWindow *discord;
  const char **users;
  int user_count;
} UsersData;

// Helper function to free message data
static void free_message_data(gpointer data)
{
  MessageData *msg_data = (MessageData *)data;
  g_free(msg_data->sender);
  g_free(msg_data->message);
  g_free(msg_data);
}

// Helper function to free users data
static void free_users_data(gpointer data)
{
  UsersData *users_data = (UsersData *)data;
  g_free(users_data->users);
  g_free(users_data);
}

// Idle function to add message to chat
static gboolean idle_add_message(gpointer data)
{
  MessageData *msg_data = (MessageData *)data;
  add_message_to_chat(msg_data->discord, msg_data->sender, msg_data->message);
  return G_SOURCE_REMOVE;
}

// Idle function to update users list
static gboolean idle_update_users(gpointer data)
{
  UsersData *users_data = (UsersData *)data;
  update_users_list(users_data->discord, users_data->users, users_data->user_count);
  return G_SOURCE_REMOVE;
}

// Function to handle incoming messages
static void *message_receiver_thread(void *arg)
{
  DiscordWindow *discord = (DiscordWindow *)arg;
  char buffer[1024];

  while (thread_running)
  {
    ssize_t bytes_received = recv(chat_sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0)
    {
      if (thread_running) // Only print error if thread is still supposed to be running
      {
        perror("Error receiving message");
        break;
      }
      continue;
    }

    buffer[bytes_received] = '\0';

    // Parse JSON message
    struct json_object *json = json_tokener_parse(buffer);
    if (!json)
    {
      g_print("Invalid JSON received\n");
      continue;
    }

    struct json_object *type_obj;
    if (!json_object_object_get_ex(json, "type", &type_obj))
    {
      g_print("Missing message type\n");
      json_object_put(json);
      continue;
    }

    const char *type = json_object_get_string(type_obj);

    if (strcmp(type, "message") == 0)
    {
      struct json_object *user_id_obj, *channel_id_obj, *message_obj;
      if (json_object_object_get_ex(json, "user_id", &user_id_obj) &&
          json_object_object_get_ex(json, "channel_id", &channel_id_obj) &&
          json_object_object_get_ex(json, "message", &message_obj))
      {
        int channel_id = json_object_get_int(channel_id_obj);
        if (channel_id == discord->current_channel_id)
        {
          // Create message data
          MessageData *msg_data = g_new(MessageData, 1);
          msg_data->discord = discord;
          msg_data->sender = g_strdup_printf("%d", json_object_get_int(user_id_obj));
          msg_data->message = g_strdup(json_object_get_string(message_obj));

          // Add message to chat in the main thread
          g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, idle_add_message, msg_data, free_message_data);
        }
      }
    }
    else if (strcmp(type, "users") == 0)
    {
      struct json_object *users_array;
      if (json_object_object_get_ex(json, "users", &users_array) &&
          json_object_is_type(users_array, json_type_array))
      {
        int count = json_object_array_length(users_array);
        const char **users = g_new(const char *, count);

        for (int i = 0; i < count; i++)
        {
          struct json_object *user_obj = json_object_array_get_idx(users_array, i);
          users[i] = g_strdup(json_object_get_string(user_obj));
        }

        // Create users data
        UsersData *users_data = g_new(UsersData, 1);
        users_data->discord = discord;
        users_data->users = users;
        users_data->user_count = count;

        // Update users list in the main thread
        g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, idle_update_users, users_data, free_users_data);
      }
    }

    json_object_put(json);
  }

  return NULL;
}

static void on_message_send(GtkWidget *button, gpointer user_data)
{
  DiscordWindow *discord = (DiscordWindow *)user_data;
  const char *message = gtk_entry_get_text(GTK_ENTRY(discord->message_entry));

  if (strlen(message) > 0)
  {
    // Send message to server
    send_message_to_server(discord->user_id, discord->current_channel_id, message);

    // Clear message entry
    gtk_entry_set_text(GTK_ENTRY(discord->message_entry), "");
  }
}

static void on_channel_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
  if (row == NULL)
    return;

  DiscordWindow *discord = (DiscordWindow *)user_data;
  const char *channel_name = gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))));
  int channel_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "channel_id"));

  discord->current_channel_id = channel_id;
  // Request channel history from server
  request_channel_history(channel_id);
}

DiscordWindow *create_discord_window(int user_id, const char *username)
{
  DiscordWindow *discord = g_new(DiscordWindow, 1);
  discord->user_id = user_id;
  discord->username = g_strdup(username);
  discord->current_channel_id = 1; // Default to first channel

  // Create main window
  discord->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(discord->window), "Discord Clone");
  gtk_window_set_default_size(GTK_WINDOW(discord->window), 1200, 800);
  g_signal_connect(discord->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  // Create main horizontal box
  GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(discord->window), main_hbox);

  // Create left sidebar for channels
  GtkWidget *channels_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(channels_vbox, 200, -1);
  gtk_box_pack_start(GTK_BOX(main_hbox), channels_vbox, FALSE, FALSE, 0);

  // Channels header
  GtkWidget *channels_header = gtk_label_new("Channels");
  gtk_widget_set_halign(channels_header, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(channels_vbox), channels_header, FALSE, FALSE, 10);

  // Channels list
  discord->channels_list = gtk_list_box_new();
  gtk_widget_set_size_request(discord->channels_list, 200, -1);
  GtkWidget *channels_scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(channels_scroll), discord->channels_list);
  gtk_box_pack_start(GTK_BOX(channels_vbox), channels_scroll, TRUE, TRUE, 0);
  g_signal_connect(discord->channels_list, "row-selected", G_CALLBACK(on_channel_selected), discord);

  // Create middle section for messages
  GtkWidget *messages_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start(GTK_BOX(main_hbox), messages_vbox, TRUE, TRUE, 0);

  // Messages view
  discord->messages_text_view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(discord->messages_text_view), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(discord->messages_text_view), GTK_WRAP_WORD_CHAR);
  discord->messages_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(discord->messages_text_view));

  GtkWidget *messages_scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(messages_scroll), discord->messages_text_view);
  gtk_box_pack_start(GTK_BOX(messages_vbox), messages_scroll, TRUE, TRUE, 0);

  // Message input area
  GtkWidget *input_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(messages_vbox), input_hbox, FALSE, FALSE, 5);

  discord->message_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(input_hbox), discord->message_entry, TRUE, TRUE, 5);

  GtkWidget *send_button = gtk_button_new_with_label("Send");
  gtk_box_pack_start(GTK_BOX(input_hbox), send_button, FALSE, FALSE, 5);
  g_signal_connect(send_button, "clicked", G_CALLBACK(on_message_send), discord);

  // Create right sidebar for users
  GtkWidget *users_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(users_vbox, 200, -1);
  gtk_box_pack_start(GTK_BOX(main_hbox), users_vbox, FALSE, FALSE, 0);

  // Users header
  GtkWidget *users_header = gtk_label_new("Online Users");
  gtk_widget_set_halign(users_header, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(users_vbox), users_header, FALSE, FALSE, 10);

  // Users list
  discord->users_list = gtk_list_box_new();
  gtk_widget_set_size_request(discord->users_list, 200, -1);
  GtkWidget *users_scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(users_scroll), discord->users_list);
  gtk_box_pack_start(GTK_BOX(users_vbox), users_scroll, TRUE, TRUE, 0);

  // Apply CSS styling
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider,
                                  "window { background-color: #36393f; }"
                                  "label { color: #ffffff; }"
                                  "entry { background-color: #40444b; color: #ffffff; border: 1px solid #202225; border-radius: 4px; padding: 8px; }"
                                  "button { background-color: #7289da; color: #ffffff; border: none; border-radius: 4px; padding: 8px 16px; }"
                                  "button:hover { background-color: #677bc4; }"
                                  "list { background-color: #2f3136; color: #ffffff; }"
                                  "list row { padding: 8px; }"
                                  "list row:selected { background-color: #7289da; }"
                                  "textview { background-color: #36393f; color: #ffffff; }",
                                  -1, NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Start message receiving thread
  thread_running = true;
  if (pthread_create(&message_thread, NULL, message_receiver_thread, discord) != 0)
  {
    perror("Failed to create message thread");
    thread_running = false;
  }

  // Show all widgets
  gtk_widget_show_all(discord->window);

  return discord;
}

void add_message_to_chat(DiscordWindow *discord_window, const char *sender, const char *message)
{
  GtkTextIter end;
  gtk_text_buffer_get_end_iter(discord_window->messages_buffer, &end);

  char *formatted_message = g_strdup_printf("%s: %s\n", sender, message);
  gtk_text_buffer_insert(discord_window->messages_buffer, &end, formatted_message, -1);
  g_free(formatted_message);

  // Scroll to bottom
  gtk_text_buffer_get_end_iter(discord_window->messages_buffer, &end);
  GtkTextMark *mark = gtk_text_buffer_create_mark(discord_window->messages_buffer, NULL, &end, TRUE);
  gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(discord_window->messages_text_view), mark, 0.0, TRUE, 0.0, 1.0);
  gtk_text_buffer_delete_mark(discord_window->messages_buffer, mark);
}

void update_users_list(DiscordWindow *discord_window, const char **users, int user_count)
{
  // Clear existing list
  GList *children = gtk_container_get_children(GTK_CONTAINER(discord_window->users_list));
  for (GList *l = children; l != NULL; l = l->next)
  {
    gtk_widget_destroy(GTK_WIDGET(l->data));
  }
  g_list_free(children);

  // Add users to list
  for (int i = 0; i < user_count; i++)
  {
    GtkWidget *label = gtk_label_new(users[i]);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_list_box_insert(GTK_LIST_BOX(discord_window->users_list), label, -1);
  }
  gtk_widget_show_all(discord_window->users_list);
}

void update_channels_list(DiscordWindow *discord_window, const char **channels, int channel_count)
{
  // Clear existing list
  GList *children = gtk_container_get_children(GTK_CONTAINER(discord_window->channels_list));
  for (GList *l = children; l != NULL; l = l->next)
  {
    gtk_widget_destroy(GTK_WIDGET(l->data));
  }
  g_list_free(children);

  // Add channels to list
  for (int i = 0; i < channel_count; i++)
  {
    GtkWidget *label = gtk_label_new(channels[i]);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    GtkWidget *row = gtk_list_box_row_new();
    gtk_container_add(GTK_CONTAINER(row), label);
    g_object_set_data(G_OBJECT(row), "channel_id", GINT_TO_POINTER(i + 1));
    gtk_list_box_insert(GTK_LIST_BOX(discord_window->channels_list), row, -1);
  }
  gtk_widget_show_all(discord_window->channels_list);
}

// Add cleanup function
void destroy_discord_window(DiscordWindow *discord)
{
  if (discord)
  {
    // Stop message thread
    thread_running = false;
    pthread_join(message_thread, NULL);

    // Disconnect from chat server
    disconnect_from_chat_server();

    // Free memory
    g_free(discord->username);
    g_free(discord);
  }
}