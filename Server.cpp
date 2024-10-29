#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>

#define PORT 8080

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for file access
int clientCount = 0; // Unique client ID counter
int server_fd;       // Server file descriptor for shutdown handling

void signalHandler(int signum) {
    std::cout << "\nServer shutting down..." << std::endl;
    close(server_fd);
    pthread_mutex_destroy(&mutex); // Clean up the mutex
    exit(0);
}

void* handleClient(void* arg) {
    int sock = *((int*)arg);
    delete (int*)arg; // Free the allocated memory for the socket descriptor
    char buffer[1024] = {0};

    // Generate a unique client identifier
    int clientID = __sync_fetch_and_add(&clientCount, 1);

    // Process incoming messages
    while (true) {
        int bytesReceived = read(sock, buffer, sizeof(buffer));
        if (bytesReceived <= 0) {
            std::cout << "Client " << clientID << " disconnected." << std::endl;
            break; // Exit the loop if the client disconnects or an error occurs
        }

        // Null-terminate the received message
        buffer[bytesReceived] = '\0';
        std::cout << "Received from Client " << clientID << ": " << buffer << std::endl;

        // Parse the parameters from the message
        std::istringstream iss(buffer);
        std::string humidity, windSpeed, temperature, overallWeather;

        std::getline(iss, humidity, ',');
        std::getline(iss, windSpeed, ',');
        std::getline(iss, temperature, ',');
        std::getline(iss, overallWeather);

        // Only write complete data rows to the file
        if (!humidity.empty() && !windSpeed.empty() && !temperature.empty() && !overallWeather.empty()) {
            pthread_mutex_lock(&mutex); // Lock the mutex for writing
            std::ofstream outfile("client_output.csv", std::ios::app);
            if (!outfile) {
                std::cerr << "Failed to open output file" << std::endl;
                pthread_mutex_unlock(&mutex);
                continue;
            }

            // Write CSV header if file is empty
            if (outfile.tellp() == 0) { // Only write the header if the file is empty
                outfile << "ClientID,Humidity,Wind Speed,Temperature,Overall Weather\n";
            }

            // Write complete data to file in CSV format
            outfile << clientID << "," << humidity << "," << windSpeed << "," << temperature << "," << overallWeather << "\n";
            outfile.close();
            pthread_mutex_unlock(&mutex); // Unlock the mutex
        }
    }

    close(sock); // Close the socket
    return nullptr;
}


int main() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Set up signal handling for graceful shutdown
    signal(SIGINT, signalHandler);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Set socket options failed" << std::endl;
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }
    
    // Start listening for connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }
    
    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // Accept clients in a loop
    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            return -1;
        }

        // Create a new thread for each client
        pthread_t thread;
        int* new_sock = new int(new_socket);
        pthread_create(&thread, nullptr, handleClient, (void*)new_sock);
        pthread_detach(thread); // Detach the thread so resources are released on exit
    }

    return 0;
}








// #include <iostream>
// #include <fstream>
// #include <cstring>
// #include <unistd.h>
// #include <pthread.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>

// #define PORT 8080

// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for file access
// int clientCount = 0; // Unique client ID counter

// void* handleClient(void* arg) {
//     int sock = *((int*)arg);
//     delete (int*)arg; // Free the allocated memory for the socket descriptor
//     char buffer[1024] = {0};
    
//     // Generate a unique client identifier
//     int clientID = __sync_fetch_and_add(&clientCount, 1);

//     // Open the output file in append mode
//     std::ofstream outfile("client_output.txt", std::ios::app);
//     if (!outfile) {
//         std::cerr << "Failed to open output file" << std::endl;
//         return nullptr;
//     }

//     // Process incoming messages
//     while (true) {
//         int bytesReceived = read(sock, buffer, sizeof(buffer));
//         if (bytesReceived <= 0) {
//             std::cout << "Client " << clientID << " disconnected." << std::endl;
//             break; // Exit the loop if the client disconnects or an error occurs
//         }

//         // Null terminate the received message
//         buffer[bytesReceived] = '\0';
//         std::cout << "Received from Client " << clientID << ": " << buffer << std::endl;

//         // Write the message and client ID to the output file
//         pthread_mutex_lock(&mutex); // Lock the mutex for writing
//         outfile << "Client " << clientID << ": " << buffer << std::endl;
//         pthread_mutex_unlock(&mutex); // Unlock the mutex
//     }

//     // Close the output file
//     outfile.close();
//     close(sock); // Close the socket
//     return nullptr;
// }

// int main() {
//     int server_fd, new_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     // Create socket file descriptor
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         std::cerr << "Socket creation failed" << std::endl;
//         return -1;
//     }

//     // Forcefully attaching socket to the port
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         std::cerr << "Set socket options failed" << std::endl;
//         return -1;
//     }
    
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     // Bind the socket to the address
//     if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
//         std::cerr << "Bind failed" << std::endl;
//         return -1;
//     }
    
//     // Start listening for connections
//     if (listen(server_fd, 3) < 0) {
//         std::cerr << "Listen failed" << std::endl;
//         return -1;
//     }
    
//     std::cout << "Server is listening on port " << PORT << "..." << std::endl;

//     // Accept clients in a loop
//     while (true) {
//         if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
//             std::cerr << "Accept failed" << std::endl;
//             return -1;
//         }

//         // Create a new thread for each client
//         pthread_t thread;
//         int* new_sock = new int(new_socket);
//         pthread_create(&thread, nullptr, handleClient, (void*)new_sock);
//         pthread_detach(thread); // Detach the thread so resources are released on exit
//     }

//     return 0;
// }
