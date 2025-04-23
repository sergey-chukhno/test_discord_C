#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "config.h"

// Function to show error dialog
void show_error_dialog(GtkWidget *parent, const char *message);

// Function to show success dialog
void show_success_dialog(GtkWidget *parent, const char *message);

#endif // GUI_H