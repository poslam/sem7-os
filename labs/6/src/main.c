#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "api_client.h"

// Application structure
typedef struct {
    GtkWidget *window;
    GtkWidget *current_temp_label;
    GtkWidget *timestamp_label;
    GtkWidget *hourly_canvas;
    GtkWidget *daily_canvas;
    GtkWidget *period_combo;
    
    TempDataArray *hourly_data;
    TempDataArray *daily_data;
    double current_temp;
    time_t current_time;
} AppData;

static AppData app_data = {0};

// Draw hourly chart
static gboolean draw_hourly_chart(GtkWidget *widget, cairo_t *cr, gpointer data) {
    TempDataArray *array = app_data.hourly_data;
    
    if (!array || array->count == 0) {
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_move_to(cr, 50, 150);
        cairo_show_text(cr, "No data available");
        return FALSE;
    }

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    
    // Margins
    int margin_left = 60;
    int margin_right = 20;
    int margin_top = 20;
    int margin_bottom = 40;
    
    int chart_width = width - margin_left - margin_right;
    int chart_height = height - margin_top - margin_bottom;
    
    // Find min and max temperature
    double min_temp = array->data[0].temperature;
    double max_temp = array->data[0].temperature;
    
    for (int i = 1; i < array->count; i++) {
        if (array->data[i].temperature < min_temp) min_temp = array->data[i].temperature;
        if (array->data[i].temperature > max_temp) max_temp = array->data[i].temperature;
    }
    
    double temp_range = max_temp - min_temp;
    if (temp_range < 1.0) temp_range = 1.0;
    
    // Draw background
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Draw grid
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_set_line_width(cr, 1.0);
    
    for (int i = 0; i <= 5; i++) {
        double y = margin_top + (chart_height * i) / 5.0;
        cairo_move_to(cr, margin_left, y);
        cairo_line_to(cr, margin_left + chart_width, y);
        cairo_stroke(cr);
    }
    
    // Draw axes
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, margin_left, margin_top);
    cairo_line_to(cr, margin_left, margin_top + chart_height);
    cairo_line_to(cr, margin_left + chart_width, margin_top + chart_height);
    cairo_stroke(cr);
    
    // Draw temperature labels
    cairo_set_font_size(cr, 10);
    for (int i = 0; i <= 5; i++) {
        double temp = max_temp - (temp_range * i / 5.0);
        double y = margin_top + (chart_height * i) / 5.0;
        
        char label[32];
        snprintf(label, sizeof(label), "%.1f°C", temp);
        
        cairo_move_to(cr, 5, y + 4);
        cairo_show_text(cr, label);
    }
    
    // Draw line chart
    cairo_set_source_rgb(cr, 0.2, 0.4, 0.8);
    cairo_set_line_width(cr, 2.0);
    
    for (int i = 0; i < array->count; i++) {
        double x = margin_left + (chart_width * i) / (double)(array->count - 1);
        double normalized = (array->data[i].temperature - min_temp) / temp_range;
        double y = margin_top + chart_height - (chart_height * normalized);
        
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);
    
    // Draw points
    cairo_set_source_rgb(cr, 0.8, 0.2, 0.2);
    for (int i = 0; i < array->count; i++) {
        double x = margin_left + (chart_width * i) / (double)(array->count - 1);
        double normalized = (array->data[i].temperature - min_temp) / temp_range;
        double y = margin_top + chart_height - (chart_height * normalized);
        
        cairo_arc(cr, x, y, 3, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    // Draw title
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, width / 2 - 80, 15);
    cairo_show_text(cr, "Hourly Temperature (24h)");
    
    return FALSE;
}

// Draw daily chart
static gboolean draw_daily_chart(GtkWidget *widget, cairo_t *cr, gpointer data) {
    TempDataArray *array = app_data.daily_data;
    
    if (!array || array->count == 0) {
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_move_to(cr, 50, 150);
        cairo_show_text(cr, "No data available");
        return FALSE;
    }

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    
    int margin_left = 60;
    int margin_right = 20;
    int margin_top = 20;
    int margin_bottom = 40;
    
    int chart_width = width - margin_left - margin_right;
    int chart_height = height - margin_top - margin_bottom;
    
    // Find min and max
    double min_temp = array->data[0].temperature;
    double max_temp = array->data[0].temperature;
    
    for (int i = 1; i < array->count; i++) {
        if (array->data[i].temperature < min_temp) min_temp = array->data[i].temperature;
        if (array->data[i].temperature > max_temp) max_temp = array->data[i].temperature;
    }
    
    double temp_range = max_temp - min_temp;
    if (temp_range < 1.0) temp_range = 1.0;
    
    // Draw background
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Draw grid
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_set_line_width(cr, 1.0);
    
    for (int i = 0; i <= 5; i++) {
        double y = margin_top + (chart_height * i) / 5.0;
        cairo_move_to(cr, margin_left, y);
        cairo_line_to(cr, margin_left + chart_width, y);
        cairo_stroke(cr);
    }
    
    // Draw axes
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, margin_left, margin_top);
    cairo_line_to(cr, margin_left, margin_top + chart_height);
    cairo_line_to(cr, margin_left + chart_width, margin_top + chart_height);
    cairo_stroke(cr);
    
    // Draw temperature labels
    cairo_set_font_size(cr, 10);
    for (int i = 0; i <= 5; i++) {
        double temp = max_temp - (temp_range * i / 5.0);
        double y = margin_top + (chart_height * i) / 5.0;
        
        char label[32];
        snprintf(label, sizeof(label), "%.1f°C", temp);
        
        cairo_move_to(cr, 5, y + 4);
        cairo_show_text(cr, label);
    }
    
    // Draw bars
    cairo_set_source_rgb(cr, 0.4, 0.2, 0.6);
    
    double bar_width = chart_width / (double)array->count * 0.8;
    
    for (int i = 0; i < array->count; i++) {
        double x = margin_left + (chart_width * i) / (double)array->count;
        double normalized = (array->data[i].temperature - min_temp) / temp_range;
        double bar_height = chart_height * normalized;
        double y = margin_top + chart_height - bar_height;
        
        cairo_rectangle(cr, x, y, bar_width, bar_height);
        cairo_fill(cr);
    }
    
    // Draw title
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, width / 2 - 80, 15);
    cairo_show_text(cr, "Daily Average (30 days)");
    
    return FALSE;
}

// Update data from API
static gboolean update_data(gpointer user_data) {
    // Get current temperature
    if (api_get_current_temp(&app_data.current_temp, &app_data.current_time) == 0) {
        char temp_str[64];
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", app_data.current_temp);
        gtk_label_set_text(GTK_LABEL(app_data.current_temp_label), temp_str);
        
        time_t t = app_data.current_time;
        struct tm *tm_info = localtime(&t);
        char time_str[128];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        gtk_label_set_text(GTK_LABEL(app_data.timestamp_label), time_str);
    }
    
    // Get hourly data
    if (app_data.hourly_data) {
        api_free_data_array(app_data.hourly_data);
    }
    app_data.hourly_data = api_get_hourly_data();
    gtk_widget_queue_draw(app_data.hourly_canvas);
    
    // Get daily data
    if (app_data.daily_data) {
        api_free_data_array(app_data.daily_data);
    }
    app_data.daily_data = api_get_daily_data();
    gtk_widget_queue_draw(app_data.daily_canvas);
    
    return TRUE; // Continue calling this function
}

// Refresh button callback
static void on_refresh_clicked(GtkWidget *widget, gpointer data) {
    update_data(NULL);
}

// Main window
static void activate(GtkApplication *app, gpointer user_data) {
    // Create main window
    app_data.window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(app_data.window), "Temperature Monitor");
    gtk_window_set_default_size(GTK_WINDOW(app_data.window), 1000, 700);
    
    // Create main vertical box
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app_data.window), main_vbox);
    
    // Header
    GtkWidget *header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label), 
                        "<span font='20' weight='bold'>🌡️ Temperature Monitoring System</span>");
    gtk_box_pack_start(GTK_BOX(main_vbox), header_label, FALSE, FALSE, 10);
    
    // Current temperature panel
    GtkWidget *current_frame = gtk_frame_new("Current Temperature");
    gtk_box_pack_start(GTK_BOX(main_vbox), current_frame, FALSE, FALSE, 5);
    
    GtkWidget *current_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(current_frame), current_vbox);
    gtk_container_set_border_width(GTK_CONTAINER(current_vbox), 10);
    
    app_data.current_temp_label = gtk_label_new("--°C");
    gtk_label_set_markup(GTK_LABEL(app_data.current_temp_label), 
                        "<span font='36' weight='bold' color='#4040FF'>--°C</span>");
    gtk_box_pack_start(GTK_BOX(current_vbox), app_data.current_temp_label, FALSE, FALSE, 5);
    
    app_data.timestamp_label = gtk_label_new("Loading...");
    gtk_box_pack_start(GTK_BOX(current_vbox), app_data.timestamp_label, FALSE, FALSE, 5);
    
    // Refresh button
    GtkWidget *refresh_btn = gtk_button_new_with_label("🔄 Refresh Data");
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(current_vbox), refresh_btn, FALSE, FALSE, 5);
    
    // Charts container
    GtkWidget *charts_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), charts_hbox, TRUE, TRUE, 5);
    
    // Hourly chart
    GtkWidget *hourly_frame = gtk_frame_new("Hourly Data");
    gtk_box_pack_start(GTK_BOX(charts_hbox), hourly_frame, TRUE, TRUE, 5);
    
    app_data.hourly_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(app_data.hourly_canvas, 400, 300);
    gtk_container_add(GTK_CONTAINER(hourly_frame), app_data.hourly_canvas);
    g_signal_connect(app_data.hourly_canvas, "draw", G_CALLBACK(draw_hourly_chart), NULL);
    
    // Daily chart
    GtkWidget *daily_frame = gtk_frame_new("Daily Data");
    gtk_box_pack_start(GTK_BOX(charts_hbox), daily_frame, TRUE, TRUE, 5);
    
    app_data.daily_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(app_data.daily_canvas, 400, 300);
    gtk_container_add(GTK_CONTAINER(daily_frame), app_data.daily_canvas);
    g_signal_connect(app_data.daily_canvas, "draw", G_CALLBACK(draw_daily_chart), NULL);
    
    // Show all widgets
    gtk_widget_show_all(app_data.window);
    
    // Initial data load
    update_data(NULL);
    
    // Auto-refresh every 30 seconds
    g_timeout_add_seconds(30, update_data, NULL);
}

int main(int argc, char *argv[]) {
    const char *server_url = "http://localhost:8080";
    
    if (argc > 1) {
        server_url = argv[1];
    }
    
    // Initialize API client
    if (api_client_init(server_url) != 0) {
        fprintf(stderr, "Failed to initialize API client\n");
        return 1;
    }
    
    printf("Connecting to server: %s\n", server_url);
    
    // Create GTK application
    GtkApplication *app = gtk_application_new("com.tempmonitor.gui", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // Cleanup
    if (app_data.hourly_data) api_free_data_array(app_data.hourly_data);
    if (app_data.daily_data) api_free_data_array(app_data.daily_data);
    
    g_object_unref(app);
    api_client_cleanup();
    
    return status;
}
