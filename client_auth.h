#ifndef CLIENT_AUTH_H
#define CLIENT_AUTH_H

#include "common.h"
#include <stdbool.h>

// Function declarations
bool register_user(int sockfd, const char *first_name, const char *last_name,
                   const char *email, const char *password);
bool login_user(int sockfd, const char *email, const char *password);

#endif // CLIENT_AUTH_H