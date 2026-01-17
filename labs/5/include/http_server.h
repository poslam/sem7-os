#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

// Start HTTP server on specified port
int http_server_start(int port);

// Stop HTTP server
void http_server_stop(void);

// Server main loop (blocking)
void http_server_run(void);

#endif // HTTP_SERVER_H
