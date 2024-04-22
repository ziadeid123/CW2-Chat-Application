#include <iostream> //Input/output stream handling.
#include <fstream> //File stream handling.
#include <sys/socket.h> // Socket programming functions.
#include <netinet/in.h> //  Internet address family functions and structures.
#include <unistd.h> // Standard symbolic constants and types.
#include <arpa/inet.h> // Functions for manipulating numeric IP addresses.
#include <cstring> // C-style string manipulation functions.
#include <openssl/sha.h> // Header file for using the SHA256 cryptographic hash function.

#define PORT 8080 // Define the port number for socket communication
#define MESSAGE_LOG_FILE "message_log.txt" // Define the file name for storing hashed messages
#define CREDENTIALS_FILE "credentials.txt" // Define the file name for storing user credentials

void logHashedMessage(const unsigned char hash[SHA256_DIGEST_LENGTH]) { // Function to log a hashed message to the message log file
    std::ofstream file(MESSAGE_LOG_FILE, std::ios::app | std::ios::binary); // Open the message log file in append mode for binary writing
    if (file.is_open()) {  
        file.write(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH);  // Write the hash to the file
        file << std::endl;  // Add a newline character after the hash
        file.close();  // Close the file
    }
}

void hashMessage(const std::string& message, unsigned char hash[SHA256_DIGEST_LENGTH]) { // Function to hash a given message using SHA-256 algorithm
    SHA256_CTX sha256; // Initialize the SHA-256 context
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, message.c_str(), message.size());  // Update the context with the message data
    SHA256_Final(hash, &sha256); // Finalize the hashing process and store the result in 'hash'
}
// Function to authenticate a user based on provided username and password
bool authenticate(const std::string& username, const std::string& password) {
    std::ifstream file(CREDENTIALS_FILE); // Open the credentials file for reading
    if (file.is_open()) {
        std::string stored_username, stored_password;  // Read the stored username and password from the file
        file >> stored_username >> stored_password;
        file.close();  // Close the file
        return (username == stored_username && password == stored_password); // Check if the provided username and password match the stored credentials
    }
    return false; // Return false if the credentials file couldn't be opened
}

void storeCredentials(const std::string& username, const std::string& password) { // Function to store new user credentials in the credentials file
    std::ofstream file(CREDENTIALS_FILE); // Open the credentials file for writing
    if (file.is_open()) { // Write the username and password to the file
        file << username << std::endl;
        file << password << std::endl;
        file.close(); // Close the file
    }
}

int main() {
    std::string username, password;

    // Check if credentials file exists
    std::ifstream credentialsFile(CREDENTIALS_FILE);
    if (!credentialsFile.is_open()) {
        std::cout << "First-time setup: Enter username: "; // If credentials file doesn't exist, prompt user to create new credentials
        std::cin >> username;
        std::cout << "Enter password: ";
        std::cin >> password;
        storeCredentials(username, password); // Store new credentials in the file
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
        std::cerr << "\n Socket creation error \n"; //socket creation error check
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {  // IP address conversion check
        std::cerr << "\nInvalid address/ Address not supported \n";
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { // Connection attempt check
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

    close(sock); //close socket
    return 0;
}
