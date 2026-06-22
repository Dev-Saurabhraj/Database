#include "../include/RedisDatabase.h"
#include<mutex>
#include<fstream>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}


bool RedisDatabase::dump(const std::string& filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    for(const auto& kv : kv_store){
        
    }

    if(!ofs) return false;
    return true;
}

bool RedisDatabase::load(const std::string& filename){
    return true;
}
