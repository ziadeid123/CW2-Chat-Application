#include <iostream> // This library is used for input and output operations, such as printing messages to the console (std::cout) and reading user input (std::cin).
#include <fstream> // This library is used for file input and output operations. In the code, it is used to write hashed messages to a log file (MESSAGE_LOG_FILE).
#include <sys/socket.h> // These libraries (The one in the current line and below) are used for socket programming in the Unix and Linux environments. They provide functions and structures for creating and managing network sockets.
#include <netinet/in.h>
#include <unistd.h> // This library provides access to the POSIX operating system API. In the code, it is used for functions like read and write for reading from and writing to sockets.
#include <cstring> // This library provides functions for manipulating C-style strings. In the code, it is used for functions like memset to set memory areas with a particular value.
#include <arpa/inet.h> // This library provides functions for converting between numeric IP addresses and host names. In the code, it is used for functions like inet_pton to convert IP addresses from text to binary form.
#include <openssl/sha.h> // This library is part of the OpenSSL toolkit and provides functions for hashing data using the SHA algorithms. In the code, it is used for hashing messages with SHA-256.

#define PORT 8080 // Define the port number to be used for communication
#define MESSAGE_LOG_FILE "message_log.txt" // Define the name of the message log file

void logHashedMessage(const unsigned char hash[SHA256_DIGEST_LENGTH]) { // Function to log a hashed message to a file
    std::ofstream file(MESSAGE_LOG_FILE, std::ios::app | std::ios::binary); // Open the message log file in append mode and binary mode
    if (file.is_open()) { // Check if the file was successfully opened
        file.write(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH); // Write the hashed message to the file
        file << std::endl;  // Add a newline character to separate entries
        file.close();  // Close the file after writing
    }
}

void hashMessage(const std::string& message, unsigned char hash[SHA256_DIGEST_LENGTH]) { // Function to calculate the SHA256 hash of a message
    SHA256_CTX sha256;  // Initialize the SHA256 context structure
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, message.c_str(), message.size());  // Update the hash calculation with the message content
    // Convert the message string to a C-style string and get its size
    SHA256_Final(hash, &sha256); // Finalize the hash calculation and store the result in the 'hash' buffer
}

int main() {
    int server_fd, new_socket; // File descriptors for the server socket and newly accepted connections
    struct sockaddr_in address; // Structure to hold server address information
    int opt = 1; // Option value for socket operations
    int addrlen = sizeof(address); // Size of the address structure
    char buffer[1024] = {0}; // Buffer to store incoming data

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { // Creating a TCP socket
        perror("socket failed"); // Print error message if socket creation fails
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { // Set socket options to allow reusing the address and port
        perror("setsockopt"); // Print error message if setting socket options fails
        exit(EXIT_FAILURE); // Exit the program with failure status
    }
    address.sin_family = AF_INET; // Set the address family (IPv4), IP address (any interface), and port number
    address.sin_addr.s_addr = INADDR_ANY; // Listen on any available network interface
    address.sin_port = htons(PORT); // Convert port number to network byte order

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { // Bind the socket to the server address
        perror("bind failed"); // Print error message if binding fails
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) { // Listen for incoming connections on the bound socket
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Client is waiting for connection..." << std::endl; // Print a message indicating the client is ready to accept connections

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) { // Accept incoming connections in a loop
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Communication loop to handle messages between server and client
        while (true) {
            memset(buffer, 0, sizeof(buffer));   // Clear the buffer before reading
            int valread = read(new_socket, buffer, sizeof(buffer)); // Read message from the client

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
            std::cin.getline(buffer, sizeof(buffer)); // Read message from server
            send(new_socket, buffer, strlen(buffer), 0); // Send message to client
            if (strcmp(buffer, "exit") == 0)
                break;
        }

        close(new_socket); // Close the socket connected to the client
    }

    close(server_fd); // Close the server socket
    return 0;
}
