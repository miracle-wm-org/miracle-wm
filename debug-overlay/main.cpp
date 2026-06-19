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

// miracle-wm-debug-overlay
//
// A GTK4 + gtk4-layer-shell client that draws live debugging information on top
// of every window: each window's logical geometry and clip area, the window
// under the cursor, the cursor position, and a list of hidden windows. The
// overlay is transparent to input except over its HUD panel (which carries the
// "copy JSON" buttons), and spans every monitor.
//
// The compositor toggles this client on/off via `miraclemsg debug`. The client
// polls the compositor over the IPC socket (IPC_GET_DEBUG_STATE) for the data
// it draws.

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <json-c/json.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

#include "ipc.h"
#include "ipc_client.h"

namespace
{
// How often (ms) the overlay re-queries the compositor and redraws.
constexpr unsigned POLL_INTERVAL_MS = 60;

struct MonitorOverlay
{
    GtkWindow* window = nullptr;
    GtkWidget* drawing_area = nullptr;
    GtkWidget* hud = nullptr;      // the input-opaque panel
    GtkWidget* hud_list = nullptr; // GtkListBox of per-window rows
    GdkMonitor* monitor = nullptr; // borrowed
};

struct AppState
{
    int socketfd = -1;
    GtkApplication* app = nullptr;
    json_object* state = nullptr; // latest IPC_GET_DEBUG_STATE reply (owned)
    GList* overlays = nullptr;    // MonitorOverlay*
    std::string hud_signature;    // detects when the HUD needs rebuilding
};

AppState* g_app = nullptr;

// ---- small json-c helpers -------------------------------------------------

int json_int(json_object* obj, char const* key, int fallback = 0)
{
    json_object* tmp = nullptr;
    return (obj && json_object_object_get_ex(obj, key, &tmp)) ? json_object_get_int(tmp) : fallback;
}

int64_t json_int64(json_object* obj, char const* key, int64_t fallback = 0)
{
    json_object* tmp = nullptr;
    return (obj && json_object_object_get_ex(obj, key, &tmp)) ? json_object_get_int64(tmp) : fallback;
}

char const* json_str(json_object* obj, char const* key, char const* fallback = "")
{
    json_object* tmp = nullptr;
    return (obj && json_object_object_get_ex(obj, key, &tmp)) ? json_object_get_string(tmp) : fallback;
}

bool json_bool(json_object* obj, char const* key, bool fallback = false)
{
    json_object* tmp = nullptr;
    return (obj && json_object_object_get_ex(obj, key, &tmp)) ? json_object_get_boolean(tmp) : fallback;
}

json_object* json_obj(json_object* obj, char const* key)
{
    json_object* tmp = nullptr;
    return (obj && json_object_object_get_ex(obj, key, &tmp)) ? tmp : nullptr;
}

json_object* state_windows()
{
    return g_app->state ? json_obj(g_app->state, "windows") : nullptr;
}

// Reads a {x,y,width,height} rectangle stored under [key] on [obj].
struct Rect
{
    int x = 0, y = 0, w = 0, h = 0;
};

Rect read_rect(json_object* obj, char const* key)
{
    Rect r;
    if (json_object* rect = json_obj(obj, key))
    {
        r.x = json_int(rect, "x");
        r.y = json_int(rect, "y");
        r.w = json_int(rect, "width");
        r.h = json_int(rect, "height");
    }
    return r;
}

// ---- clipboard ------------------------------------------------------------

void copy_text_to_clipboard(GtkWidget* widget, char const* text)
{
    GdkClipboard* clipboard = gtk_widget_get_clipboard(widget);
    gdk_clipboard_set(clipboard, G_TYPE_STRING, text);
}

void on_copy_window_clicked(GtkButton* button, gpointer)
{
    auto const target = static_cast<uint64_t>(
        GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(button), "debug-id")));

    json_object* windows = state_windows();
    if (!windows)
        return;

    size_t const count = json_object_array_length(windows);
    for (size_t i = 0; i < count; i++)
    {
        json_object* w = json_object_array_get_idx(windows, i);
        if (static_cast<uint64_t>(json_int64(w, "debug_id", -1)) == target)
        {
            char const* json = json_object_to_json_string_ext(w, JSON_C_TO_STRING_PRETTY);
            copy_text_to_clipboard(GTK_WIDGET(button), json);
            return;
        }
    }
}

void on_copy_all_clicked(GtkButton* button, gpointer)
{
    if (!g_app->state)
        return;
    char const* json = json_object_to_json_string_ext(g_app->state, JSON_C_TO_STRING_PRETTY);
    copy_text_to_clipboard(GTK_WIDGET(button), json);
}

// ---- drawing --------------------------------------------------------------

void set_source_rgba(cairo_t* cr, double r, double g, double b, double a)
{
    cairo_set_source_rgba(cr, r, g, b, a);
}

void draw_label(cairo_t* cr, double x, double y, std::string const& text)
{
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, text.c_str(), &ext);

    // Backing plate for legibility over any window content.
    set_source_rgba(cr, 0.0, 0.0, 0.0, 0.65);
    cairo_rectangle(cr, x - 2, y - ext.height - 3, ext.width + 6, ext.height + 6);
    cairo_fill(cr);

    set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_move_to(cr, x + 1, y - 1);
    cairo_show_text(cr, text.c_str());
}

void draw_cb(GtkDrawingArea*, cairo_t* cr, int width, int height, gpointer user_data)
{
    auto* overlay = static_cast<MonitorOverlay*>(user_data);
    if (!g_app->state)
        return;

    GdkRectangle geo;
    gdk_monitor_get_geometry(overlay->monitor, &geo);

    int64_t const under_cursor = json_int64(g_app->state, "window_under_cursor", -1);

    if (json_object* windows = state_windows())
    {
        size_t const count = json_object_array_length(windows);
        for (size_t i = 0; i < count; i++)
        {
            json_object* w = json_object_array_get_idx(windows, i);
            Rect const r = read_rect(w, "rect");

            // Cull windows that don't touch this monitor.
            if (r.x + r.w < geo.x || r.x > geo.x + geo.width || r.y + r.h < geo.y || r.y > geo.y + geo.height)
                continue;

            double const lx = r.x - geo.x;
            double const ly = r.y - geo.y;
            int64_t const id = json_int64(w, "debug_id", -1);
            bool const focused = json_bool(w, "focused");
            bool const visible = json_bool(w, "visible", true);

            // Outline colour: yellow under cursor, green focused, grey hidden, cyan otherwise.
            if (id == under_cursor && under_cursor >= 0)
                set_source_rgba(cr, 1.0, 0.85, 0.1, 0.95);
            else if (focused)
                set_source_rgba(cr, 0.3, 1.0, 0.4, 0.9);
            else if (!visible)
                set_source_rgba(cr, 0.5, 0.5, 0.55, 0.5);
            else
                set_source_rgba(cr, 0.2, 0.8, 1.0, 0.85);

            cairo_set_line_width(cr, 2.0);
            cairo_set_dash(cr, nullptr, 0, 0);
            cairo_rectangle(cr, lx + 1, ly + 1, r.w - 2, r.h - 2);
            cairo_stroke(cr);

            // Clip / visible area (relative to the logical rect), drawn dashed.
            Rect const clip = read_rect(w, "window_rect");
            if (clip.w > 0 && clip.h > 0 && (clip.x != 0 || clip.y != 0 || clip.w != r.w || clip.h != r.h))
            {
                double const dashes[] = { 6.0, 4.0 };
                set_source_rgba(cr, 1.0, 0.4, 0.8, 0.9);
                cairo_set_line_width(cr, 1.5);
                cairo_set_dash(cr, dashes, 2, 0);
                cairo_rectangle(cr, lx + clip.x + 0.5, ly + clip.y + 0.5, clip.w - 1, clip.h - 1);
                cairo_stroke(cr);
                cairo_set_dash(cr, nullptr, 0, 0);
            }

            // Input region(s): where the surface actually accepts pointer input.
            // Drawn in orange (translucent fill + outline). An empty input_region
            // means the whole surface accepts input, so fall back to input_bounds.
            {
                cairo_set_line_width(cr, 1.5);
                bool drew_region = false;
                if (json_object* regions = json_obj(w, "input_region"))
                {
                    size_t const rn = json_object_array_length(regions);
                    for (size_t k = 0; k < rn; k++)
                    {
                        json_object* ir = json_object_array_get_idx(regions, k);
                        int const iw = json_int(ir, "width");
                        int const ih = json_int(ir, "height");
                        if (iw <= 0 || ih <= 0)
                            continue;
                        double const ilx = json_int(ir, "x") - geo.x;
                        double const ily = json_int(ir, "y") - geo.y;
                        set_source_rgba(cr, 1.0, 0.55, 0.0, 0.12);
                        cairo_rectangle(cr, ilx, ily, iw, ih);
                        cairo_fill(cr);
                        set_source_rgba(cr, 1.0, 0.55, 0.0, 0.9);
                        cairo_rectangle(cr, ilx + 0.5, ily + 0.5, iw - 1, ih - 1);
                        cairo_stroke(cr);
                        drew_region = true;
                    }
                }
                if (!drew_region)
                {
                    Rect const ib = read_rect(w, "input_bounds");
                    if (ib.w > 0 && ib.h > 0)
                    {
                        set_source_rgba(cr, 1.0, 0.55, 0.0, 0.9);
                        cairo_rectangle(cr, (ib.x - geo.x) + 0.5, (ib.y - geo.y) + 0.5, ib.w - 1, ib.h - 1);
                        cairo_stroke(cr);
                    }
                }
            }

            // Colocated label: id, app id, geometry, and clip if it differs.
            std::string label = "#" + std::to_string(id) + " " + json_str(w, "app_id");
            std::string geom = std::to_string(r.x) + "," + std::to_string(r.y) + " " + std::to_string(r.w) + "x" + std::to_string(r.h);
            draw_label(cr, lx + 4, ly + 16, label);
            draw_label(cr, lx + 4, ly + 34, geom);
        }
    }

    // Cursor crosshair + coordinate readout.
    if (json_object* cursor = json_obj(g_app->state, "cursor"))
    {
        int const cx = json_int(cursor, "x");
        int const cy = json_int(cursor, "y");
        double const lx = cx - geo.x;
        double const ly = cy - geo.y;
        if (lx >= 0 && lx <= width && ly >= 0 && ly <= height)
        {
            set_source_rgba(cr, 1.0, 0.2, 0.2, 0.9);
            cairo_set_line_width(cr, 1.0);
            cairo_move_to(cr, lx - 12, ly);
            cairo_line_to(cr, lx + 12, ly);
            cairo_move_to(cr, lx, ly - 12);
            cairo_line_to(cr, lx, ly + 12);
            cairo_stroke(cr);
            draw_label(cr, lx + 14, ly + 14, std::to_string(cx) + "," + std::to_string(cy));
        }
    }
}

// ---- HUD ------------------------------------------------------------------

GtkWidget* build_window_row(json_object* w)
{
    int64_t const id = json_int64(w, "debug_id", -1);
    bool const visible = json_bool(w, "visible", true);

    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(row, 2);
    gtk_widget_set_margin_bottom(row, 2);

    std::string text = "#" + std::to_string(id) + "  " + json_str(w, "app_id");
    char const* output = json_str(w, "output");
    if (output && *output)
        text += "  [" + std::string(output) + "]";
    if (!visible)
        text += "  (hidden)";

    GtkWidget* label = gtk_label_new(text.c_str());
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_add_css_class(label, visible ? "hud-window" : "hud-window-hidden");
    gtk_box_append(GTK_BOX(row), label);

    GtkWidget* copy = gtk_button_new_with_label("copy");
    gtk_widget_add_css_class(copy, "hud-copy");
    g_object_set_data(G_OBJECT(copy), "debug-id", GSIZE_TO_POINTER(static_cast<size_t>(id)));
    g_signal_connect(copy, "clicked", G_CALLBACK(on_copy_window_clicked), nullptr);
    gtk_box_append(GTK_BOX(row), copy);

    return row;
}

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

// Rebuilds every overlay's HUD list, but only when the set of windows changed
// (so per-frame polling doesn't destroy button hover/press state).
void update_huds()
{
    json_object* windows = state_windows();

    std::string signature;
    if (windows)
    {
        size_t const count = json_object_array_length(windows);
        for (size_t i = 0; i < count; i++)
        {
            json_object* w = json_object_array_get_idx(windows, i);
            signature += std::to_string(json_int64(w, "debug_id", -1));
            signature += json_bool(w, "visible", true) ? "+" : "-";
            signature += ';';
        }
    }

    if (signature == g_app->hud_signature)
        return;
    g_app->hud_signature = signature;

    for (GList* l = g_app->overlays; l != nullptr; l = l->next)
    {
        auto* o = static_cast<MonitorOverlay*>(l->data);
        clear_list(o->hud_list);
        if (!windows)
            continue;
        size_t const count = json_object_array_length(windows);
        for (size_t i = 0; i < count; i++)
            gtk_list_box_append(GTK_LIST_BOX(o->hud_list), build_window_row(json_object_array_get_idx(windows, i)));
    }
}

// Keeps the overlay click-through everywhere except over its HUD panel.
void update_input_region(MonitorOverlay* o)
{
    GtkNative* native = gtk_widget_get_native(GTK_WIDGET(o->window));
    if (!native)
        return;
    GdkSurface* surface = gtk_native_get_surface(native);
    if (!surface)
        return;

    cairo_region_t* region = cairo_region_create(); // empty -> fully click-through
    graphene_rect_t bounds;
    if (o->hud && gtk_widget_get_mapped(o->hud) && gtk_widget_compute_bounds(o->hud, GTK_WIDGET(o->window), &bounds))
    {
        cairo_rectangle_int_t r = {
            static_cast<int>(std::floor(bounds.origin.x)),
            static_cast<int>(std::floor(bounds.origin.y)),
            static_cast<int>(std::ceil(bounds.size.width)),
            static_cast<int>(std::ceil(bounds.size.height))
        };
        cairo_region_union_rectangle(region, &r);
    }
    gdk_surface_set_input_region(surface, region);
    cairo_region_destroy(region);
}

// ---- polling --------------------------------------------------------------

gboolean on_tick(gpointer)
{
    uint32_t len = 0;
    char* response = ipc_single_command(g_app->socketfd, IPC_GET_DEBUG_STATE, "", &len);
    if (!response)
    {
        // Compositor closed the connection (overlay toggled off / shutting down).
        g_application_quit(G_APPLICATION(g_app->app));
        return G_SOURCE_REMOVE;
    }

    if (json_object* parsed = json_tokener_parse(response))
    {
        if (g_app->state)
            json_object_put(g_app->state);
        g_app->state = parsed;
    }
    free(response);

    update_huds();
    for (GList* l = g_app->overlays; l != nullptr; l = l->next)
    {
        auto* o = static_cast<MonitorOverlay*>(l->data);
        gtk_widget_queue_draw(o->drawing_area);
        update_input_region(o);
    }
    return G_SOURCE_CONTINUE;
}

// ---- setup ----------------------------------------------------------------

void on_window_realize(GtkWidget* widget, gpointer user_data)
{
    (void)widget;
    update_input_region(static_cast<MonitorOverlay*>(user_data));
}

MonitorOverlay* create_overlay(GtkApplication* app, GdkMonitor* monitor)
{
    auto* o = g_new0(MonitorOverlay, 1);
    o->monitor = monitor;

    GtkWidget* window = gtk_application_window_new(app);
    o->window = GTK_WINDOW(window);
    gtk_widget_add_css_class(window, "debug-overlay-window");

    gtk_layer_init_for_window(o->window);
    gtk_layer_set_monitor(o->window, monitor);
    gtk_layer_set_layer(o->window, GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_namespace(o->window, "miracle-wm-debug-overlay");
    gtk_layer_set_anchor(o->window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(o->window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(o->window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(o->window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(o->window, -1);
    gtk_layer_set_keyboard_mode(o->window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);

    GtkWidget* overlay = gtk_overlay_new();

    o->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(o->drawing_area, TRUE);
    gtk_widget_set_vexpand(o->drawing_area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(o->drawing_area), draw_cb, o, nullptr);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), o->drawing_area);

    // HUD panel (top-right): title, "copy all" button, scrollable window list.
    o->hud = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(o->hud, "hud");
    gtk_widget_set_halign(o->hud, GTK_ALIGN_END);
    gtk_widget_set_valign(o->hud, GTK_ALIGN_START);
    gtk_widget_set_margin_top(o->hud, 12);
    gtk_widget_set_margin_end(o->hud, 12);
    gtk_widget_set_size_request(o->hud, 320, -1);

    GtkWidget* title = gtk_label_new("miracle-wm debug overlay");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_add_css_class(title, "hud-title");
    gtk_box_append(GTK_BOX(o->hud), title);

    GtkWidget* copy_all = gtk_button_new_with_label("Copy debug state JSON");
    gtk_widget_add_css_class(copy_all, "hud-copy");
    g_signal_connect(copy_all, "clicked", G_CALLBACK(on_copy_all_clicked), nullptr);
    gtk_box_append(GTK_BOX(o->hud), copy_all);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, FALSE);
    gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scrolled), 480);
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrolled), TRUE);
    o->hud_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(o->hud_list), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(o->hud_list, "hud-list");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), o->hud_list);
    gtk_box_append(GTK_BOX(o->hud), scrolled);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), o->hud);
    gtk_window_set_child(o->window, overlay);

    g_signal_connect(window, "realize", G_CALLBACK(on_window_realize), o);
    gtk_window_present(o->window);
    return o;
}

void destroy_overlays()
{
    for (GList* l = g_app->overlays; l != nullptr; l = l->next)
    {
        auto* o = static_cast<MonitorOverlay*>(l->data);
        gtk_window_destroy(o->window);
        g_free(o);
    }
    g_list_free(g_app->overlays);
    g_app->overlays = nullptr;
}

void build_overlays()
{
    destroy_overlays();
    g_app->hud_signature.clear();

    GdkDisplay* display = gdk_display_get_default();
    GListModel* monitors = gdk_display_get_monitors(display);
    guint const n = g_list_model_get_n_items(monitors);
    for (guint i = 0; i < n; i++)
    {
        auto* monitor = static_cast<GdkMonitor*>(g_list_model_get_item(monitors, i));
        g_app->overlays = g_list_append(g_app->overlays, create_overlay(g_app->app, monitor));
        g_object_unref(monitor); // create_overlay keeps a borrowed pointer; the display owns the monitor
    }
}

void on_monitors_changed(GListModel*, guint, guint, guint, gpointer)
{
    build_overlays();
}

void load_css()
{
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        ".debug-overlay-window { background-color: transparent; }"
        ".hud { background-color: rgba(20, 20, 24, 0.92); border-radius: 8px; padding: 10px; color: #eaeaea; }"
        ".hud-title { font-weight: bold; color: #ffffff; margin-bottom: 4px; }"
        ".hud-window { font-family: monospace; font-size: 12px; color: #d6e9ff; }"
        ".hud-window-hidden { font-family: monospace; font-size: 12px; color: #8a8f98; }"
        ".hud-copy { padding: 0 8px; min-height: 22px; }"
        ".hud-list { background: transparent; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

void activate(GtkApplication* app, gpointer)
{
    g_app->app = app;
    load_css();
    build_overlays();

    GListModel* monitors = gdk_display_get_monitors(gdk_display_get_default());
    g_signal_connect(monitors, "items-changed", G_CALLBACK(on_monitors_changed), nullptr);

    g_timeout_add(POLL_INTERVAL_MS, on_tick, nullptr);

    // The windows start mapped; keep the application alive regardless.
    g_application_hold(G_APPLICATION(app));
}
}

int main(int argc, char** argv)
{
    char* socket_path = get_socketpath();
    if (!socket_path)
    {
        g_printerr("Unable to locate the miracle-wm IPC socket\n");
        return 1;
    }

    AppState state {};
    g_app = &state;
    state.socketfd = ipc_open_socket(socket_path);
    free(socket_path);

    GtkApplication* app = gtk_application_new("org.miracle_wm.DebugOverlay", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
    int const status = g_application_run(G_APPLICATION(app), argc, argv);

    if (state.state)
        json_object_put(state.state);
    ipc_close_socket(state.socketfd);
    g_object_unref(app);
    g_app = nullptr;
    return status;
}
