#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kDefaultPort = 8273;
constexpr int kReadTimeoutSeconds = 2;

std::vector<std::string> splitCommand(const std::string& command) {
    std::istringstream iss(command);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part) {
        parts.push_back(part);
    }
    return parts;
}

std::string toResp(const std::string& command) {
    const auto parts = splitCommand(command);
    std::ostringstream out;
    out << "*" << parts.size() << "\r\n";
    for (const auto& part : parts) {
        out << "$" << part.size() << "\r\n" << part << "\r\n";
    }
    return out.str();
}

std::string toInlineCommand(const std::string& command) {
    if (command.size() >= 2 && command.substr(command.size() - 2) == "\r\n") {
        return command;
    }
    return command + "\r\n";
}

int connectToServer(const std::string& host, int port) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const std::string portText = std::to_string(port);
    const int status = getaddrinfo(host.c_str(), portText.c_str(), &hints, &result);
    if (status != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(status) << "\n";
        return -1;
    }

    int sock = -1;
    for (addrinfo* addr = result; addr != nullptr; addr = addr->ai_next) {
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sock < 0) {
            continue;
        }

        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(result);
    return sock;
}

bool readResponse(int sock, std::string* response) {
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(sock, &readFds);

    timeval timeout{};
    timeout.tv_sec = kReadTimeoutSeconds;

    const int ready = select(sock + 1, &readFds, nullptr, nullptr, &timeout);
    if (ready <= 0) {
        return false;
    }

    char buffer[4096];
    const ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        return false;
    }

    response->assign(buffer, bytes);
    return true;
}

bool sendCommand(int sock, const std::string& command, bool useResp) {
    const std::string payload = useResp ? toResp(command) : toInlineCommand(command);
    const ssize_t sent = send(sock, payload.c_str(), payload.size(), 0);
    return sent == static_cast<ssize_t>(payload.size());
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program
              << " [--host HOST] [--port PORT] [--resp] [command...]\n"
              << "Examples:\n"
              << "  " << program << " PING\n"
              << "  " << program << " --resp SET name saurabh\n"
              << "  " << program << " --host 127.0.0.1 --port 8273\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = kDefaultPort;
    bool useResp = false;
    std::vector<std::string> commandParts;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
            continue;
        }
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
            continue;
        }
        if (arg == "--resp") {
            useResp = true;
            continue;
        }
        commandParts.push_back(arg);
    }

    const int sock = connectToServer(host, port);
    if (sock < 0) {
        std::cerr << "Could not connect to " << host << ":" << port << "\n";
        return 1;
    }

    if (!commandParts.empty()) {
        std::ostringstream command;
        for (size_t i = 0; i < commandParts.size(); ++i) {
            if (i > 0) {
                command << ' ';
            }
            command << commandParts[i];
        }

        if (!sendCommand(sock, command.str(), useResp)) {
            std::cerr << "Failed to send command\n";
            close(sock);
            return 1;
        }

        std::string response;
        if (readResponse(sock, &response)) {
            std::cout << response;
        } else {
            std::cout << "(no response before timeout)\n";
        }

        close(sock);
        return 0;
    }

    std::cout << "Connected to " << host << ":" << port
              << ". Type commands, or QUIT to exit.\n";
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line == "QUIT" || line == "quit" || line == "exit") {
            break;
        }
        if (line.empty()) {
            continue;
        }

        if (!sendCommand(sock, line, useResp)) {
            std::cerr << "Failed to send command\n";
            break;
        }

        std::string response;
        if (readResponse(sock, &response)) {
            std::cout << response;
        } else {
            std::cout << "(no response before timeout)\n";
        }
    }

    close(sock);
    return 0;
}
