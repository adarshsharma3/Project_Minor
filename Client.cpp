// Identifier for that client is also being sent along with the message 


#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // Send client identifier
    std::string client_id;
    std::cout << "Enter your client ID: ";
    std::getline(std::cin, client_id);
    send(sock, client_id.c_str(), client_id.length(), 0);

    // Send message to server
    std::string message;
    while (true) {
        std::cout << "Enter message (type 'exit' to quit): ";
        std::getline(std::cin, message);
        if (message == "exit") break;
        send(sock, message.c_str(), message.length(), 0);
    }

    close(sock);
    return 0;
}











































// #include <iostream>
// #include <fstream>
// #include <string>
// #include <cstring>
// #include <arpa/inet.h>
// #include <unistd.h>

// #define PORT 8080

// int main() {
//     int sock = 0;
//     struct sockaddr_in serv_addr;   
//     const char *message =  "City\t\t\tTemperature (°C)\tHumidity (%)\tWind Speed (km/h)\tWeather Description\n"
//     "New York\t\t18\t\t65\t\t15\t\tClear\n";

//     // Create socket
//     if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//         std::cerr << "Socket creation error" << std::endl;
//         return -1;
//     }

//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_port = htons(PORT);

//     // Convert IPv4 and IPv6 addresses from text to binary form
//     if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
//         std::cerr << "Invalid address/ Address not supported" << std::endl;
//         return -1;
//     }

//     // Connect to server
//     if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
//         std::cerr << "Connection Failed" << std::endl;
//         return -1;
//     }

//     // Write message to file
//     std::ofstream outFile("client_output.txt");
//     if (outFile.is_open()) {
//         outFile << message << std::endl; // Write message to file
//         outFile.close();
//     } else {
//         std::cerr << "Unable to open file" << std::endl;
//     }

//     // Send message to server
//     send(sock, message, strlen(message), 0);
//     std::cout << "Message sent to server" << std::endl;

//     close(sock);
//     return 0;
// }
