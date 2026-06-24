#include "../include/RedisDatabase.h"
#include<mutex>
#include<fstream>
#include<sstream>
#include<algorithm>

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


    bool RedisDatabase::flushAll(){
        std::lock_guard<std::mutex> lock(db_mutex);
        kv_store.clear();
        list_store.clear();
        hash_store.clear();
        return true;
    }

    void RedisDatabase::set(const std::string key, const std::string value){
        std::lock_guard<std::mutex> lock(db_mutex);
        kv_store[key] = value;
    }

    bool RedisDatabase::get(const std::string key, std::string& value){
        std::lock_guard<std::mutex> lock(db_mutex);
        auto it = kv_store.find(key);
        if(it!=kv_store.end()){
            value = it->second;
            return true;
        }

        return false;

    }
    std::vector<std::string> RedisDatabase::keys(){
        std::lock_guard<std::mutex> lock(db_mutex);
        std::vector<std::string> result;
        for(const auto& pair : kv_store){
            result.push_back(pair.first);
        }
        for(const auto& pair : list_store){
            result.push_back(pair.first);
        }
        for(const auto& pair : hash_store){
            result.push_back(pair.first);
        }
        return result;
    }

    std::string RedisDatabase::type(const std::string& key){
        std::lock_guard<std::mutex> lock(db_mutex);
        if(kv_store.find(key)!= kv_store.end()){
            return "string";
        } 
        else if(list_store.find(key)!= list_store.end()){
            return "list";
        } 
        else if(hash_store.find(key)!= hash_store.end()){
            return "hash";
        }else{
            return "none";
        }

    }
    bool RedisDatabase::del(const std::string &key){
        std::lock_guard<std::mutex> lock(db_mutex);
        bool deleted = false;
        deleted |= kv_store.erase(key)>0;
        deleted |= list_store.erase(key)>0;
        deleted |= hash_store.erase(key)>0;
        return deleted;
    }

    bool RedisDatabase::rename(const std::string& oldKey, const std::string &newKey){
        std::lock_guard<std::mutex> lock(db_mutex);
        auto it = kv_store.find(oldKey);
        if(it!=kv_store.end()){
            std::string value = it->second;
            kv_store.erase(it);
            kv_store[newKey] = value;
            return true;
        }
        auto itl = list_store.find(oldKey);
        if(itl!=list_store.end()){
            list_store[newKey] = itl->second;
            list_store.erase(itl);
            return true;
        }
        auto ith = hash_store.find(oldKey);
        if(ith!=hash_store.end()){
            hash_store[newKey] = ith->second;
            hash_store.erase(ith);
            return true;
        }
        auto itExpire = expiry_map.find(oldKey);
        if(itExpire!=expiry_map.end()){
            expiry_map[newKey] = itExpire->second;
            expiry_map.erase(itExpire);
            return true;
        }
       
        return false;
    }

    bool RedisDatabase::expire(const std::string& key, int  seconds){
        std::lock_guard<std::mutex> lock(db_mutex);
        bool exist = (kv_store.find(key)!= kv_store.end())||
                     (list_store.find(key)!=list_store.end())||
                     (hash_store.find(key)!=hash_store.end());
        if(!exist) return false;
        expiry_map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        return true;
    }


    ssize_t RedisDatabase::llen(const std::string & key){
        std::lock_guard<std::mutex> lock(db_mutex);
        auto it = list_store.find(key);
        if(it!= list_store.end()){
            return it->second.size();
        }
        return 0;
     }

    void RedisDatabase::lpush(const std::string& key, const std::string& value){
        std::lock_guard<std::mutex> lock(db_mutex);
        list_store[key].insert(list_store[key].begin(), value);
    }

    void RedisDatabase::rpush(const std::string& key, const std::string& value){
        std::lock_guard<std::mutex> lock(db_mutex);
        list_store[key].push_back(value);
    }

    bool RedisDatabase::lpop(const std::string& key, std::string& value){
        std::lock_guard<std::mutex> lock(db_mutex);
        auto it = list_store.find(key);
        if(it!= list_store.end() && !it->second.empty()){
            value = it->second.front();
            it->second.erase(it->second.begin());
            return true;
        }
        return false;
    }

    bool RedisDatabase::rpop(const std::string& key, std::string& value){
         std::lock_guard<std::mutex> lock(db_mutex);
        auto it = list_store.find(key);
        if(it!= list_store.end() && !it->second.empty()){
            value = it->second.back();
            it->second.pop_back();
                return true;
        }
        return false;
    }


    int RedisDatabase::lrem(const std::string& key, int count, const std::string& value){
        std::lock_guard<std::mutex> lock(db_mutex);
        int removed = 0;
        auto it = list_store.find(key);
        if(it == list_store.end())
            return 0;
        auto & lst = it->second;

        if(count ==0){
            // remove all occurence 
            auto new_end = std::remove(lst.begin(), lst.end(), value);
            removed = std::distance(new_end, lst.end());
            lst.erase(new_end, lst.end());
        }else if(count > 0 ){
            // remove from head to tail;
            for(auto iter  = lst.begin(); iter!= lst.end() && removed < count; ){
                if(*iter == value){
                    iter = lst.erase(iter);
                    ++removed;
                }else{
                    ++iter;
                }
            }
        }else {
            for(auto revit = lst.rbegin();  revit!= lst.rend() && removed < (-count);){
                if(*revit ==value){
                    auto fwdIterator = revit.base();
                    --fwdIterator;
                    fwdIterator = lst.erase(fwdIterator);
                    ++removed;
                    revit = std::reverse_iterator<std::vector<std::string>::iterator>(fwdIterator);
                }else{
                    ++revit;
                }
            }
        }

        return removed;
    }


    bool RedisDatabase::lindex(const std::string& key, int index, std::string& value){
        std::lock_guard<std::mutex> lock(db_mutex);
        auto it = list_store.find(key);
        if(it==list_store.end()) return false;
        const auto& lst = it->second;
        if(index < 0)
            index = lst.size() + index;
        
        if(index < 0 || static_cast<ssize_t>(index) >= lst.size())
            return false;
        value = lst[index];
            return true;

    }
    bool RedisDatabase::lset(const std::string& key, int index, const std::string& value){
        std::lock_guard<std::mutex> lock(db_mutex);
        auto it = list_store.find(key);
        if(it==list_store.end()) return false;


        auto& lst = it->second;
        
        if(index < 0)
            index = lst.size() + index;
        
        if(index < 0 || static_cast<ssize_t>(index) >= lst.size())
            return false;

        lst[index] = value;
            return true;
    }