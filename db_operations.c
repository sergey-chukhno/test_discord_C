#include "db_operations.h"
#include <stdio.h>
#include <string.h>

// User operations
bool create_user(PGconn *conn, const char *first_name, const char *last_name,
                 const char *email, const char *password_hash)
{
  const char *query = "INSERT INTO USERS (first_name, last_name, email, password_hash) "
                      "VALUES ($1, $2, $3, $4) RETURNING user_id";
  const char *values[4] = {first_name, last_name, email, password_hash};
  int lengths[4] = {strlen(first_name), strlen(last_name), strlen(email), strlen(password_hash)};
  int formats[4] = {0, 0, 0, 0}; // 0 for text format

  PGresult *res = PQexecParams(conn, query, 4, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_TUPLES_OK);
  PQclear(res);

  return success;
}

bool update_user_status(PGconn *conn, int user_id, const char *status)
{
  const char *query = "UPDATE USERS SET status = $1 WHERE user_id = $2";
  const char *values[2] = {status, NULL};
  int lengths[2] = {strlen(status), 0};
  int formats[2] = {0, 0}; // 0 for text format

  char user_id_str[20];
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  values[1] = user_id_str;
  lengths[1] = strlen(user_id_str);

  PGresult *res = PQexecParams(conn, query, 2, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

bool update_user_last_login(PGconn *conn, int user_id)
{
  const char *query = "UPDATE USERS SET last_login = CURRENT_TIMESTAMP WHERE user_id = $1";
  const char *values[1] = {NULL};
  int lengths[1] = {0};
  int formats[1] = {0}; // 0 for text format

  char user_id_str[20];
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  values[0] = user_id_str;
  lengths[0] = strlen(user_id_str);

  PGresult *res = PQexecParams(conn, query, 1, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

bool deactivate_user(PGconn *conn, int user_id)
{
  const char *query = "UPDATE USERS SET is_active = false WHERE user_id = $1";
  const char *values[1] = {NULL};
  int lengths[1] = {0};
  int formats[1] = {0}; // 0 for text format

  char user_id_str[20];
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  values[0] = user_id_str;
  lengths[0] = strlen(user_id_str);

  PGresult *res = PQexecParams(conn, query, 1, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

// Channel operations
int create_channel(PGconn *conn, const char *name, const char *description,
                   const char *channel_type, int created_by, bool is_private)
{
  const char *query = "INSERT INTO CHANNELS (name, description, channel_type, created_by, is_private) "
                      "VALUES ($1, $2, $3, $4, $5) RETURNING channel_id";
  const char *values[5] = {name, description, channel_type, NULL, NULL};
  int lengths[5] = {strlen(name), strlen(description), strlen(channel_type), 0, 0};
  int formats[5] = {0, 0, 0, 0, 0}; // 0 for text format

  char created_by_str[20];
  char is_private_str[2];
  snprintf(created_by_str, sizeof(created_by_str), "%d", created_by);
  snprintf(is_private_str, sizeof(is_private_str), "%d", is_private ? 1 : 0);
  values[3] = created_by_str;
  values[4] = is_private_str;
  lengths[3] = strlen(created_by_str);
  lengths[4] = strlen(is_private_str);

  PGresult *res = PQexecParams(conn, query, 5, NULL, values, lengths, formats, 0);
  int channel_id = -1;

  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
  {
    channel_id = atoi(PQgetvalue(res, 0, 0));
  }

  PQclear(res);
  return channel_id;
}

bool add_channel_member(PGconn *conn, int user_id, int channel_id)
{
  const char *query = "INSERT INTO CHANNEL_MEMBERS (user_id, channel_id) VALUES ($1, $2)";
  const char *values[2] = {NULL, NULL};
  int lengths[2] = {0, 0};
  int formats[2] = {0, 0}; // 0 for text format

  char user_id_str[20];
  char channel_id_str[20];
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  snprintf(channel_id_str, sizeof(channel_id_str), "%d", channel_id);
  values[0] = user_id_str;
  values[1] = channel_id_str;
  lengths[0] = strlen(user_id_str);
  lengths[1] = strlen(channel_id_str);

  PGresult *res = PQexecParams(conn, query, 2, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

bool remove_channel_member(PGconn *conn, int user_id, int channel_id)
{
  const char *query = "DELETE FROM CHANNEL_MEMBERS WHERE user_id = $1 AND channel_id = $2";
  const char *values[2] = {NULL, NULL};
  int lengths[2] = {0, 0};
  int formats[2] = {0, 0}; // 0 for text format

  char user_id_str[20];
  char channel_id_str[20];
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  snprintf(channel_id_str, sizeof(channel_id_str), "%d", channel_id);
  values[0] = user_id_str;
  values[1] = channel_id_str;
  lengths[0] = strlen(user_id_str);
  lengths[1] = strlen(channel_id_str);

  PGresult *res = PQexecParams(conn, query, 2, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

// Message operations
int create_message(PGconn *conn, int user_id, int channel_id, const char *content)
{
  const char *query = "INSERT INTO MESSAGES (user_id, channel_id, content) "
                      "VALUES ($1, $2, $3) RETURNING message_id";
  const char *values[3] = {NULL, NULL, content};
  int lengths[3] = {0, 0, strlen(content)};
  int formats[3] = {0, 0, 0}; // 0 for text format

  char user_id_str[20];
  char channel_id_str[20];
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  snprintf(channel_id_str, sizeof(channel_id_str), "%d", channel_id);
  values[0] = user_id_str;
  values[1] = channel_id_str;
  lengths[0] = strlen(user_id_str);
  lengths[1] = strlen(channel_id_str);

  PGresult *res = PQexecParams(conn, query, 3, NULL, values, lengths, formats, 0);
  int message_id = -1;

  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
  {
    message_id = atoi(PQgetvalue(res, 0, 0));
  }

  PQclear(res);
  return message_id;
}

bool edit_message(PGconn *conn, int message_id, const char *new_content)
{
  const char *query = "UPDATE MESSAGES SET content = $1, is_edited = true, "
                      "edited_at = CURRENT_TIMESTAMP WHERE message_id = $2";
  const char *values[2] = {new_content, NULL};
  int lengths[2] = {strlen(new_content), 0};
  int formats[2] = {0, 0}; // 0 for text format

  char message_id_str[20];
  snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
  values[1] = message_id_str;
  lengths[1] = strlen(message_id_str);

  PGresult *res = PQexecParams(conn, query, 2, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

bool delete_message(PGconn *conn, int message_id)
{
  const char *query = "DELETE FROM MESSAGES WHERE message_id = $1";
  const char *values[1] = {NULL};
  int lengths[1] = {0};
  int formats[1] = {0}; // 0 for text format

  char message_id_str[20];
  snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
  values[0] = message_id_str;
  lengths[0] = strlen(message_id_str);

  PGresult *res = PQexecParams(conn, query, 1, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

// Direct message operations
int create_direct_message(PGconn *conn, int sender_id, int recipient_id,
                          const char *content)
{
  const char *query = "INSERT INTO DIRECT_MESSAGES (sender_id, recipient_id, content) "
                      "VALUES ($1, $2, $3) RETURNING dm_id";
  const char *values[3] = {NULL, NULL, content};
  int lengths[3] = {0, 0, strlen(content)};
  int formats[3] = {0, 0, 0}; // 0 for text format

  char sender_id_str[20];
  char recipient_id_str[20];
  snprintf(sender_id_str, sizeof(sender_id_str), "%d", sender_id);
  snprintf(recipient_id_str, sizeof(recipient_id_str), "%d", recipient_id);
  values[0] = sender_id_str;
  values[1] = recipient_id_str;
  lengths[0] = strlen(sender_id_str);
  lengths[1] = strlen(recipient_id_str);

  PGresult *res = PQexecParams(conn, query, 3, NULL, values, lengths, formats, 0);
  int dm_id = -1;

  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
  {
    dm_id = atoi(PQgetvalue(res, 0, 0));
  }

  PQclear(res);
  return dm_id;
}

bool mark_direct_message_read(PGconn *conn, int dm_id)
{
  const char *query = "UPDATE DIRECT_MESSAGES SET is_read = true WHERE dm_id = $1";
  const char *values[1] = {NULL};
  int lengths[1] = {0};
  int formats[1] = {0}; // 0 for text format

  char dm_id_str[20];
  snprintf(dm_id_str, sizeof(dm_id_str), "%d", dm_id);
  values[0] = dm_id_str;
  lengths[0] = strlen(dm_id_str);

  PGresult *res = PQexecParams(conn, query, 1, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

// Role operations
int create_role(PGconn *conn, const char *name, const char *color, int permission_level)
{
  const char *query = "INSERT INTO ROLES (name, color, permission_level) "
                      "VALUES ($1, $2, $3) RETURNING role_id";
  const char *values[3] = {name, color, NULL};
  int lengths[3] = {strlen(name), strlen(color), 0};
  int formats[3] = {0, 0, 0}; // 0 for text format

  char permission_level_str[20];
  snprintf(permission_level_str, sizeof(permission_level_str), "%d", permission_level);
  values[2] = permission_level_str;
  lengths[2] = strlen(permission_level_str);

  PGresult *res = PQexecParams(conn, query, 3, NULL, values, lengths, formats, 0);
  int role_id = -1;

  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
  {
    role_id = atoi(PQgetvalue(res, 0, 0));
  }

  PQclear(res);
  return role_id;
}

bool assign_role_to_user(PGconn *conn, int user_id, int role_id, int channel_id)
{
  const char *query = "INSERT INTO USER_ROLES (user_id, role_id, channel_id) "
                      "VALUES ($1, $2, $3)";
  const char *values[3] = {NULL, NULL, NULL};
  int lengths[3] = {0, 0, 0};
  int formats[3] = {0, 0, 0}; // 0 for text format

  char user_id_str[20];
  char role_id_str[20];
  char channel_id_str[20];
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  snprintf(role_id_str, sizeof(role_id_str), "%d", role_id);
  snprintf(channel_id_str, sizeof(channel_id_str), "%d", channel_id);
  values[0] = user_id_str;
  values[1] = role_id_str;
  values[2] = channel_id_str;
  lengths[0] = strlen(user_id_str);
  lengths[1] = strlen(role_id_str);
  lengths[2] = strlen(channel_id_str);

  PGresult *res = PQexecParams(conn, query, 3, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

bool remove_role_from_user(PGconn *conn, int user_role_id)
{
  const char *query = "DELETE FROM USER_ROLES WHERE user_role_id = $1";
  const char *values[1] = {NULL};
  int lengths[1] = {0};
  int formats[1] = {0}; // 0 for text format

  char user_role_id_str[20];
  snprintf(user_role_id_str, sizeof(user_role_id_str), "%d", user_role_id);
  values[0] = user_role_id_str;
  lengths[0] = strlen(user_role_id_str);

  PGresult *res = PQexecParams(conn, query, 1, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

// Reaction operations
bool add_reaction(PGconn *conn, int message_id, int user_id, const char *emoji)
{
  const char *query = "INSERT INTO REACTIONS (message_id, user_id, emoji) "
                      "VALUES ($1, $2, $3)";
  const char *values[3] = {NULL, NULL, emoji};
  int lengths[3] = {0, 0, strlen(emoji)};
  int formats[3] = {0, 0, 0}; // 0 for text format

  char message_id_str[20];
  char user_id_str[20];
  snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
  snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
  values[0] = message_id_str;
  values[1] = user_id_str;
  lengths[0] = strlen(message_id_str);
  lengths[1] = strlen(user_id_str);

  PGresult *res = PQexecParams(conn, query, 3, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

bool remove_reaction(PGconn *conn, int reaction_id)
{
  const char *query = "DELETE FROM REACTIONS WHERE reaction_id = $1";
  const char *values[1] = {NULL};
  int lengths[1] = {0};
  int formats[1] = {0}; // 0 for text format

  char reaction_id_str[20];
  snprintf(reaction_id_str, sizeof(reaction_id_str), "%d", reaction_id);
  values[0] = reaction_id_str;
  lengths[0] = strlen(reaction_id_str);

  PGresult *res = PQexecParams(conn, query, 1, NULL, values, lengths, formats, 0);
  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  PQclear(res);

  return success;
}

// Attachment operations
int add_attachment(PGconn *conn, int message_id, const char *file_name,
                   const char *file_path, int file_size, const char *mime_type)
{
  const char *query = "INSERT INTO ATTACHMENTS (message_id, file_name, file_path, "
                      "file_size, mime_type) VALUES ($1, $2, $3, $4, $5) RETURNING attachment_id";
  const char *values[5] = {NULL, file_name, file_path, NULL, mime_type};
  int lengths[5] = {0, strlen(file_name), strlen(file_path), 0, strlen(mime_type)};
  int formats[5] = {0, 0, 0, 0, 0}; // 0 for text format

  char message_id_str[20];
  char file_size_str[20];
  snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
  snprintf(file_size_str, sizeof(file_size_str), "%d", file_size);
  values[0] = message_id_str;
  values[3] = file_size_str;
  lengths[0] = strlen(message_id_str);
  lengths[3] = strlen(file_size_str);

  PGresult *res = PQexecParams(conn, query, 5, NULL, values, lengths, formats, 0);
  int attachment_id = -1;

  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
  {
    attachment_id = atoi(PQgetvalue(res, 0, 0));
  }

  PQclear(res);
  return attachment_id;
}