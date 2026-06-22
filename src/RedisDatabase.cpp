#include "../include/RedisDatabase.h"
#include<mutex>
#include<fstream>
#include<sstream>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}


bool RedisDatabase::dump(const std::string& filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    for(const auto& kv : kv_store){
        ofs<<"K "<<kv.first<<" "<<kv.second<<"\n";
    }
    for(const auto & kv : list_store){
        ofs<<"L "<<kv.first;
        for(const auto& l : kv.second){
            ofs<<" "<<l;
        }
        ofs<<"\n";
    }
    for(const auto& kv : hash_store){
        ofs<<"H "<< kv.first;
        for(const auto& field_val : kv.second){
            ofs<<" "<<field_val.first<<":"<<field_val.second;
        } 
        ofs<<"\n";
    }

    if(!ofs) return false;
    return true;
}

bool RedisDatabase::load(const std::string& filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if(!ifs) return false;
    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;

    while(std::getline(ifs, line)){

        std::istringstream iss(line);
        char type;
        iss >> type;
        if(type=='k'){
            std::string key,  val;
            iss>>key>>val;
            kv_store[key] = val;
        }else if(type=='L'){
            std::string key;
            std::string item;
            std::vector<std::string> vals;
            iss>>key;
            while(iss>>item){
                vals.push_back(item);
            }
            list_store[key] = vals;
        }else if(type == 'H'){
            std::string key;
            std::string pair;
            std::unordered_map<std::string, std::string> hash;
            while(iss>>pair){
                auto pos = pair.find(':');
                std::string field = pair.substr(0, pos);
                std::string value = pair.substr(pos+1);
                hash[field] = value;
            }
            hash_store[key] = hash;
        }


    }
    
    return true;
}
