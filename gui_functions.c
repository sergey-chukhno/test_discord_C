#include "gui.h"
#include <gtk/gtk.h>

void show_error_dialog(GtkWidget *parent, const char *message)
{
  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_OK,
                                             "%s",
                                             message);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void show_success_dialog(GtkWidget *parent, const char *message)
{
  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "%s",
                                             message);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}