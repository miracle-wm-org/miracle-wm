/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

// miracle-wm-basic-error-reporter
//
// A small GTK4 + gtk4-layer-shell client that subscribes to the miracle-wm
// `config_errors` IPC event and displays configuration errors fullscreen. The
// user dismisses the report with the "X" button (or Escape), which closes the
// client. The compositor re-launches it the next time a configuration load
// produces errors.

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <json-c/json.h>

#include <cstring>
#include <glib-unix.h>
#include <libintl.h>
#include <locale.h>

#include "ipc.h"
#include "ipc_client.h"

#define _(s) gettext(s)

namespace
{
struct AppState
{
    int socketfd = -1;
    GtkApplication* app = nullptr;
    GtkWindow* window = nullptr;
    GtkWidget* list = nullptr;       // GtkListBox holding one row per error
    GtkWidget* empty_hint = nullptr; // shown when there are no errors
};

void clear_list(GtkWidget* list)
{
    GtkWidget* child = gtk_widget_get_first_child(list);
    while (child)
    {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(list), child);
        child = next;
    }
}

GtkWidget* build_error_row(struct json_object* error)
{
    struct json_object* tmp = nullptr;
    const char* filename = json_object_object_get_ex(error, "filename", &tmp) ? json_object_get_string(tmp) : "";
    int line = json_object_object_get_ex(error, "line", &tmp) ? json_object_get_int(tmp) : 0;
    int column = json_object_object_get_ex(error, "column", &tmp) ? json_object_get_int(tmp) : 0;
    const char* level = json_object_object_get_ex(error, "level", &tmp) ? json_object_get_string(tmp) : "error";
    const char* message = json_object_object_get_ex(error, "message", &tmp) ? json_object_get_string(tmp) : "";

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    GtkWidget* message_label = gtk_label_new(message);
    gtk_label_set_xalign(GTK_LABEL(message_label), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(message_label), TRUE);
    gtk_label_set_selectable(GTK_LABEL(message_label), TRUE);
    gtk_widget_add_css_class(message_label, g_strcmp0(level, "warning") == 0 ? "error-warning" : "error-error");
    gtk_box_append(GTK_BOX(box), message_label);

    char* location = g_strdup_printf("%s:%d:%d", filename, line, column);
    GtkWidget* location_label = gtk_label_new(location);
    g_free(location);
    gtk_label_set_xalign(GTK_LABEL(location_label), 0.0f);
    gtk_label_set_selectable(GTK_LABEL(location_label), TRUE);
    gtk_widget_add_css_class(location_label, "error-location");
    gtk_box_append(GTK_BOX(box), location_label);

    return box;
}

void update_errors(AppState* state, const char* payload)
{
    clear_list(state->list);

    struct json_object* arr = json_tokener_parse(payload);
    size_t count = 0;
    if (arr && json_object_is_type(arr, json_type_array))
    {
        count = json_object_array_length(arr);
        for (size_t i = 0; i < count; i++)
            gtk_list_box_append(GTK_LIST_BOX(state->list), build_error_row(json_object_array_get_idx(arr, i)));
    }
    if (arr)
        json_object_put(arr);

    if (count > 0)
    {
        gtk_widget_set_visible(state->empty_hint, FALSE);
        gtk_widget_set_visible(state->list, TRUE);
        gtk_window_present(state->window);
    }
    else
    {
        // A clean reload arrived; nothing to show.
        gtk_widget_set_visible(GTK_WIDGET(state->window), FALSE);
    }
}

gboolean on_socket_readable(gint fd, GIOCondition condition, gpointer user_data)
{
    (void)fd;
    auto* state = static_cast<AppState*>(user_data);
    if (condition & (G_IO_HUP | G_IO_ERR))
    {
        g_application_quit(G_APPLICATION(state->app));
        return G_SOURCE_REMOVE;
    }

    struct ipc_response* response = ipc_recv_response(state->socketfd);
    if (!response)
    {
        // Compositor closed the connection.
        g_application_quit(G_APPLICATION(state->app));
        return G_SOURCE_REMOVE;
    }

    if (response->type == static_cast<uint32_t>(IPC_EVENT_CONFIG_ERRORS))
        update_errors(state, response->payload);

    free_ipc_response(response);
    return G_SOURCE_CONTINUE;
}

void on_close_clicked(GtkButton* button, gpointer user_data)
{
    (void)button;
    auto* state = static_cast<AppState*>(user_data);
    g_application_quit(G_APPLICATION(state->app));
}

gboolean on_key_pressed(
    GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType modifiers, gpointer user_data)
{
    (void)controller;
    (void)keycode;
    (void)modifiers;
    auto* state = static_cast<AppState*>(user_data);
    if (keyval == GDK_KEY_Escape)
    {
        g_application_quit(G_APPLICATION(state->app));
        return TRUE;
    }
    return FALSE;
}

void load_css()
{
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background-color: rgba(20, 20, 24, 0.97); }"
        ".reporter-title { font-size: 20px; font-weight: bold; color: #ffffff; }"
        ".reporter-subtitle { color: #cfcfcf; }"
        ".error-error { color: #ff6b6b; font-weight: bold; }"
        ".error-warning { color: #ffd166; font-weight: bold; }"
        ".error-location { color: #9aa0a6; font-family: monospace; font-size: 12px; }"
        ".close-button { font-size: 18px; font-weight: bold; min-width: 36px; min-height: 36px; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

void activate(GtkApplication* app, gpointer user_data)
{
    auto* state = static_cast<AppState*>(user_data);
    state->app = app;

    load_css();

    GtkWidget* window = gtk_application_window_new(app);
    state->window = GTK_WINDOW(window);
    gtk_window_set_title(state->window, _("miracle-wm configuration errors"));

    gtk_layer_init_for_window(state->window);
    gtk_layer_set_layer(state->window, GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_namespace(state->window, "miracle-wm-error-reporter");
    gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    // Span the entire output, ignoring exclusive zones reserved by other
    // layer-shell surfaces (e.g. waybar). Without this, the compositor shrinks
    // this surface to avoid waybar's exclusive zone and "pushes it down".
    gtk_layer_set_exclusive_zone(state->window, -1);
    gtk_layer_set_keyboard_mode(state->window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(state->window, root);

    // Header: title + big close button.
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(header, 16);
    gtk_widget_set_margin_bottom(header, 8);
    gtk_widget_set_margin_start(header, 24);
    gtk_widget_set_margin_end(header, 24);

    GtkWidget* titles = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_hexpand(titles, TRUE);
    GtkWidget* title = gtk_label_new(_("Configuration errors"));
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_add_css_class(title, "reporter-title");
    GtkWidget* subtitle = gtk_label_new(_("miracle-wm could not fully load your configuration."));
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);
    gtk_widget_add_css_class(subtitle, "reporter-subtitle");
    gtk_box_append(GTK_BOX(titles), title);
    gtk_box_append(GTK_BOX(titles), subtitle);
    gtk_box_append(GTK_BOX(header), titles);

    GtkWidget* close_button = gtk_button_new_with_label("✕"); // ✕
    gtk_widget_set_tooltip_text(close_button, _("Dismiss"));
    gtk_widget_set_valign(close_button, GTK_ALIGN_START);
    gtk_widget_add_css_class(close_button, "close-button");
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_clicked), state);
    gtk_box_append(GTK_BOX(header), close_button);

    gtk_box_append(GTK_BOX(root), header);

    // Body: scrollable list of errors.
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_margin_start(scrolled, 24);
    gtk_widget_set_margin_end(scrolled, 24);
    gtk_widget_set_margin_bottom(scrolled, 24);

    state->list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state->list), GTK_SELECTION_NONE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), state->list);
    gtk_box_append(GTK_BOX(root), scrolled);

    state->empty_hint = gtk_label_new(_("No configuration errors."));
    gtk_widget_set_visible(state->empty_hint, FALSE);

    GtkEventController* key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed), state);
    gtk_widget_add_controller(window, key_controller);

    // Subscribe to config errors. The compositor replies with the current set of
    // errors immediately, then again on every subsequent reload.
    const char* sub_payload = "[\"config_errors\"]";
    uint32_t len = static_cast<uint32_t>(strlen(sub_payload));
    char* reply = ipc_single_command(state->socketfd, IPC_SUBSCRIBE, sub_payload, &len);
    free(reply);

    g_unix_fd_add(state->socketfd, static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR), on_socket_readable, state);

    // Keep the application alive even though the window starts hidden.
    g_application_hold(G_APPLICATION(app));
}
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    char* socket_path = get_socketpath();
    if (!socket_path)
    {
        g_printerr("%s\n", _("Unable to locate the miracle-wm IPC socket"));
        return 1;
    }

    AppState state {};
    state.socketfd = ipc_open_socket(socket_path);
    free(socket_path);

    GtkApplication* app = gtk_application_new("org.miracle_wm.BasicErrorReporter", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &state);
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    ipc_close_socket(state.socketfd);
    g_object_unref(app);
    return status;
}
