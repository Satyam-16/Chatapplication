#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")

#define IP_ADDRESS "127.0.0.1"  // Change to "192.168.56.1" if needed
#define DEFAULT_PORT "3504"
#define DEFAULT_BUFLEN 512

struct client_type {
    SOCKET socket;
    int id;
};

const int MAX_CLIENTS = 5;

void process_client(client_type new_client, std::vector<client_type>& clients);

int main() {
    WSADATA wsaData;
    struct addrinfo hints, * server = NULL;
    SOCKET server_socket = INVALID_SOCKET;
    std::vector<client_type> clients(MAX_CLIENTS);
    int temp_id = -1;

    // Initialize Winsock
    std::cout << "Initializing Winsock..." << std::endl;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Setup hints
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Setup Server
    std::cout << "Setting up server..." << std::endl;
    getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &hints, &server);

    // Create a listening socket for connecting to server
    std::cout << "Creating server socket..." << std::endl;
    server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    // Bind the socket
    bind(server_socket, server->ai_addr, (int)server->ai_addrlen);
    freeaddrinfo(server); // Free the addrinfo structure

    // Listen for incoming connections
    std::cout << "Listening..." << std::endl;
    listen(server_socket, SOMAXCONN);

    // Initialize client list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = INVALID_SOCKET;
    }

    while (true) {
        // Accept a new client
        SOCKET incoming = accept(server_socket, NULL, NULL);
        if (incoming == INVALID_SOCKET) continue;

        // Assign a temporary id for the new client
        temp_id = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == INVALID_SOCKET) {
                clients[i].socket = incoming;
                clients[i].id = i;
                temp_id = i;
                break;
            }
        }

        // If there is an available slot
        if (temp_id != -1) {
            std::cout << "Client #" << clients[temp_id].id << " accepted." << std::endl;

            // Create a thread to handle the new client
            std::thread(process_client, clients[temp_id], std::ref(clients)).detach();
        }
        else {
            std::string msg = "Server is full";
            send(incoming, msg.c_str(), msg.length(), 0);
            closesocket(incoming);
        }
    }

    // Close listening socket
    closesocket(server_socket);
    WSACleanup();
    return 0;
}

void process_client(client_type new_client, std::vector<client_type>& clients) {
    char recv_buffer[DEFAULT_BUFLEN];
    int iResult;

    // Session loop
    while (true) {
        memset(recv_buffer, 0, DEFAULT_BUFLEN);
        iResult = recv(new_client.socket, recv_buffer, DEFAULT_BUFLEN, 0);

        if (iResult > 0) {
            std::string msg = "Client #" + std::to_string(new_client.id) + ": " + recv_buffer;
            std::cout << msg;

            // Broadcast to other clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket != INVALID_SOCKET && clients[i].id != new_client.id) {
                    send(clients[i].socket, msg.c_str(), msg.length(), 0);
                }
            }
        }
        else if (iResult == 0) {
            std::cout << "Client #" << new_client.id << " disconnected." << std::endl;
            closesocket(new_client.socket);
            clients[new_client.id].socket = INVALID_SOCKET;
            break;
        }
        else {
            std::cout << "recv() failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }
}
