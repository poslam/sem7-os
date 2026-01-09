#include "http_server.h"

#include <cstring>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using SocketType = SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketType = int;
#endif

namespace lab5 {

static std::string make_response(const std::string& body, const std::string& status = "200 OK", const std::string& content_type = "application/json") {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << "\r\n";
    oss << "Content-Type: " << content_type << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    return oss.str();
}

bool HttpServer::start(int port, std::function<std::pair<std::string, std::string>(const std::string&)> handler, std::string& err) {
    handler_ = std::move(handler);
    running_ = true;
#ifdef _WIN32
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        err = "WSAStartup failed";
        return false;
    }
#endif
    thread_ = std::thread([this, port]() { run(port); });
    return true;
}

void HttpServer::stop() {
    running_ = false;
#ifdef _WIN32
    if (listen_fd_ != -1) {
        closesocket(static_cast<SocketType>(listen_fd_));
        listen_fd_ = -1;
    }
#else
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
#endif
    if (thread_.joinable()) thread_.join();
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::run(int port) {
    const int listen_port = port == 0 ? 8080 : port;
    SocketType s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0
#ifdef _WIN32
        || s == INVALID_SOCKET
#endif
    ) {
        return;
    }
    listen_fd_ = static_cast<long long>(s);
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(listen_port));
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
        listen_fd_ = -1;
        return;
    }
    listen(s, 5);

    while (running_) {
        SocketType client = accept(s, nullptr, nullptr);
#ifdef _WIN32
        if (client == INVALID_SOCKET) break;
#else
        if (client < 0) break;
#endif
        char buf[4096];
        int n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            std::string req(buf);
            std::string body = "{}";
            std::string content_type = "application/json";
            if (handler_) {
                auto res = handler_(req);
                body = res.first;
                content_type = res.second.empty() ? content_type : res.second;
            }
            auto resp = make_response(body, "200 OK", content_type);
            send(client, resp.c_str(), static_cast<int>(resp.size()), 0);
        }
#ifdef _WIN32
        closesocket(client);
#else
        close(client);
#endif
    }

#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
    listen_fd_ = -1;
}

}  // namespace lab5
