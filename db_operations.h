#ifndef DB_OPERATIONS_H
#define DB_OPERATIONS_H

#include "db_connection.h"
#include <stdbool.h>

// User operations
bool create_user(PGconn *conn, const char *first_name, const char *last_name,
                 const char *email, const char *password_hash);
bool update_user_status(PGconn *conn, int user_id, const char *status);
bool update_user_last_login(PGconn *conn, int user_id);
bool deactivate_user(PGconn *conn, int user_id);

// Channel operations
int create_channel(PGconn *conn, const char *name, const char *description,
                   const char *channel_type, int created_by, bool is_private);
bool add_channel_member(PGconn *conn, int user_id, int channel_id);
bool remove_channel_member(PGconn *conn, int user_id, int channel_id);

// Message operations
int create_message(PGconn *conn, int user_id, int channel_id, const char *content);
bool edit_message(PGconn *conn, int message_id, const char *new_content);
bool delete_message(PGconn *conn, int message_id);

// Direct message operations
int create_direct_message(PGconn *conn, int sender_id, int recipient_id,
                          const char *content);
bool mark_direct_message_read(PGconn *conn, int dm_id);

// Role operations
int create_role(PGconn *conn, const char *name, const char *color, int permission_level);
bool assign_role_to_user(PGconn *conn, int user_id, int role_id, int channel_id);
bool remove_role_from_user(PGconn *conn, int user_role_id);

// Reaction operations
bool add_reaction(PGconn *conn, int message_id, int user_id, const char *emoji);
bool remove_reaction(PGconn *conn, int reaction_id);

// Attachment operations
int add_attachment(PGconn *conn, int message_id, const char *file_name,
                   const char *file_path, int file_size, const char *mime_type);

#endif // DB_OPERATIONS_H