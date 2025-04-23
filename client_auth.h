#ifndef CLIENT_AUTH_H
#define CLIENT_AUTH_H

#include <stdbool.h>

// Socket file descriptor for chat connection
extern int chat_sockfd;

typedef struct
{
  bool success;
  int user_id;
  char error_message[256];
} LoginResult;

// Client-side authentication functions
bool client_register_user(int sockfd, const char *first_name, const char *last_name,
                          const char *email, const char *password);
LoginResult client_login_user(int sockfd, const char *email, const char *password);

// Connect to the chat server
bool connect_to_chat_server(int user_id, const char *username);

// Send a message to a specific channel
bool send_message_to_server(int user_id, int channel_id, const char *message);

// Request message history for a channel
bool request_channel_history(int channel_id);

// Disconnect from the chat server
void disconnect_from_chat_server(void);

#endif // CLIENT_AUTH_H