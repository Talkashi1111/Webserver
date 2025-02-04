#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include "../inc/HttpRequest.hpp"
#include "../inc/HttpResponse.hpp"
#include "../inc/Logger.hpp"

#define PORT 8080
#define BUFFER_SIZE 8192

class HttpServer {
public:
    HttpServer(int port) : port(port), serverSocket(-1), logger("server.log") {}

    ~HttpServer() {
        if (serverSocket != -1) {
            close(serverSocket);
        }
    }

    void start() {
        setupServerSocket();
        std::stringstream ss;
        ss << "Server running on port " << port << "...";
        logger.log(ss.str());

        while (true) {
            int clientSocket = acceptConnection();
            if (clientSocket != -1) {
                handleClient(clientSocket);
            }
        }
    }

private:
    int port;
    int serverSocket;
    Logger logger;

    void setupServerSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(serverSocket, 10) < 0) {
            perror("Listen failed");
            exit(EXIT_FAILURE);
        }
    }

    int acceptConnection() {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        std::memset(&clientAddr, 0, sizeof(clientAddr));

        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSocket < 0) {
            perror("Accept failed");
        }
        return clientSocket;
    }

    void handleClient(int clientSocket) {
        char buffer[BUFFER_SIZE];
        std::memset(buffer, 0, BUFFER_SIZE);

        ssize_t bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1);
        if (bytesRead < 0) {
            perror("Read failed");
            close(clientSocket);
            return;
        }

        std::string rawRequest(buffer, bytesRead);
        logger.log("Received Request:\n" + rawRequest);

        try
        {
            // Parse the HTTP request
            HttpRequest request;
            request.parse(rawRequest);

            if (request.getTarget() == "/")
                request.setTarget("/index.html");

            std::string target = "data/www" + request.getTarget();

            // Generate a response
            std::ifstream file(target.c_str());
            std::ifstream file404("data/www/error/404.html");
            HttpResponse response;

            if (file.is_open())
            {
                response.setVersion("HTTP/1.1");
                response.setStatus(200);
                response.setReason("OK");
                std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                response.setHeader("Content-Type", "text/html");
                response.setHeader("Content-Length", toString(body.size()));
                response.setBody(body);
            }
            else
            {
                response.setVersion("HTTP/1.1");
                response.setStatus(404);
                response.setReason("Not Found");
                std::string body((std::istreambuf_iterator<char>(file404)), std::istreambuf_iterator<char>());
                response.setHeader("Content-Type", "text/html");
                response.setHeader("Content-Length", toString(body.size()));
                response.setBody(body);
            }

            // Serialize and send the response
            std::string responseString = response.serialize();
            logger.log("Sending Response:\n" + responseString);

            send(clientSocket, responseString.c_str(), responseString.size(), 0);
        } catch (const std::exception& e) {
            logger.log("Error: " + std::string(e.what()));
        }

        close(clientSocket);
    }
};

int main() {
    try {
        HttpServer server(PORT);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
