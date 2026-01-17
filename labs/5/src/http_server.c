#include "http_server.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

static int server_socket = -1;
static int running = 0;

// Helper function to send HTTP response
static void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    char header[1024];
    snprintf(header, sizeof(header),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Connection: close\r\n"
             "\r\n",
             status, content_type, strlen(body));
    
    send(client_socket, header, strlen(header), 0);
    send(client_socket, body, strlen(body), 0);
}

// Handle HTTP requests
static void handle_request(int client_socket, const char *request) {
    char method[16], path[256];
    sscanf(request, "%s %s", method, path);

    printf("Request: %s %s\n", method, path);

    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        // Serve the web interface
        FILE *f = fopen("web/index.html", "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            char *content = malloc(size + 1);
            fread(content, 1, size, f);
            content[size] = '\0';
            fclose(f);
            
            send_response(client_socket, "200 OK", "text/html", content);
            free(content);
        } else {
            send_response(client_socket, "404 Not Found", "text/plain", "File not found");
        }
    }
    else if (strcmp(path, "/api/current") == 0) {
        // Get current temperature
        double temp;
        time_t timestamp;
        
        if (db_get_current_temp(&temp, &timestamp) == 0) {
            char json[512];
            snprintf(json, sizeof(json),
                    "{\"temperature\": %.2f, \"timestamp\": %ld}",
                    temp, (long)timestamp);
            send_response(client_socket, "200 OK", "application/json", json);
        } else {
            send_response(client_socket, "404 Not Found", "application/json", 
                         "{\"error\": \"No data available\"}");
        }
    }
    else if (strncmp(path, "/api/hourly", 11) == 0) {
        // Get hourly data for last 24 hours
        time_t now = time(NULL);
        time_t start = now - (24 * 3600);
        
        double *temps = NULL;
        time_t *timestamps = NULL;
        int count = 0;
        
        if (db_get_hourly_data(start, now, &temps, &timestamps, &count) == 0 && count > 0) {
            char *json = malloc(count * 100 + 100);
            strcpy(json, "{\"data\": [");
            
            for (int i = 0; i < count; i++) {
                char item[100];
                snprintf(item, sizeof(item),
                        "%s{\"temperature\": %.2f, \"timestamp\": %ld}",
                        i > 0 ? "," : "", temps[i], (long)timestamps[i]);
                strcat(json, item);
            }
            strcat(json, "]}");
            
            send_response(client_socket, "200 OK", "application/json", json);
            free(json);
            free(temps);
            free(timestamps);
        } else {
            send_response(client_socket, "200 OK", "application/json", "{\"data\": []}");
        }
    }
    else if (strncmp(path, "/api/daily", 10) == 0) {
        // Get daily data for last 30 days
        time_t now = time(NULL);
        time_t start = now - (30 * 24 * 3600);
        
        double *temps = NULL;
        time_t *timestamps = NULL;
        int count = 0;
        
        if (db_get_daily_data(start, now, &temps, &timestamps, &count) == 0 && count > 0) {
            char *json = malloc(count * 100 + 100);
            strcpy(json, "{\"data\": [");
            
            for (int i = 0; i < count; i++) {
                char item[100];
                snprintf(item, sizeof(item),
                        "%s{\"temperature\": %.2f, \"timestamp\": %ld}",
                        i > 0 ? "," : "", temps[i], (long)timestamps[i]);
                strcat(json, item);
            }
            strcat(json, "]}");
            
            send_response(client_socket, "200 OK", "application/json", json);
            free(json);
            free(temps);
            free(timestamps);
        } else {
            send_response(client_socket, "200 OK", "application/json", "{\"data\": []}");
        }
    }
    else {
        send_response(client_socket, "404 Not Found", "text/plain", "Not Found");
    }
}

int http_server_start(int port) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        fprintf(stderr, "Failed to create socket\n");
        return -1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to bind socket\n");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 10) < 0) {
        fprintf(stderr, "Failed to listen on socket\n");
        close(server_socket);
        return -1;
    }

    running = 1;
    printf("HTTP server started on port %d\n", port);
    return 0;
}

void http_server_stop(void) {
    running = 0;
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

void http_server_run(void) {
    char buffer[4096];
    
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                fprintf(stderr, "Failed to accept connection\n");
            }
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            handle_request(client_socket, buffer);
        }

        close(client_socket);
    }
}
