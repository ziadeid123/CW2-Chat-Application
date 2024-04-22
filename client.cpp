#include <iostream> //Input/output stream handling.
#include <fstream> //File stream handling.
#include <sys/socket.h>  // Socket programming functions.
#include <netinet/in.h> //  Internet address family functions and structures.
#include <unistd.h> // Standard symbolic constants and types.
#include <arpa/inet.h>  // Functions for manipulating numeric IP addresses.
#include <cstring> // C-style string manipulation functions.
#include <openssl/sha.h> // Header file for using the SHA256 cryptographic hash function.

#define PORT 8080 // Define the port number for socket communication.
#define MESSAGE_LOG_FILE "message_log.txt" // Define the file name for storing hashed messages.
#define CREDENTIALS_FILE "credentials.txt" // Define the file name for storing user credentials.

// Caesar cipher key
const int caesar_key = 3; // Shift by 3 places

// Function to encrypt data using Caesar cipher
std::string encrypt(const std::string& plaintext, int key) {
    std::string ciphertext = plaintext;
    for (char& c : ciphertext) {
        if (isalpha(c)) {
            char base = isupper(c) ? 'A' : 'a';
            c = ((c - base + key) % 26) + base; // Shift letter by key places
        }
    }
    return ciphertext;
}

// Function to decrypt data using Caesar cipher
std::string decrypt(const std::string& ciphertext, int key) {
    return encrypt(ciphertext, 26 - key); // Decrypting is the same as encrypting with the key's complement
}

void logHashedMessage(const unsigned char hash[SHA256_DIGEST_LENGTH]) { // Function to log a hashed message to the message log file
    std::ofstream file(MESSAGE_LOG_FILE, std::ios::app | std::ios::binary);
    if (file.is_open()) {  // Open the message log file in append mode for binary writing
        file.write(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH);// Write the hash to the file
        file << std::endl; // Add a newline character after the hash
        file.close();
    }
}
// Function to hash a given message using SHA-256 algorithm
void hashMessage(const std::string& message, unsigned char hash[SHA256_DIGEST_LENGTH]) {
    SHA256_CTX sha256;  // Initialize the SHA-256 context
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, message.c_str(), message.size()); // Update the context with the message data
    SHA256_Final(hash, &sha256); // Finalize the hashing process and store the result in 'hash'
}
// Function to authenticate a user based on provided username and password
bool authenticate(const std::string& username, const std::string& password) {
    std::ifstream file(CREDENTIALS_FILE); // Open the credentials file for reading
    if (file.is_open()) {
        std::string stored_username, stored_password; // Read the stored username and password from the file
        file >> stored_username >> stored_password; // Check if the provided username and password match the stored credentials
        file.close();

        // Decrypt the stored credentials
        std::string decrypted_username = decrypt(stored_username, caesar_key);
        std::string decrypted_password = decrypt(stored_password, caesar_key);

        return (username == decrypted_username && password == decrypted_password);
    }
    return false;
}
// Function to store new user credentials in the credentials file
void storeCredentials(const std::string& username, const std::string& password) {
    std::ofstream file(CREDENTIALS_FILE); // Open the credentials file for writing
    if (file.is_open()) {
        // Encrypt username and password before storing
        std::string encrypted_username = encrypt(username, caesar_key);
        std::string encrypted_password = encrypt(password, caesar_key);

        file << encrypted_username << std::endl;
        file << encrypted_password << std::endl;
        file.close();
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
        storeCredentials(username, password);  // Store new credentials in the file
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
    messageLog.close();
 // Connect to the server
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "\n Socket creation error \n";
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) { // IP address conversion check
        std::cerr << "\nInvalid address/ Address not supported \n";
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "\nConnection Failed \n";
        return -1;
    }
// Communication loop
    while(true) {
        std::cout << username << ": "; // Display username as prefix
        std::cin.ignore();  // Clear input buffer
        std::cin.getline(buffer, sizeof(buffer));
// Hash the message
        unsigned char hash[SHA256_DIGEST_LENGTH];
        hashMessage(buffer, hash);
        logHashedMessage(hash);  // Log the hashed message
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
