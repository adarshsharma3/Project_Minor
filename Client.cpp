#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_MODEL 9090
#define PORT_MAIN 8080
#define ROWS_PER_CLIENT 120
#define TOTAL_ROWS 602  // Total number of rows in your dataset

// Load data from CSV file for a specific range of rows
std::vector<std::vector<double>> loadData(int startRow, int rowCount) {
    std::vector<std::vector<double>> data;
    std::ifstream file("dataset.csv");
    std::string line;
    int currentRow = 0;

    while (std::getline(file, line)) {
        if (currentRow >= startRow && currentRow < startRow + rowCount) {
            std::istringstream ss(line);
            std::vector<double> row;
            double value;
            while (ss >> value) {
                row.push_back(value);
                if (ss.peek() == ',') ss.ignore();
            }
            data.push_back(row);
        }
        currentRow++;
    }
    return data;
}

int main() {
    int sock_model = 0, sock_main = 0;
    struct sockaddr_in serv_addr_model, serv_addr_main;

    // Create socket for Model Server
    if ((sock_model = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error for Model Server" << std::endl;
        return -1;
    }

    serv_addr_model.sin_family = AF_INET;
    serv_addr_model.sin_port = htons(PORT_MODEL);

    // Convert IPv4 and IPv6 addresses from text to binary form for Model Server
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr_model.sin_addr) <= 0) {
        std::cerr << "Invalid address for Model Server" << std::endl;
        return -1;
    }

    // Connect to the Model Server directly
    if (connect(sock_model, (struct sockaddr*)&serv_addr_model, sizeof(serv_addr_model)) < 0) {
        std::cerr << "Connection Failed to Model Server" << std::endl;
        return -1;
    }

    // Ask for the client ID
    int client_id;
    std::cout << "Enter your client ID (1 to 5): ";
    std::cin >> client_id;

    // Calculate the range of rows for this client
    int startRow = (client_id - 1) * ROWS_PER_CLIENT;
    int rowCount = ROWS_PER_CLIENT;

    // Adjust rowCount for last client if there are less than 120 rows remaining
    if (startRow + rowCount > TOTAL_ROWS) {
        rowCount = TOTAL_ROWS - startRow;
    }

    std::cout << "Client ID: " << client_id << ", Sending startRow: " << startRow << ", rowCount: " << rowCount << std::endl;

    // Here we load the data (even though we won't send it, you can keep this for debugging or later use)
    auto data = loadData(startRow, rowCount);

    // Serialize the startRow and rowCount to a string format to send to Model Server
    std::string dataStr = std::to_string(startRow) + "," + std::to_string(rowCount) + "\n";

    // Send startRow and rowCount to Model Server
    send(sock_model, dataStr.c_str(), dataStr.size(), 0);
    std::cout << "Sent startRow and rowCount to Model Server" << std::endl;

    // Receive acknowledgment from Model Server
    char ack_buffer[1024] = {0};
    int bytesReceived = recv(sock_model, ack_buffer, sizeof(ack_buffer), 0);
    if (bytesReceived > 0) {
        ack_buffer[bytesReceived] = '\0';  // Null-terminate the received acknowledgment
        std::cout << "Received acknowledgment from Model Server: " << ack_buffer << std::endl;
    } else {
        std::cerr << "Failed to receive acknowledgment from Model Server" << std::endl;
        close(sock_model);
        return -1;
    }

    // Now, receive model parameters from Model Server
    char model_params[1024] = {0};
    read(sock_model, model_params, sizeof(model_params));
    std::cout << "Received model parameters from Model Server: " << model_params << std::endl;

    // Close the connection to Model Server
    close(sock_model);

    // Now, send the acknowledgment message and model parameters to the Main Server
    if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error for Main Server" << std::endl;
        return -1;
    }

    serv_addr_main.sin_family = AF_INET;
    serv_addr_main.sin_port = htons(PORT_MAIN);

    // Convert IPv4 and IPv6 addresses from text to binary form for Main Server
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr_main.sin_addr) <= 0) {
        std::cerr << "Invalid address for Main Server" << std::endl;
        return -1;
    }

    // Connect to the Main Server
    if (connect(sock_main, (struct sockaddr*)&serv_addr_main, sizeof(serv_addr_main)) < 0) {
        std::cerr << "Connection Failed to Main Server" << std::endl;
        return -1;
    }

    // Construct the message to send to Main Server
    std::string message = "Client ID: " + std::to_string(client_id) + "\n";
    message += "Acknowledgment: " + std::string(ack_buffer) + "\n";
    message += "Model Parameters: " + std::string(model_params) + "\n";

    // Send the acknowledgment and model parameters to Main Server
    send(sock_main, message.c_str(), message.size(), 0);
    std::cout << "Sent acknowledgment and model parameters to Main Server" << std::endl;

    // Close the connection to Main Server
    close(sock_main);

    return 0;
}
