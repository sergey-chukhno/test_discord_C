#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auth.h"
#include "db_connection.h"

int main()
{
  // Connect to the database
  PGconn *conn = connect_to_database();
  if (conn == NULL)
  {
    printf("Failed to connect to database\n");
    return 1;
  }

  printf("Testing authentication system...\n\n");

  // Test 1: Register a new user
  printf("Test 1: Register new user\n");
  AuthResult reg_result = register_user(conn, "Test", "User", "test@example.com", "password123");
  if (reg_result.success)
  {
    printf("✓ Registration successful! User ID: %d\n", reg_result.user_id);
  }
  else
  {
    printf("✗ Registration failed: %s\n", reg_result.error_message);
  }

  // Test 2: Try to register with same email
  printf("\nTest 2: Try to register with existing email\n");
  AuthResult dup_result = register_user(conn, "Test", "User2", "test@example.com", "password123");
  if (!dup_result.success)
  {
    printf("✓ Correctly prevented duplicate registration: %s\n", dup_result.error_message);
  }
  else
  {
    printf("✗ Failed to prevent duplicate registration\n");
  }

  // Test 3: Login with correct credentials
  printf("\nTest 3: Login with correct credentials\n");
  AuthResult login_result = login_user(conn, "test@example.com", "password123");
  if (login_result.success)
  {
    printf("✓ Login successful! User ID: %d\n", login_result.user_id);
  }
  else
  {
    printf("✗ Login failed: %s\n", login_result.error_message);
  }

  // Test 4: Login with incorrect password
  printf("\nTest 4: Login with incorrect password\n");
  AuthResult wrong_pass_result = login_user(conn, "test@example.com", "wrongpassword");
  if (!wrong_pass_result.success)
  {
    printf("✓ Correctly rejected incorrect password: %s\n", wrong_pass_result.error_message);
  }
  else
  {
    printf("✗ Failed to reject incorrect password\n");
  }

  // Test 5: Login with non-existent email
  printf("\nTest 5: Login with non-existent email\n");
  AuthResult wrong_email_result = login_user(conn, "nonexistent@example.com", "password123");
  if (!wrong_email_result.success)
  {
    printf("✓ Correctly rejected non-existent email: %s\n", wrong_email_result.error_message);
  }
  else
  {
    printf("✗ Failed to reject non-existent email\n");
  }

  // Clean up
  PQfinish(conn);
  return 0;
}