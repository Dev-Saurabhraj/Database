
#include "RedisServer.h"
#include <iostream>
#include "RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
//for linux
#include <sys/socket.h> //(this is the socket library);
#include <netinet/in.h>
//for windows 
// #include <winsock2.h>
// #include <ws2tcpip.h>//*

#include <unistd.h>
#include <vector>
#include <thread>
#include <cstring>
#include<signal.h>

static RedisServer* globalServer = nullptr;


void singalHandler(int signum){
    if(globalServer){
        std::cout<<"\nCaught signal "<<signum<<", shutting down...\n";
        globalServer->shutdown();
    }
    exit(signum);
}



void RedisServer:: setupSignalHandler(){
    signal(SIGINT, singalHandler);
}

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true){
    globalServer = this;
    setupSignalHandler();
}

void RedisServer::shutdown(){
    running = false;

    if(server_socket !=-1){
        close(server_socket);
        std::cout<<"server shoutdown Complete\n";
    }
}

void RedisServer::run(){
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
// AF_inet this mean the ipv4 protocol  AF_INET6 for ipv4;
// SOCK_STEAM this is for TCP 
// for UDP we use SOCK_DGRAM for UDP 

//The combination of the 3 arguments determines the socket protocol:

// Protocol	Arguments
// IPv4+TCP	socket(AF_INET, SOCK_STREAM, 0)
// IPv6+TCP	socket(AF_INET6, SOCK_STREAM, 0)
// IPv4+UDP	socket(AF_INET, SOCK_DGRAM, 0)
// IPv6+UDP	socket(AF_INET6, SOCK_DGRAM, 0)

    if(server_socket<0) {
        std::cerr<<"Error Creating Server Socket\n";
        return;
    }
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0){
        std::cerr<<"Error binding server socket\n";
        return;
    }

    if(listen(server_socket, 10) < 0){
        std::cerr<<"Error Listening on socket\n";
        return;
    }

    std::cout<<"Redis Server is running on Port"<<port<<"\n";

    std::vector<std::thread> threads;
    RedisCommandHandler cmdHandler;

    while(running){
        int client_socket = accept(server_socket, nullptr, nullptr); 
        if(client_socket <0){
            if(running){
                std::cerr<<"Error Accepting client Connectoin\n";
            }
            continue;
        }

        threads.emplace_back([client_socket, &cmdHandler](){
            char buffer[1024];
            while(true){
                memset(buffer, 0, sizeof(buffer));
                int bytes = recv(client_socket, buffer, sizeof(buffer)-1, 0);
                if(bytes <=0) break;
                std::string request(buffer, bytes);
                std::string response = cmdHandler.processCommand(request);
                send(client_socket, response.c_str(), response.size(), 0);
            }
            close(client_socket);

        });
	    
	    
    }

    for(auto & t : threads){
        if(t.joinable()) t.join();
    }
    //shut down;
    if(RedisDatabase::getInstance().dump("dump.my_rdb")){
        std::cout<<"Database Dumped to dump.my_rdb\n";
    }else{
        std::cerr<<"Error dumping database\n";
    }

}
