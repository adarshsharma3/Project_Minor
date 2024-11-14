#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <cmath>
#include <limits>

#define PORT_MAIN 8080
#define ROWS_PER_CLIENT 120
#define TOTAL_ROWS 602  // Total number of rows in your dataset

// Data structure for storing dataset rows
struct DataRow {
    std::vector<double> features;
    int label;
};

// SVM Model class
class SVMModel {
public:
    std::vector<std::vector<double>> support_vectors;
    std::vector<double> coefficients;
    double intercept;

    // Simplified training function
    void train(const std::vector<DataRow>& data) {
        int numFeatures = data[0].features.size();
        int numSamples = data.size();

        coefficients = std::vector<double>(numFeatures, 0.0);
        intercept = 0.0;

        double C = 1.0;             // Regularization parameter
        double learning_rate = 0.01;
        int max_iterations = 1000;
        double tolerance = 1e-3;

        for (int iter = 0; iter < max_iterations; ++iter) {
            double cost = 0.0;
            std::vector<double> gradient(numFeatures, 0.0);
            double intercept_gradient = 0.0;

            for (int i = 0; i < numSamples; ++i) {
                const auto& row = data[i];
                const auto& features = row.features;
                int label = row.label;

                double decision_value = intercept;
                for (int j = 0; j < numFeatures; ++j) {
                    decision_value += coefficients[j] * features[j];
                }

                double margin = 1 - label * decision_value;
                if (margin > 0) {
                    for (int j = 0; j < numFeatures; ++j) {
                        gradient[j] += -label * features[j];
                    }
                    intercept_gradient += -label;
                    cost += margin;
                }

                for (int j = 0; j < numFeatures; ++j) {
                    gradient[j] += 2 * coefficients[j];
                }
            }

            for (int j = 0; j < numFeatures; ++j) {
                coefficients[j] -= learning_rate * gradient[j] / numSamples;
            }
            intercept -= learning_rate * intercept_gradient / numSamples;

            if (cost < tolerance) break;
        }

        for (int i = 0; i < numSamples; ++i) {
            const auto& row = data[i];
            const auto& features = row.features;
            int label = row.label;

            double decision_value = intercept;
            for (int j = 0; j < numFeatures; ++j) {
                decision_value += coefficients[j] * features[j];
            }

            if (1 - label * decision_value > 0) {
                support_vectors.push_back(features);
            }
        }
    }

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

int main() {
    int sock_main = 0;
    struct sockaddr_in serv_addr_main;

    int client_id;
    std::cout << "Enter your client ID (1 to 5): ";
    std::cin >> client_id;

    int startRow = (client_id - 1) * ROWS_PER_CLIENT;
    int rowCount = ROWS_PER_CLIENT;
    if (startRow + rowCount > TOTAL_ROWS) {
        rowCount = TOTAL_ROWS - startRow;
    }

    auto data = loadData(startRow, rowCount);

    // Train SVM model using loaded data
    SVMModel model;
    model.train(data);

    // Get model parameters as a string
    std::string modelParams = model.getModelParameters();

    // Create socket for Main Server
    if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error for Main Server" << std::endl;
        return -1;
    }

    serv_addr_main.sin_family = AF_INET;
    serv_addr_main.sin_port = htons(PORT_MAIN);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr_main.sin_addr) <= 0) {
        std::cerr << "Invalid address for Main Server" << std::endl;
        return -1;
    }

    // Connect to the Main Server
    if (connect(sock_main, (struct sockaddr*)&serv_addr_main, sizeof(serv_addr_main)) < 0) {
        std::cerr << "Connection Failed to Main Server" << std::endl;
        return -1;
    }

    // Construct and send message to Main Server
    std::string message = "Client ID: " + std::to_string(client_id) + "\n";
    message += "Model Parameters:\n" + modelParams;

    send(sock_main, message.c_str(), message.size(), 0);
    std::cout << "Sent model parameters to Main Server" << std::endl;

    // Close the connection to Main Server
    close(sock_main);

    return 0;
}
