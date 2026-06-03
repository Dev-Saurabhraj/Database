#include <iostream>
#include "../include/RedisServer.h"
#include <thread>
#include<chrono>


int main(int argc , char* argv[]) {
    int port = 8273; // this is the default port
    if(argc>=2) port = std::stoi(argv[1]);
    RedisServer server(port);
    // background persistancce : dump the database evey 300 seconds. 
    std::thread persistanceThread([](){
        while(true){
           std::this_thread::sleep_for(std::chrono::seconds(300));
        }
    });

    persistanceThread.detach();

    return 0;
}