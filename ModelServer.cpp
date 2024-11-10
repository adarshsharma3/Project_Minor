#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>  // For handling client connections in separate threads

#define PORT_MODEL 9090
#define ROWS_PER_CLIENT 120
#define TOTAL_ROWS 602  // Total number of rows in your dataset

struct DataRow {
    std::vector<double> features;
    int label;
};

#include <iostream>
#include <vector>
#include <cmath>
#include <limits>

class SVMModel {
public:
    std::vector<std::vector<double>> support_vectors;
    std::vector<double> coefficients;
    double intercept;

    // A function to train the SVM model (simplified)
    void train(const std::vector<DataRow>& data) {
        // Number of features and data points
        int numFeatures = data[0].features.size();
        int numSamples = data.size();

        // Initialize the coefficients (weights) to zero
        coefficients = std::vector<double>(numFeatures, 0.0);
        intercept = 0.0;

        double C = 1.0; // Regularization parameter (penalty for misclassifications)
        double learning_rate = 0.01; // Learning rate for gradient descent
        int max_iterations = 1000; // Maximum number of iterations
        double tolerance = 1e-3; // Tolerance for convergence

        // Gradient Descent optimization
        for (int iter = 0; iter < max_iterations; ++iter) {
            double cost = 0.0;
            std::vector<double> gradient(numFeatures, 0.0); // Gradients for coefficients
            double intercept_gradient = 0.0; // Gradient for intercept

            for (int i = 0; i < numSamples; ++i) {
                // Get the features and label for the current data point
                const auto& row = data[i];
                const auto& features = row.features;
                int label = row.label;

                // Calculate the decision function: w * x + b
                double decision_value = 0.0;
                for (int j = 0; j < numFeatures; ++j) {
                    decision_value += coefficients[j] * features[j];
                }
                decision_value += intercept;

                // Check if the current data point is a support vector
                // The hinge loss function: max(0, 1 - y * (w * x + b))
                double margin = 1 - label * decision_value;
                if (margin > 0) {
                    // If it's a support vector, update the gradients
                    for (int j = 0; j < numFeatures; ++j) {
                        gradient[j] += -label * features[j];
                    }
                    intercept_gradient += -label;
                    cost += margin;
                }

                // Regularization term: L2 norm (w^2)
                for (int j = 0; j < numFeatures; ++j) {
                    gradient[j] += 2 * coefficients[j];  // L2 regularization gradient
                }
            }

            // Update the coefficients and intercept using the gradients
            for (int j = 0; j < numFeatures; ++j) {
                coefficients[j] -= learning_rate * gradient[j] / numSamples;
            }
            intercept -= learning_rate * intercept_gradient / numSamples;

            // Check if cost is small enough (convergence)
            if (cost < tolerance) {
                break;
            }
        }

        // Identify the support vectors (those with a margin greater than 0)
        for (int i = 0; i < numSamples; ++i) {
            const auto& row = data[i];
            const auto& features = row.features;
            int label = row.label;

            // Calculate the decision function: w * x + b
            double decision_value = 0.0;
            for (int j = 0; j < numFeatures; ++j) {
                decision_value += coefficients[j] * features[j];
            }
            decision_value += intercept;

            // If margin > 0, add to support vectors
            if (1 - label * decision_value > 0) {
                support_vectors.push_back(features);
            }
        }
    }

    // A function to return model parameters as a string
    std::string getModelParameters() {
        std::stringstream ss;
        ss << "Support Vectors:\n";
        for (const auto& vec : support_vectors) {
            for (const auto& value : vec) {
                ss << value << " ";
            }
            ss << "\n";
        }
        ss << "Coefficients:\n";
        for (const auto& coeff : coefficients) {
            ss << coeff << " ";
        }
        ss << "\nIntercept: " << intercept << std::endl;
        return ss.str();
    }
};

// Load data from CSV file for a specific range of rows
std::vector<DataRow> loadData(int startRow, int rowCount) {
    std::vector<DataRow> data;
    std::ifstream file("dataset.csv");
    std::string line;
    int currentRow = 0;

    while (std::getline(file, line)) {
        if (currentRow >= startRow && currentRow < startRow + rowCount) {
            std::istringstream ss(line);
            DataRow row;
            double value;
            while (ss >> value) {
                row.features.push_back(value);
                if (ss.peek() == ',') ss.ignore();
            }
            row.label = static_cast<int>(row.features.back());
            row.features.pop_back();
            data.push_back(row);
        }
        currentRow++;
    }
    return data;
}

// Function to handle each client connection
void handleClient(int new_socket) {
    char buffer[1024] = {0};

    // Receive data from client (startRow and rowCount)
    int bytesReceived = recv(new_socket, buffer, sizeof(buffer) - 1, 0);  // Prevent buffer overflow
    if (bytesReceived <= 0) {
        std::cerr << "Failed to receive data from client, bytesReceived: " << bytesReceived << std::endl;
        close(new_socket);  // Close socket if nothing was received or if error occurred
        return;
    }

    buffer[bytesReceived] = '\0';  // Null-terminate the received string for safety
    std::cout << "Received data: " << buffer << std::endl;

    // Parse startRow and rowCount from the received string
    int startRow, rowCount;
    if (sscanf(buffer, "%d,%d", &startRow, &rowCount) != 2) {
        std::cerr << "Failed to parse startRow and rowCount" << std::endl;
        std::string errorMsg = "Error parsing startRow and rowCount";
        send(new_socket, errorMsg.c_str(), errorMsg.size(), 0);
        close(new_socket);  // Close socket
        return;
    }

    std::cout << "startRow: " << startRow << ", rowCount: " << rowCount << std::endl;

    // Load the data for the requested range
    auto data = loadData(startRow, rowCount);

    // Print the loaded data (for testing purposes)
    std::cout << "Loaded " << data.size() << " rows of data:" << std::endl;
    for (const auto& row : data) {
        for (const auto& feature : row.features) {
            std::cout << feature << " ";
        }
        std::cout << "Label: " << row.label << std::endl;
    }

    // Train the SVM model with the loaded data
    SVMModel svm;
    svm.train(data);

    // Get model parameters as a string
    std::string modelParams = svm.getModelParameters();

    // Send the model parameters back to the client
    send(new_socket, modelParams.c_str(), modelParams.size(), 0);

    // Close the client socket after processing
    close(new_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // Set SO_REUSEADDR socket option
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt failed" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_MODEL);

    // Bind socket to the port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "Model Server listening on port " << PORT_MODEL << std::endl;

    // Accept incoming client connections and handle them in separate threads
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        std::thread clientThread(handleClient, new_socket);
        clientThread.detach();  // Detach the thread to handle each client independently
    }

    return 0;
}
