#include "common.h"
#include "client_auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json-c/json.h>

int main()
{
  int sockfd;
  struct sockaddr_in server_addr;

  // Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Configure server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // Connect to server
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("Connection failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("Connected to server\n");

  // Authentication menu
  int choice;
  char first_name[100], last_name[100], email[150], password[100];

  while (1)
  {
    printf("\n1. Register\n2. Login\n3. Exit\nEnter your choice: ");
    scanf("%d", &choice);
    getchar(); // Clear newline

    switch (choice)
    {
    case 1: // Register
      printf("Enter first name: ");
      fgets(first_name, sizeof(first_name), stdin);
      first_name[strcspn(first_name, "\n")] = '\0';

      printf("Enter last name: ");
      fgets(last_name, sizeof(last_name), stdin);
      last_name[strcspn(last_name, "\n")] = '\0';

      printf("Enter email: ");
      fgets(email, sizeof(email), stdin);
      email[strcspn(email, "\n")] = '\0';

      printf("Enter password: ");
      fgets(password, sizeof(password), stdin);
      password[strcspn(password, "\n")] = '\0';

      if (register_user(sockfd, first_name, last_name, email, password))
      {
        printf("Registration successful!\n");
      }
      break;

    case 2: // Login
      printf("Enter email: ");
      fgets(email, sizeof(email), stdin);
      email[strcspn(email, "\n")] = '\0';

      printf("Enter password: ");
      fgets(password, sizeof(password), stdin);
      password[strcspn(password, "\n")] = '\0';

      if (login_user(sockfd, email, password))
      {
        printf("Login successful!\n");
        // TODO: Enter chat interface
      }
      break;

    case 3: // Exit
      // Send disconnect message
      struct json_object *json = json_object_new_object();
      json_object_object_add(json, "type", json_object_new_int(MSG_DISCONNECT));
      json_object_object_add(json, "content", json_object_new_string("Goodbye!"));
      const char *json_str = json_object_to_json_string(json);
      send(sockfd, json_str, strlen(json_str), 0);
      json_object_put(json);

      close(sockfd);
      exit(EXIT_SUCCESS);

    default:
      printf("Invalid choice. Please try again.\n");
    }
  }

  return 0;
}