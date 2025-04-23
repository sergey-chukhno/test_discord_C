#include "client_auth.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

// Global variable for chat socket
int chat_sockfd = -1;

// Register a new user
bool client_register_user(int sockfd, const char *first_name, const char *last_name,
                          const char *email, const char *password)
{
  struct json_object *json = json_object_new_object();
  json_object_object_add(json, "type", json_object_new_string("register"));
  json_object_object_add(json, "first_name", json_object_new_string(first_name));
  json_object_object_add(json, "last_name", json_object_new_string(last_name));
  json_object_object_add(json, "email", json_object_new_string(email));
  json_object_object_add(json, "password", json_object_new_string(password));

  const char *json_str = json_object_to_json_string(json);
  send(sockfd, json_str, strlen(json_str), 0);

  // Wait for response
  char buffer[1024];
  ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received <= 0)
  {
    printf("Failed to receive response from server\n");
    json_object_put(json);
    return false;
  }

  buffer[bytes_received] = '\0';
  struct json_object *response = json_tokener_parse(buffer);

  struct json_object *success_obj;
  bool success = false;
  if (json_object_object_get_ex(response, "success", &success_obj))
  {
    success = json_object_get_boolean(success_obj);
  }

  if (!success)
  {
    struct json_object *error_obj;
    if (json_object_object_get_ex(response, "error", &error_obj))
    {
      printf("Registration failed: %s\n", json_object_get_string(error_obj));
    }
  }

  json_object_put(json);
  json_object_put(response);
  return success;
}

// Login an existing user
LoginResult client_login_user(int sockfd, const char *email, const char *password)
{
  LoginResult result = {false, -1, ""};

  struct json_object *json = json_object_new_object();
  json_object_object_add(json, "type", json_object_new_string("login"));
  json_object_object_add(json, "email", json_object_new_string(email));
  json_object_object_add(json, "password", json_object_new_string(password));

  const char *json_str = json_object_to_json_string(json);
  printf("Sending login request: %s\n", json_str);

  ssize_t bytes_sent = send(sockfd, json_str, strlen(json_str), 0);
  if (bytes_sent < 0)
  {
    printf("Failed to send login request: %s\n", strerror(errno));
    snprintf(result.error_message, sizeof(result.error_message), "Failed to send login request: %s", strerror(errno));
    json_object_put(json);
    return result;
  }

  // Wait for response
  char buffer[1024];
  ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received <= 0)
  {
    printf("Failed to receive response from server: %s\n", strerror(errno));
    snprintf(result.error_message, sizeof(result.error_message), "Failed to receive response from server: %s", strerror(errno));
    json_object_put(json);
    return result;
  }

  buffer[bytes_received] = '\0';
  printf("Received server response: %s\n", buffer);

  struct json_object *response = json_tokener_parse(buffer);
  if (!response)
  {
    printf("Failed to parse server response as JSON\n");
    snprintf(result.error_message, sizeof(result.error_message), "Failed to parse server response as JSON");
    json_object_put(json);
    return result;
  }

  struct json_object *success_obj;
  if (json_object_object_get_ex(response, "success", &success_obj))
  {
    result.success = json_object_get_boolean(success_obj);
    if (result.success)
    {
      struct json_object *user_id_obj;
      if (json_object_object_get_ex(response, "user_id", &user_id_obj))
      {
        result.user_id = json_object_get_int(user_id_obj);
        printf("Login successful! User ID: %d\n", result.user_id);
      }
      else
      {
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message), "Server response missing user ID");
      }
    }
  }

  if (!result.success)
  {
    struct json_object *error_obj;
    if (json_object_object_get_ex(response, "error", &error_obj))
    {
      snprintf(result.error_message, sizeof(result.error_message), "%s", json_object_get_string(error_obj));
      printf("Login failed: %s\n", result.error_message);
    }
    else
    {
      snprintf(result.error_message, sizeof(result.error_message), "Unknown error");
      printf("Login failed: Unknown error\n");
    }
  }

  json_object_put(json);
  json_object_put(response);
  return result;
}

bool connect_to_chat_server(int user_id, const char *username)
{
  struct sockaddr_in server_addr;

  // Create socket
  chat_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (chat_sockfd < 0)
  {
    perror("Error creating socket");
    return false;
  }

  // Initialize server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
  {
    perror("Invalid address");
    close(chat_sockfd);
    return false;
  }

  // Connect to server
  if (connect(chat_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("Connection failed");
    close(chat_sockfd);
    return false;
  }

  // Send join message as JSON
  struct json_object *json = json_object_new_object();
  json_object_object_add(json, "type", json_object_new_string("join"));
  json_object_object_add(json, "user_id", json_object_new_int(user_id));
  json_object_object_add(json, "username", json_object_new_string(username));

  const char *json_str = json_object_to_json_string(json);
  if (send(chat_sockfd, json_str, strlen(json_str), 0) < 0)
  {
    perror("Send failed");
    json_object_put(json);
    close(chat_sockfd);
    return false;
  }

  json_object_put(json);
  return true;
}

bool send_message_to_server(int user_id, int channel_id, const char *message)
{
  if (chat_sockfd < 0)
    return false;

  // Create JSON message
  struct json_object *json = json_object_new_object();
  json_object_object_add(json, "type", json_object_new_string("message"));
  json_object_object_add(json, "user_id", json_object_new_int(user_id));
  json_object_object_add(json, "channel_id", json_object_new_int(channel_id));
  json_object_object_add(json, "message", json_object_new_string(message));

  const char *json_str = json_object_to_json_string(json);
  if (send(chat_sockfd, json_str, strlen(json_str), 0) < 0)
  {
    perror("Send failed");
    json_object_put(json);
    return false;
  }

  json_object_put(json);
  return true;
}

bool request_channel_history(int channel_id)
{
  if (chat_sockfd < 0)
    return false;

  // Create JSON request
  struct json_object *json = json_object_new_object();
  json_object_object_add(json, "type", json_object_new_string("history"));
  json_object_object_add(json, "channel_id", json_object_new_int(channel_id));

  const char *json_str = json_object_to_json_string(json);
  if (send(chat_sockfd, json_str, strlen(json_str), 0) < 0)
  {
    perror("Send failed");
    json_object_put(json);
    return false;
  }

  json_object_put(json);
  return true;
}

void disconnect_from_chat_server(void)
{
  if (chat_sockfd >= 0)
  {
    close(chat_sockfd);
    chat_sockfd = -1;
  }
}