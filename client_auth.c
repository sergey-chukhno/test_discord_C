#include "client_auth.h"
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>

// Register a new user
bool register_user(int sockfd, const char *first_name, const char *last_name,
                   const char *email, const char *password)
{
  struct json_object *json = json_object_new_object();
  json_object_object_add(json, "type", json_object_new_int(MSG_CONNECT));

  struct json_object *content = json_object_new_object();
  json_object_object_add(content, "auth_type", json_object_new_string("register"));
  json_object_object_add(content, "first_name", json_object_new_string(first_name));
  json_object_object_add(content, "last_name", json_object_new_string(last_name));
  json_object_object_add(content, "email", json_object_new_string(email));
  json_object_object_add(content, "password", json_object_new_string(password));

  json_object_object_add(json, "content", content);

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
bool login_user(int sockfd, const char *email, const char *password)
{
  struct json_object *json = json_object_new_object();
  json_object_object_add(json, "type", json_object_new_int(MSG_CONNECT));

  struct json_object *content = json_object_new_object();
  json_object_object_add(content, "auth_type", json_object_new_string("login"));
  json_object_object_add(content, "email", json_object_new_string(email));
  json_object_object_add(content, "password", json_object_new_string(password));

  json_object_object_add(json, "content", content);

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
      printf("Login failed: %s\n", json_object_get_string(error_obj));
    }
  }

  json_object_put(json);
  json_object_put(response);
  return success;
}