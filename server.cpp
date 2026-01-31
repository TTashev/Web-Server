#include <algorithm>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <functional>
#include <fstream>
#include <sys/socket.h>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_CLIENTS = 10;

void* handleHttpRequest(void * socketData)
{
    std::vector<char> buffer(BUFFER_SIZE);

    int clientSocketFD;
    clientSocketFD = *(int *) socketData;
    delete (int*)socketData;
    std::cout << "Thread started on connection.\n";
    std::cout << "client id " << clientSocketFD << "\n";

    int message_len = recv(clientSocketFD, &buffer[0], buffer.size(), 0);
    if(message_len > 0)
    {
        const std::string bufferStr(buffer.data());

        std::cout << buffer.data() << "\n";
	std::string messageSuccess = "HTTP/1.1 200 OK\r\n"
				     "Content-Type: text/plain\r\n"
				     "Content-Length: 13\r\n"
				     "\r\n"
				     "Hello World!\n";

	std::string messageError = "HTTP/1.1 404 Not Found\r\n"
                                   "Content-Length: 27\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "\r\n"
	                           "Error, resource not found.\n";

	std::string messageHeader = "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/plain\r\n"
				    "Content-Length: 5\r\n"
                                    "\r\n";
        
	auto it = std::find(buffer.begin(), buffer.end(), '/');

	// send html file
	std::size_t sendFileEndpoint = bufferStr.find("/test");
        if(sendFileEndpoint != std::string::npos)
        {
            std::ifstream file("index.html");
            if (!file) {
                std::cerr << "Error: Could not open index.html\n";
            }

            std::ostringstream contentStream;
            contentStream << file.rdbuf();
            std::string content = contentStream.str();
    
            // Create a basic HTTP response
            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: " + std::to_string(content.size()) + "\r\n"
                                   "Connection: close\r\n"
                                   "\r\n" + content;

    	    send(clientSocketFD, response.c_str(), response.size(), 0);
	}


	// respond to echo endpoint
	std::size_t findEchoEndpoint = bufferStr.find("/echo");
	if(findEchoEndpoint != std::string::npos)
	{
            size_t echoEndPos = bufferStr.find(' ', findEchoEndpoint);
            if(echoEndPos != std::string::npos)
            {
		std::string echoStr = bufferStr.substr(findEchoEndpoint + 6, echoEndPos - (findEchoEndpoint + 5));
		messageHeader.insert(messageHeader.size() - 1, echoStr);
		messageHeader.insert(messageHeader.size() - 1, "\r\n");
		write(clientSocketFD, messageHeader.data(), messageHeader.size());
		close(clientSocketFD);
		pthread_exit(NULL);
            }
	}


	// respond to user-agent endpoint
	std::size_t findEndPoint = bufferStr.find('/');
	std::size_t userAgentEndPoint = bufferStr.find("user-agent", findEndPoint);
	if(userAgentEndPoint != std::string::npos)
	{
	    std::string userAgentStr = "User-Agent: ";
            std::size_t start = bufferStr.find("User-Agent: ");
            start += userAgentStr.length();
            size_t end = bufferStr.find("\r\n", start);
            if (end != std::string::npos)
            {
                std::cout << bufferStr.substr(start, end - start) << "\n";
            }
	}

        if(buffer[std::distance(buffer.begin(), ++it)] == ' ') 
        {
	    int writeRes = write(clientSocketFD, messageSuccess.data(), messageSuccess.size());
	    if(writeRes < 0)
            {
                std::cout << "Write to socket err" << writeRes << "\n";
            }
        }
        else
        {
            write(clientSocketFD, messageError.data(), messageError.size());
        }
    }
    else
    {
	std::cerr << "Error receiving bytes from client socket. \n";
    }

    close(clientSocketFD);

    pthread_exit(NULL);
}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cerr << "Port not found" << std::endl;
	return 1;
    }
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    int port = atoi(argv[1]);
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0

    if (bind(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }

    if (listen(socketFD, MAX_CLIENTS) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return 1;
    }

    char * ip = inet_ntoa(serverAddress.sin_addr);
    std::cout << "Server socket was established successfully.\n";
    std::cout << "Listening on address " << ip << " port " << htons(serverAddress.sin_port) << "\n";

    while(1)
    {
        struct sockaddr_in clientAddress;
        int clientAddressSize = sizeof(struct sockaddr_in);
	int * clientSocketFD = new int;
        *clientSocketFD = accept(socketFD, (struct sockaddr *) &clientAddress, (socklen_t*)&clientAddressSize);
        if(*clientSocketFD > 0)
        {
            std::cout << "Client connection was successful, client socked id: " << clientSocketFD << "\n";

            pthread_t id;
            int create_thread_res = pthread_create(&id, nullptr, &handleHttpRequest, clientSocketFD);
	    if(create_thread_res != 0)
            {
                std::cerr << "Error creating a thread! \n";
            }
	    int detach_res = pthread_detach(id);
            if(detach_res != 0)
	    {
	        std::cerr << "Detach failed! \n";
	    }
        }
    }

    std::cout << "Server shutdown!\n";
    close(socketFD);
    shutdown(socketFD, SHUT_RDWR);

    return 0;
}


