#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <unordered_map>

#define PORT 8080
#define BUFFER_SIZE 1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<int, std::string> clientParameters;

// Function to write parameters to output.csv
void writeToCSV(const std::string& data) {
    std::ofstream outputFile;
    // Open file in append mode
    outputFile.open("output.csv", std::ios::app);
    if (outputFile.is_open()) {
        outputFile << data << std::endl;  // Append the data as a new row
        outputFile.close();
    } else {
        std::cerr << "Failed to open output.csv for writing." << std::endl;
    }
}

// Function to handle client connection and store the parameters
void* handleClient(void* arg) {
    int sock = *((int*)arg);
    delete (int*)arg;

    char buffer[BUFFER_SIZE] = {0};

    // Receive client ID (assuming it was sent first)
    int clientID;
    int bytesReceived = read(sock, &clientID, sizeof(clientID));
    if (bytesReceived <= 0) {
        std::cerr << "Error or connection closed while receiving client ID." << std::endl;
        close(sock);
        return nullptr;
    }
    std::cout << "Received client ID: " << clientID << std::endl;

    // Receive the startRow and rowCount as a string (it was sent from the client)
    bytesReceived = read(sock, buffer, BUFFER_SIZE);
    if (bytesReceived <= 0) {
        std::cerr << "Error or connection closed while receiving startRow and rowCount." << std::endl;
        close(sock);
        return nullptr;
    }
    buffer[bytesReceived] = '\0';  // Null-terminate the received string
    std::cout << "Received startRow and rowCount from Client " << clientID << ": " << buffer << std::endl;

    // Receive model parameters (these will be sent after startRow and rowCount)
    char modelParams[BUFFER_SIZE] = {0};
    bytesReceived = read(sock, modelParams, BUFFER_SIZE);
    if (bytesReceived <= 0) {
        std::cerr << "Error or connection closed while receiving model parameters." << std::endl;
        close(sock);
        return nullptr;
    }
    modelParams[bytesReceived] = '\0';  // Null-terminate the received string

    // Debugging output: print the received model parameters
    std::cout << "Received model parameters from Client " << clientID << ": " << modelParams << std::endl;

    // Save parameters in a map
    pthread_mutex_lock(&mutex);
    clientParameters[clientID] = modelParams;
    // Write the received model parameters to output.csv
    writeToCSV(modelParams);
    pthread_mutex_unlock(&mutex);

    close(sock);
    return nullptr;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Set socket options failed" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed" << std::endl;
            return -1;
        }

        pthread_t thread;
        int* new_sock = new int(new_socket);
        pthread_create(&thread, nullptr, handleClient, (void*)new_sock);
        pthread_detach(thread);
    }

    return 0;
}
