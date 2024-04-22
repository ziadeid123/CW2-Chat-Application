#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <openssl/sha.h>

#define PORT 8080
#define MESSAGE_LOG_FILE "message_log.txt"

void logHashedMessage(const unsigned char hash[SHA256_DIGEST_LENGTH]) {
    std::ofstream file(MESSAGE_LOG_FILE, std::ios::app | std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH);
        file << std::endl;
        file.close();
    }
}

void hashMessage(const std::string& message, unsigned char hash[SHA256_DIGEST_LENGTH]) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, message.c_str(), message.size());
    SHA256_Final(hash, &sha256);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is listening for connections..." << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Communication loop
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int valread = read(new_socket, buffer, sizeof(buffer));
            if (valread <= 0)
                break;

            // Hash the received message
            unsigned char hash[SHA256_DIGEST_LENGTH];
            hashMessage(buffer, hash);

            // Log the hashed message
            logHashedMessage(hash);

            // Print the received message in plain text
            std::cout << "Client: " << buffer << std::endl;

            if (strcmp(buffer, "exit") == 0)
                break;

            // Server sending message back to client
            std::cout << "You: ";
            std::cin.getline(buffer, sizeof(buffer));
            send(new_socket, buffer, strlen(buffer), 0);
            if (strcmp(buffer, "exit") == 0)
                break;
        }

        close(new_socket);
    }

    close(server_fd);
    return 0;
}
