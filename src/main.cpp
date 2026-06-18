#include <iostream>
#include "RedisServer.h"
#include <thread>
#include<chrono>
#include"../include/RedisDatabase.h"

int main(int argc , char* argv[]) {
    int port = 8273; // this is the default port
    if(argc>=2) port = std::stoi(argv[1]);
    RedisServer server(port);
    // background persistancce : dump the database evey 300 seconds. 
    std::thread persistanceThread([](){
        while(true){
           std::this_thread::sleep_for(std::chrono::seconds(300));
            if(!RedisDatabase::getInstance().dump("dump.my_rdb"))
                std::cerr<<"Error dumping Database\n";
            else
                std::cout<<"Database dumped to dump.my_rdb\n";
        }
    });

    persistanceThread.detach();
    server.run();

    return 0;
}
