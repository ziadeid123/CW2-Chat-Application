#include <iostream> // This library is used for input and output operations, such as printing messages to the console (std::cout) and reading user input (std::cin).
#include <fstream>// This library is used for file input and output operations. In the code, it is used to write hashed messages to a log file (MESSAGE_LOG_FILE).
#include <sys/socket.h>// These libraries (The one in the current line and below) are used for socket programming in the Unix and Linux environments. They provide functions and structures for creating and managing network sockets
#include <netinet/in.h>
#include <unistd.h>// This library provides access to the POSIX operating system API. In the code, it is used for functions like read and write for reading from and writing to sockets.
#include <cstring> // This library provides functions for manipulating C-style strings. In the code, it is used for functions like memset to set memory areas with a particular value.
#include <arpa/inet.h> // This library provides functions for converting between numeric IP addresses and host names. In the code, it is used for functions like inet_pton to convert IP addresses from text to binary form
#include <openssl/sha.h> // This library is part of the OpenSSL toolkit and provides functions for hashing data using the SHA algorithms. In the code, it is used for hashing messages with SHA-256

#define PORT 8080 // Define the port number to be used for communication
#define MESSAGE_LOG_FILE "message_log.txt" // Define the name of the message log file
#define CREDENTIALS_FILE "client2.txt" //Define the name of the text file for storing passwords for client 2.

const int caesar_key = 3; // Define the Caesar cipher key
// Function to encrypt plaintext using Caesar cipher
std::string encrypt(const std::string& plaintext, int key) { 
    std::string ciphertext = plaintext; // Initialize ciphertext with the plaintext
    for (char& c : ciphertext) { // Loop through each character in the plaintext
        if (isalpha(c)) { // Check if the character is alphabetic
            char base = isupper(c) ? 'A' : 'a'; // Determine the base character based on case (uppercase or lowercase)
            c = ((c - base + key) % 26) + base; // Apply the Caesar cipher shift and ensure it stays within the alphabetic range
        }
    }
    return ciphertext;
}
// Function to decrypt ciphertext using Caesar cipher
std::string decrypt(const std::string& ciphertext, int key) {// Decrypt ciphertext by shifting in the opposite direction
    return encrypt(ciphertext, 26 - key);
}
// Function to log hashed message to a file
void logHashedMessage(const unsigned char hash[SHA256_DIGEST_LENGTH]) {// Open the message log file in append mode and binary mode
    std::ofstream file(MESSAGE_LOG_FILE, std::ios::app | std::ios::binary);
    if (file.is_open()) {// Check if the file is successfully opened
        file.write(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH);
        file << std::endl;// Add a newline character after writing the hash
        file.close();
    }
}
// Function to hash a message using SHA-256
void hashMessage(const std::string& message, unsigned char hash[SHA256_DIGEST_LENGTH]) { // Declare the SHA-256 context
    SHA256_CTX sha256;
    SHA256_Init(&sha256); // Initialize the SHA-256 context
    SHA256_Update(&sha256, message.c_str(), message.size());  // Update the hash calculation with the message data
    SHA256_Final(hash, &sha256); // Finalize the hash calculation and store the result in 'hash'
}
// Function to authenticate a user based on provided username and password
bool authenticate(const std::string& username, const std::string& password) {
    std::ifstream file(CREDENTIALS_FILE);
    if (file.is_open()) {
        std::string stored_username, stored_password; // Declare variables to store the stored username and password
        file >> stored_username >> stored_password; // Read the stored username and password from the file
        file.close();

        std::string decrypted_username = decrypt(stored_username, caesar_key);// Decrypt the stored username
        std::string decrypted_password = decrypt(stored_password, caesar_key);// Decrypt the stored password
// Check if the provided username and password match the stored credentials
        return (username == decrypted_username && password == decrypted_password);
    }
    return false; // Return false if the file couldn't be opened or if authentication fails
}
// Function to set up initial credentials if the credentials file doesn't exist
void setupCredentials() {
    std::string username, password; // Declare variables to store the username and password
    std::cout << "First-time setup: Enter username: "; // Prompt the user to enter the username
    std::cin >> username;// Read the username from the user
    std::cout << "Enter password: "; // Prompt the user to enter the password
    std::cin >> password;  // Read the password from the user
// Encrypt the username and password before storing
    std::string encrypted_username = encrypt(username, caesar_key);
    std::string encrypted_password = encrypt(password, caesar_key);
// Open the credentials file for writing
    std::ofstream file(CREDENTIALS_FILE);
    if (file.is_open()) {
        file << encrypted_username << std::endl;// Write the encrypted username to the file
        file << encrypted_password << std::endl;// Write the encrypted password to the file
        file.close();
    }
}
// Function to get the username from the credentials file
std::string getUsername() {
    std::ifstream file(CREDENTIALS_FILE);
    std::string username; // Declare a variable to store the username
    if (file.is_open()) {
        std::getline(file, username); // Read the username from the file
        file.close();
    }
    return decrypt(username, caesar_key); // Decrypt the username and return it
}

int main() {
    std::ifstream credentialsFile(CREDENTIALS_FILE); // Open the credentials file to check if it exists
    if (!credentialsFile.is_open()) {
        setupCredentials(); // Set up initial credentials if file does not exist
    }

    std::string username, password; // Declare variables to store the username and password
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;
// Check if the provided username and password are valid
    if (!authenticate(username, password)) {
        std::cerr << "Invalid username or password.\n"; // Print error message for invalid credentials
        return 1;  //Exit the program with error status
    }
// File descriptors for the server socket and newly accepted connections
    int server_fd, new_socket; 
    struct sockaddr_in address;// Structure to hold server address information
    int opt = 1; // Option value for socket operations
    int addrlen = sizeof(address); // Size of the address structure
    char buffer[1024] = {0}; // Buffer to store incoming data
// Create a socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); // Print error message if socket creation fails
        exit(EXIT_FAILURE);
    }
// Set socket options to allow reusing the address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET; // Set the address family (IPv4)
    address.sin_addr.s_addr = INADDR_ANY; // Listen on any available network interface
    address.sin_port = htons(PORT); // Convert port number to network byte order
// Bind the socket to the server address. If binding fails, print an error message and exit.
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) { // Start listening for incoming connections on the bound socket. If listening fails, print an error message and exit.
        perror("listen");
        exit(EXIT_FAILURE);
    }
// Get the username of the client from the credentials file
    std::string clientUsername = getUsername();
    std::cout << "Server is waiting for connection with user: " << clientUsername << std::endl;
// Main server loop to accept and handle incoming connections
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
// Communication loop to handle messages between server and client
        while (true) {
            memset(buffer, 0, sizeof(buffer)); // Clear the buffer before reading
            int valread = read(new_socket, buffer, sizeof(buffer)); // Read message from the client

            if (valread <= 0)
                break;
// Hash the received message
            unsigned char hash[SHA256_DIGEST_LENGTH];
            hashMessage(buffer, hash);
            logHashedMessage(hash);
// Prompt the server to enter a message with the client's username as prefix
            std::cout << "Client 1: " << buffer << std::endl;

            if (strcmp(buffer, "exit") == 0)
                break;

            std::cout << clientUsername << ": ";
            std::cin.ignore();
            std::cin.getline(buffer, sizeof(buffer));
            send(new_socket, buffer, strlen(buffer), 0); // Send the message to the client
            if (strcmp(buffer, "exit") == 0)
                break;
        }

        close(new_socket); // Close the socket connected to the client
    }

    close(server_fd); // Close the server socket
    return 0;  // Return 0 to indicate successful execution
}
