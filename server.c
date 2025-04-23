#include "common.h"
#include "db_connection.h"
#include "db_operations.h"
#include "auth.h"
#include <json-c/json.h>
#include <signal.h>
#include <errno.h>

static volatile int running = 1;

void handle_signal(int sig)
{
  (void)sig; // Suppress unused parameter warning
  running = 0;
}

// Send error response to client
void send_error_response(int client_fd, const char *error_message)
{
  struct json_object *response = json_object_new_object();
  json_object_object_add(response, "success", json_object_new_boolean(false));
  json_object_object_add(response, "error", json_object_new_string(error_message));

  const char *response_str = json_object_to_json_string(response);
  send(client_fd, response_str, strlen(response_str), 0);
  json_object_put(response);
}

// Handle authentication requests
void handle_auth_request(int client_fd, PGconn *db_conn, const char *content)
{
  printf("Received auth request: %s\n", content);

  struct json_object *json = json_tokener_parse(content);
  if (!json)
  {
    printf("Invalid JSON format\n");
    send_error_response(client_fd, "Invalid JSON format");
    return;
  }

  struct json_object *type_obj;
  if (!json_object_object_get_ex(json, "type", &type_obj))
  {
    printf("Missing message type\n");
    send_error_response(client_fd, "Missing message type");
    json_object_put(json);
    return;
  }

  const char *type = json_object_get_string(type_obj);
  printf("Auth type: %s\n", type);
  AuthResult result;

  if (strcmp(type, "register") == 0)
  {
    struct json_object *first_name_obj, *last_name_obj, *email_obj, *password_obj;
    if (!json_object_object_get_ex(json, "first_name", &first_name_obj) ||
        !json_object_object_get_ex(json, "last_name", &last_name_obj) ||
        !json_object_object_get_ex(json, "email", &email_obj) ||
        !json_object_object_get_ex(json, "password", &password_obj))
    {
      printf("Missing required fields for registration\n");
      send_error_response(client_fd, "Missing required fields for registration");
      json_object_put(json);
      return;
    }

    result = register_user(db_conn,
                           json_object_get_string(first_name_obj),
                           json_object_get_string(last_name_obj),
                           json_object_get_string(email_obj),
                           json_object_get_string(password_obj));
  }
  else if (strcmp(type, "login") == 0)
  {
    struct json_object *email_obj, *password_obj;
    if (!json_object_object_get_ex(json, "email", &email_obj) ||
        !json_object_object_get_ex(json, "password", &password_obj))
    {
      printf("Missing email or password\n");
      send_error_response(client_fd, "Missing email or password");
      json_object_put(json);
      return;
    }

    const char *email = json_object_get_string(email_obj);
    const char *password = json_object_get_string(password_obj);
    printf("Login attempt for email: %s\n", email);

    result = login_user(db_conn, email, password);
    printf("Login result: %s\n", result.success ? "success" : "failed");
  }
  else
  {
    printf("Invalid auth type: %s\n", type);
    send_error_response(client_fd, "Invalid auth type");
    json_object_put(json);
    return;
  }

  // Send response
  struct json_object *response = json_object_new_object();
  json_object_object_add(response, "success", json_object_new_boolean(result.success));
  if (result.success)
  {
    json_object_object_add(response, "user_id", json_object_new_int(result.user_id));
  }
  else
  {
    json_object_object_add(response, "error", json_object_new_string(result.error_message));
  }

  const char *response_str = json_object_to_json_string(response);
  printf("Sending response: %s\n", response_str);
  send(client_fd, response_str, strlen(response_str), 0);

  json_object_put(json);
  json_object_put(response);
}

// Handle client connection
void handle_client(int client_fd, struct sockaddr_in client_addr)
{
  char buffer[MAX_BUFFER_SIZE];
  ssize_t bytes_read;
  PGconn *db_conn = connect_to_database();

  if (!db_conn)
  {
    printf("Failed to connect to database for client %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    close(client_fd);
    return;
  }

  printf("New client connected from %s:%d\n",
         inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

  while ((bytes_read = recv(client_fd, buffer, MAX_BUFFER_SIZE - 1, 0)) > 0)
  {
    buffer[bytes_read] = '\0';

    struct json_object *json = json_tokener_parse(buffer);
    if (!json)
    {
      send_error_response(client_fd, "Invalid JSON format");
      continue;
    }

    struct json_object *type_obj;
    if (!json_object_object_get_ex(json, "type", &type_obj))
    {
      send_error_response(client_fd, "Missing message type");
      json_object_put(json);
      continue;
    }

    const char *type = json_object_get_string(type_obj);

    if (strcmp(type, "register") == 0 || strcmp(type, "login") == 0)
    {
      handle_auth_request(client_fd, db_conn, buffer);
    }
    else if (strcmp(type, "join") == 0)
    {
      // Handle join request
      struct json_object *user_id_obj, *username_obj;
      if (!json_object_object_get_ex(json, "user_id", &user_id_obj) ||
          !json_object_object_get_ex(json, "username", &username_obj))
      {
        send_error_response(client_fd, "Missing user_id or username");
        json_object_put(json);
        continue;
      }

      int user_id = json_object_get_int(user_id_obj);
      const char *username = json_object_get_string(username_obj);

      // TODO: Add user to online users list
      struct json_object *response = json_object_new_object();
      json_object_object_add(response, "success", json_object_new_boolean(true));
      json_object_object_add(response, "message", json_object_new_string("Joined successfully"));

      const char *response_str = json_object_to_json_string(response);
      send(client_fd, response_str, strlen(response_str), 0);
      json_object_put(response);
    }
    else if (strcmp(type, "message") == 0)
    {
      // Handle chat message
      struct json_object *user_id_obj, *channel_id_obj, *message_obj;
      if (!json_object_object_get_ex(json, "user_id", &user_id_obj) ||
          !json_object_object_get_ex(json, "channel_id", &channel_id_obj) ||
          !json_object_object_get_ex(json, "message", &message_obj))
      {
        send_error_response(client_fd, "Missing required fields for message");
        json_object_put(json);
        continue;
      }

      int user_id = json_object_get_int(user_id_obj);
      int channel_id = json_object_get_int(channel_id_obj);
      const char *message = json_object_get_string(message_obj);

      // Store message in database
      int message_id = create_message(db_conn, user_id, channel_id, message);
      if (message_id > 0)
      {
        struct json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(true));
        json_object_object_add(response, "message_id", json_object_new_int(message_id));

        const char *response_str = json_object_to_json_string(response);
        send(client_fd, response_str, strlen(response_str), 0);
        json_object_put(response);
      }
      else
      {
        send_error_response(client_fd, "Failed to store message");
      }
    }
    else if (strcmp(type, "history") == 0)
    {
      // Handle history request
      struct json_object *channel_id_obj;
      if (!json_object_object_get_ex(json, "channel_id", &channel_id_obj))
      {
        send_error_response(client_fd, "Missing channel_id");
        json_object_put(json);
        continue;
      }

      int channel_id = json_object_get_int(channel_id_obj);
      // TODO: Retrieve message history from database
      struct json_object *response = json_object_new_object();
      json_object_object_add(response, "success", json_object_new_boolean(true));
      json_object_object_add(response, "messages", json_object_new_array());

      const char *response_str = json_object_to_json_string(response);
      send(client_fd, response_str, strlen(response_str), 0);
      json_object_put(response);
    }
    else
    {
      send_error_response(client_fd, "Unknown message type");
    }

    json_object_put(json);
  }

  printf("Client %s:%d disconnected\n",
         inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
  close_database_connection(db_conn);
  close(client_fd);
}

int main()
{
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  // Set up signal handlers
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGCHLD, SIG_IGN); // Prevent zombie processes

  // Test database connection first
  PGconn *db_conn = connect_to_database();
  if (!db_conn)
  {
    printf("Failed to connect to database. Server startup aborted.\n");
    return 1;
  }
  printf("Database connection successful. Server starting...\n");
  close_database_connection(db_conn);

  // Create socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Set socket options
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("Setsockopt failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // Configure server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);

  // Bind socket
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("Bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // Listen for connections
  if (listen(server_fd, MAX_CLIENTS) < 0)
  {
    perror("Listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server is listening on port %d...\n", SERVER_PORT);

  while (running)
  {
    // Accept client connection
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
      if (errno == EINTR)
      {
        // Interrupted system call, check if we should continue running
        continue;
      }
      perror("Accept failed");
      continue;
    }

    // Fork a new process to handle the client
    pid_t pid = fork();
    if (pid < 0)
    {
      perror("Fork failed");
      close(client_fd);
      continue;
    }

    if (pid == 0)
    {
      // Child process
      close(server_fd); // Close server socket in child
      handle_client(client_fd, client_addr);
      exit(0); // Ensure child process exits after handling client
    }
    else
    {
      // Parent process
      close(client_fd); // Close client socket in parent
    }
  }

  // Cleanup
  close(server_fd);
  printf("\nServer shutting down...\n");
  return 0;
}