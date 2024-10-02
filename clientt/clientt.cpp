#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define IP_ADDRESS "127.0.0.1" // Change to "192.168.56.1" if needed
#define DEFAULT_PORT "3504"

struct client_type {
    SOCKET socket;
    int id;
    char received_message[DEFAULT_BUFLEN];
};

void process_client(client_type& client);

int main() {
    WSADATA wsa_data;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    client_type client = { INVALID_SOCKET, -1, "" };
    int iResult = 0;

    std::cout << "Starting Client...\n";

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (iResult != 0) {
        std::cout << "WSAStartup() failed with error: " << iResult << std::endl;
        return 1;
    }

    // Setup hints
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::cout << "Connecting...\n";

    // Resolve the server address and port
    iResult = getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo() failed with error: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        client.socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (client.socket == INVALID_SOCKET) {
            std::cout << "socket() failed with error: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(client.socket);
            client.socket = INVALID_SOCKET;
            continue;
        }
        break; // Successful connection
    }

    freeaddrinfo(result);

    if (client.socket == INVALID_SOCKET) {
        std::cout << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Successfully Connected\n";

    // Start receiving messages in a separate thread
    std::thread my_thread(process_client, std::ref(client));

    // Main loop to send messages
    std::string sent_message;
    while (true) {
        std::cout << "Enter message: ";
        getline(std::cin, sent_message);

        // Send the message to the server
        iResult = send(client.socket, sent_message.c_str(), sent_message.length(), 0);
        if (iResult <= 0) {
            std::cout << "send() failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Shutdown the connection since no more data will be sent
    closesocket(client.socket);
    WSACleanup();
    return 0;
}

void process_client(client_type& client) {
    int iResult;
    while (true) {
        memset(client.received_message, 0, DEFAULT_BUFLEN);
        iResult = recv(client.socket, client.received_message, DEFAULT_BUFLEN, 0);

        if (iResult > 0) {
            std::cout << "Received: " << client.received_message << std::endl;
        }
        else if (iResult == 0) {
            std::cout << "Connection closed by the server." << std::endl;
            break;
        }
        else {
            std::cout << "recv() failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }
}
