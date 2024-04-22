#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <openssl/sha.h>

#define PORT 8080
#define MESSAGE_LOG_FILE "message_log.txt"
#define CREDENTIALS_FILE "credentials.txt"

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

bool authenticate(const std::string& username, const std::string& password) {
    std::ifstream file(CREDENTIALS_FILE);
    if (file.is_open()) {
        std::string stored_username, stored_password;
        file >> stored_username >> stored_password;
        file.close();
        return (username == stored_username && password == stored_password);
    }
    return false;
}

void storeCredentials(const std::string& username, const std::string& password) {
    std::ofstream file(CREDENTIALS_FILE);
    if (file.is_open()) {
        file << username << std::endl;
        file << password << std::endl;
        file.close();
    }
}

int main() {
    std::string username, password;

    // Check if credentials file exists
    std::ifstream credentialsFile(CREDENTIALS_FILE);
    if (!credentialsFile.is_open()) {
        std::cout << "First-time setup: Enter username: ";
        std::cin >> username;
        std::cout << "Enter password: ";
        std::cin >> password;
        storeCredentials(username, password);
    }

    // Prompt for username and password each time
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;

    // Check if credentials are valid
    if (!authenticate(username, password)) {
        std::cerr << "Invalid username or password.\n";
        return 1;
    }

    int sock = 0, valread;
    char buffer[1024] = {0};

    // Create or open the message log file
    std::ofstream messageLog(MESSAGE_LOG_FILE, std::ios::app);
    messageLog.close(); // Close the file immediately

    // Connect to the server
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "\n Socket creation error \n";
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "\nInvalid address/ Address not supported \n";
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "\nConnection Failed \n";
        return -1;
    }

    // Communication loop
    while(true) {
        std::cout << "You: ";
        std::cin.ignore(); // Clear input buffer
        std::cin.getline(buffer, sizeof(buffer));

        // Hash the message
        unsigned char hash[SHA256_DIGEST_LENGTH];
        hashMessage(buffer, hash);

        // Log the hashed message
        logHashedMessage(hash);

        // Send the message to the server
        send(sock, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "exit") == 0)
            break;

        // Receive response from the server
        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, sizeof(buffer));
        if (valread <= 0)
            break;
        std::cout << "Server: " << buffer << std::endl;
        if (strcmp(buffer, "exit") == 0)
            break;
    }

    close(sock);
    return 0;
}
