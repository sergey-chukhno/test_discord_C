#include <gtk/gtk.h>
#include "auth.h"
#include "db_connection.h"
#include "client_auth.h"
#include "gui.h"
#include "discord_window.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Global variables for the main window and database connection
static GtkWidget *window;
static PGconn *conn;

// Callback for registration form
static void on_register_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *window = g_object_get_data(G_OBJECT(widget), "window");
  GtkWidget *first_name_entry = g_object_get_data(G_OBJECT(widget), "first_name_entry");
  GtkWidget *last_name_entry = g_object_get_data(G_OBJECT(widget), "last_name_entry");
  GtkWidget *email_entry = g_object_get_data(G_OBJECT(widget), "email_entry");
  GtkWidget *password_entry = g_object_get_data(G_OBJECT(widget), "password_entry");
  GtkWidget *confirm_password_entry = g_object_get_data(G_OBJECT(widget), "confirm_password_entry");

  if (!window || !first_name_entry || !last_name_entry || !email_entry || !password_entry || !confirm_password_entry)
  {
    g_print("Error: Missing widget data\n");
    return;
  }

  const char *first_name = gtk_entry_get_text(GTK_ENTRY(first_name_entry));
  const char *last_name = gtk_entry_get_text(GTK_ENTRY(last_name_entry));
  const char *email = gtk_entry_get_text(GTK_ENTRY(email_entry));
  const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));
  const char *confirm_password = gtk_entry_get_text(GTK_ENTRY(confirm_password_entry));

  // Validate input
  if (strlen(first_name) == 0 || strlen(last_name) == 0 ||
      strlen(email) == 0 || strlen(password) == 0 || strlen(confirm_password) == 0)
  {
    show_error_dialog(window, "All fields are required");
    return;
  }

  if (strcmp(password, confirm_password) != 0)
  {
    show_error_dialog(window, "Passwords do not match");
    return;
  }

  // Connect to server
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    show_error_dialog(window, "Failed to create socket");
    return;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
  {
    show_error_dialog(window, "Invalid address");
    close(sockfd);
    return;
  }

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    show_error_dialog(window, "Connection failed");
    close(sockfd);
    return;
  }

  // Register user
  if (client_register_user(sockfd, first_name, last_name, email, password))
  {
    show_success_dialog(window, "Registration successful! Please login.");
    close(sockfd);
    return;
  }
  else
  {
    show_error_dialog(window, "Registration failed");
    close(sockfd);
    return;
  }
}

// Callback for login form
static void on_login_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *window = g_object_get_data(G_OBJECT(widget), "window");
  GtkWidget *email_entry = g_object_get_data(G_OBJECT(widget), "email_entry");
  GtkWidget *password_entry = g_object_get_data(G_OBJECT(widget), "password_entry");

  if (!window || !email_entry || !password_entry)
  {
    g_print("Error: Missing widget data\n");
    return;
  }

  const char *email = gtk_entry_get_text(GTK_ENTRY(email_entry));
  const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

  // Validate input
  if (strlen(email) == 0 || strlen(password) == 0)
  {
    show_error_dialog(window, "Email and password are required");
    return;
  }

  // Connect to server
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    show_error_dialog(window, "Failed to create socket");
    return;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
  {
    show_error_dialog(window, "Invalid address");
    close(sockfd);
    return;
  }

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    show_error_dialog(window, "Connection failed");
    close(sockfd);
    return;
  }

  // Login user
  LoginResult login_result = client_login_user(sockfd, email, password);
  if (login_result.success)
  {
    // Hide login window
    gtk_widget_hide(window);

    // Create Discord window
    DiscordWindow *discord = create_discord_window(login_result.user_id, email);

    // Connect to chat server using the same socket
    if (!connect_to_chat_server(login_result.user_id, email))
    {
      show_error_dialog(window, "Failed to connect to chat server");
      destroy_discord_window(discord);
      gtk_widget_show(window);
      close(sockfd);
    }
    else
    {
      // Request initial channel list and user list
      request_channel_history(1); // Request history for default channel
    }
  }
  else
  {
    show_error_dialog(window, login_result.error_message);
    close(sockfd);
  }
}

// Create registration form
static GtkWidget *create_registration_form()
{
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(vbox, 20);
  gtk_widget_set_margin_end(vbox, 20);
  gtk_widget_set_margin_top(vbox, 20);
  gtk_widget_set_margin_bottom(vbox, 20);

  // Create form fields
  GtkWidget *first_name_entry = gtk_entry_new();
  GtkWidget *last_name_entry = gtk_entry_new();
  GtkWidget *email_entry = gtk_entry_new();
  GtkWidget *password_entry = gtk_entry_new();
  GtkWidget *confirm_password_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
  gtk_entry_set_visibility(GTK_ENTRY(confirm_password_entry), FALSE);

  // Create labels
  GtkWidget *first_name_label = gtk_label_new("First Name:");
  GtkWidget *last_name_label = gtk_label_new("Last Name:");
  GtkWidget *email_label = gtk_label_new("Email:");
  GtkWidget *password_label = gtk_label_new("Password:");
  GtkWidget *confirm_password_label = gtk_label_new("Confirm Password:");

  // Create register button
  GtkWidget *register_button = gtk_button_new_with_label("Register");
  GtkWidget *status_label = gtk_label_new("");

  // Add widgets to the form
  gtk_box_pack_start(GTK_BOX(vbox), first_name_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), first_name_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), last_name_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), last_name_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), email_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), email_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), password_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), password_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), confirm_password_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), confirm_password_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), register_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

  // Store references to widgets for the callback
  g_object_set_data(G_OBJECT(register_button), "window", window);
  g_object_set_data(G_OBJECT(register_button), "first_name_entry", first_name_entry);
  g_object_set_data(G_OBJECT(register_button), "last_name_entry", last_name_entry);
  g_object_set_data(G_OBJECT(register_button), "email_entry", email_entry);
  g_object_set_data(G_OBJECT(register_button), "password_entry", password_entry);
  g_object_set_data(G_OBJECT(register_button), "confirm_password_entry", confirm_password_entry);

  // Connect the callback
  g_signal_connect(register_button, "clicked", G_CALLBACK(on_register_clicked), NULL);

  return vbox;
}

// Create login form
static GtkWidget *create_login_form()
{
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(vbox, 20);
  gtk_widget_set_margin_end(vbox, 20);
  gtk_widget_set_margin_top(vbox, 20);
  gtk_widget_set_margin_bottom(vbox, 20);

  // Create form fields
  GtkWidget *email_entry = gtk_entry_new();
  GtkWidget *password_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

  // Create labels
  GtkWidget *email_label = gtk_label_new("Email:");
  GtkWidget *password_label = gtk_label_new("Password:");

  // Create login button
  GtkWidget *login_button = gtk_button_new_with_label("Login");
  GtkWidget *status_label = gtk_label_new("");

  // Add widgets to the form
  gtk_box_pack_start(GTK_BOX(vbox), email_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), email_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), password_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), password_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), login_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

  // Store references to widgets for the callback
  g_object_set_data(G_OBJECT(login_button), "window", window);
  g_object_set_data(G_OBJECT(login_button), "email_entry", email_entry);
  g_object_set_data(G_OBJECT(login_button), "password_entry", password_entry);
  g_object_set_data(G_OBJECT(login_button), "status", status_label);

  // Connect the callback
  g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_clicked), NULL);

  return vbox;
}

// Main function
int main(int argc, char *argv[])
{
  // Initialize GTK
  gtk_init(&argc, &argv);

  // Connect to database
  conn = connect_to_database();
  if (PQstatus(conn) != CONNECTION_OK)
  {
    fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
    return 1;
  }

  // Create main window
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Authentication System");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  // Create notebook for tabs
  GtkWidget *notebook = gtk_notebook_new();
  gtk_container_add(GTK_CONTAINER(window), notebook);

  // Add registration and login tabs
  GtkWidget *registration_form = create_registration_form();
  GtkWidget *login_form = create_login_form();

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), registration_form, gtk_label_new("Register"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), login_form, gtk_label_new("Login"));

  // Apply dark theme
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider,
                                  "* { background-color: #000000; }"
                                  "window, box { background-color: #000000; }"
                                  "label { color: #ffffff; background-color: #000000; }"
                                  "entry { background-color: #000000; color: #808080; border: 1px solid #404040; border-radius: 4px; padding: 5px; }"
                                  "button { background-color: #d3d3d3; color: #000000; border-radius: 4px; padding: 8px 16px; }"
                                  "button:hover { background-color: #ffffff; }"
                                  "notebook { background-color: #000000; border: none; }"
                                  "notebook tab { background-color: #000000; color: #ffffff; padding: 8px 16px; }"
                                  "notebook tab:checked { background-color: #1a1a1a; border-bottom: 2px solid #d3d3d3; }",
                                  -1, NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Show window and start main loop
  gtk_widget_show_all(window);
  gtk_main();

  // Cleanup
  PQfinish(conn);
  return 0;
}