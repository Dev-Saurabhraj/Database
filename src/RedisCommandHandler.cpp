#include "RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include<vector>
#include<sstream>
#include<algorithm>
#include<string>


//RESP - redis seriallization protocol 
// *2\r\n&4\r\n\PING\r\n&4\r\n\TEST\r\n;
//*2-> there will be any array of length 2 
// &4 -> means that string of length for (ex-> PING)
//[PING, TEST];



std::vector<std::string> parseRespCommand(const std::string &input){
    std::vector<std::string> tokens;
    if(input.empty()) return tokens;


    // if it will not start whith * then we will check wheather it will start with whitespace 
    // then we will fallback to whitespacc handler

    //white space handler;
    if(input[0] !='*'){
        std::istringstream iss(input);
        std::string token;
        while(iss>>token)
            tokens.push_back(token);
        
        return tokens;
    }

    // *2\r\n&4\r\n\PING\r\n&4\r\n\TEST\r\n
    size_t pos = 0;

    if(input[pos]!='*')return tokens;

    pos++; // skip the *

    // crlf = Carriage Return(\r) , Line Feed (\n);
    size_t crlf = input.find("\r\n", pos);

    // it will return the position of \r\n if not found 

    if(crlf== std::string::npos) return tokens;
    

    int numElelments = std::stoi(input.substr(pos, crlf-pos));
    pos = crlf+2;

    for(int i = 0; i<numElelments; i++){
        if(pos>= input.size()|| input[pos]!='$') break;
        pos++; // it will skip the dollar sign $

        crlf = input.find("\r\n", pos);
        if(crlf == std::string::npos) break;
        int len = std::stoi(input.substr(pos, crlf - pos));

        pos = crlf+2;
        if (pos + len > input.size()) break;
        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        pos+=len+2; // skip token and CRLF;
     }

    return tokens;
}

RedisCommandHandler::RedisCommandHandler(){

}
static std::string handlePing(const std::vector<std::string>& tokens , RedisDatabase& db){
    return "+PONG\r\n";
}

static std::string handleEcho(const std::vector<std::string>& tokens , RedisDatabase& db){
    if(tokens.size()<2)
        return "-Error : ECHO requrires a message\r\n";
    else 
        return "+" + tokens[1] + "\r\n"; 
}

static std::string handleFlushAll(const std::vector<std::string>& tokens , RedisDatabase& db){
    db.flushAll();
    return "+OK\r\n";
}

static std::string handleSet(const std::vector<std::string>& tokens , RedisDatabase& db){
     if(tokens.size() < 3){
        return "-Error: SET requires key and value \r\n";
    }
    else{
        db.set(tokens[1], tokens[2]);
        return "+OK\r\n";
    }
}

static std::string handleGet(const std::vector<std::string>& tokens , RedisDatabase& db){
     if(tokens.size() < 2){
            return"-Error: GET requires key \r\n";
        }else{
            std::string value;
            if(db.get(tokens[1], value)){
                return "$" + std::to_string(value.size())+ "\r\n" + value +"\r\n";
            }else{
                return "$-1\r\n";
            }  
        }
}
static std::string handleDel(const std::vector<std::string>& tokens , RedisDatabase& db, const std::string cmd){
    if(tokens.size() < 2){
            return "-Error : "+ cmd+ " requires keys\r\n";
        }else{
            bool res = db.del(tokens[1]);
            return ":"+ std::to_string((res?1:0)) + "\r\n";
        }
}

static std::string handleRename(const std::vector<std::string>& tokens , RedisDatabase& db){
         if(tokens.size()<3){
            return "-Error : RENAME requires old key and new key \r\n";
        }else{
            if(db.rename(tokens[1], tokens[2]))
                return "+OK\r\n";
        }
}
static std::string handleExpire(const std::vector<std::string>& tokens , RedisDatabase& db){
    if(tokens.size()<3){
            return "-Error : EXPIRE requires key and time in seconds\r\n";
        }else{
            int seconds = std::stoi(tokens[2]);
            if(db.expire(tokens[1], seconds))
                return "+OK\r\n";
        }
}
static std::string handleKeys(const std::vector<std::string>& tokens , RedisDatabase& db){
     std::vector<std::string> allKeys = db.keys();
        return "*"+ std::to_string(allKeys.size()) + "\r\n";
        for(const auto& key : allKeys){
            return +"$"+ std::to_string(key.size())+ "\r\n" + key+ "\r\n";
        }
}

static std::string handleType(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size() < 2){
            return "-Error : TYPE requires key\r\n";
        }else{
            return "+" + db.type(tokens[1])+ "\r\n";   
        }
}

static std::string handleLlen(const std::vector<std::string> &tokens, RedisDatabase& db){
    if(tokens.size() < 2)
        return "-Error : Llen requires key\r\n";
    ssize_t len = db.llen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleLpush(const std::vector<std::string> &tokens, RedisDatabase& db){
    if(tokens.size() < 3)
        return "-Error : Lpush requires key and value\r\n";
    db.lpush(tokens[1], tokens[2]);
    ssize_t len = db.llen(tokens[1]);
        return ":" + std::to_string(len) + "\r\n";
}

static std::string handleRpush(const std::vector<std::string> &tokens, RedisDatabase& db){
    if(tokens.size() < 3)
        return "-Error : Rpush requires key and value\r\n";
    db.rpush(tokens[1], tokens[2]);
    ssize_t len = db.llen(tokens[1]);
        return ":" + std::to_string(len) + "\r\n";

}
static std::string handleLpop(const std::vector<std::string> &tokens, RedisDatabase& db){
    if(tokens.size() < 2)
        return "-Error : LPOP requires key\r\n";

        std::string val;
        if(db.lpop(tokens[1], val)){
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        }

return "$-1\r\n";
}
static std::string handleRpop(const std::vector<std::string> &tokens, RedisDatabase& db){
 if(tokens.size() < 2)
        return "-Error : RPOP requires key\r\n";
        std::string val;
        if(db.lpop(tokens[1], val)){
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        }

    return "$-1\r\n";
} 

static std::string handleLrem(const std::vector<std::string> &tokens, RedisDatabase& db){

    if(tokens.size() < 4)
        return "-Error : LREM required key, count and value";

     try{
         int count = std::stoi(tokens[2]);
         int removed = db.lrem(tokens[1], count, tokens[3]);
         return ":" + std::to_string(removed) + "\r\n";

     }catch(std::exception&){
        return "-Error : Invalid count\r\n";
     }

}
static std::string handleLindex(const std::vector<std::string> &tokens, RedisDatabase& db){
if(tokens.size() < 3)
     return "-Error : LINDEX required key and  index ";

     try{
         int index = std::stoi(tokens[1]);
         std::string value;
         if(db.lindex(tokens[1], index, value))
            return ":" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
        else 
            return  "$-1\r\n";
     }catch(std::exception&){
        return "-Error : Invalid index\r\n";
     }
}

static std::string handleLset(const std::vector<std::string> &tokens, RedisDatabase& db){
    if(tokens.size() < 4)
     return "-Error : LSET required key , index and value";
     try{
        int index = std::stoi(tokens[2]);
        if(db.lset(tokens[1], index, tokens[3]))
            return "+OK\r\n";
        else 
            return "-Error : Index out of range\r\n";
        
     }catch(std::exception&){
        return "-Error: Invalid index\r\n";
     }
}
// HASH Commands handler function implemention;

static std::string handleHset(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 4)
        return "-Error : HSET requires key , field and value\r\n";
    db.hset(tokens[1], tokens[2], tokens[3]);
        return ":1\r\n";
}
static std::string handleHget(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 3)
        return "-Error : HGET requires key and field \r\n";
}
static std::string handleHdel(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 3)
        return "-Error : HDEL requires key and field \r\n";
}
static std::string handleHexists(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 3)
        return "-Error : HEXISTS requires key and field \r\n";
}
static std::string handleHgetAll(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 2)
        return "-Error : HGETALL requires key\r\n";
}
static std::string handleHkeys(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 2)
        return "-Error : HKEYS requires key\r\n";
}
static std::string handleHlen(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 2)
        return "-Error : HLEN requires key\r\n";
}
static std::string handleHvals(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 2)
        return "-Error : HVALS requires key\r\n";
}
static std::string handleHmset(const std::vector<std::string> & tokens, RedisDatabase& db){
    if(tokens.size()< 4 || (tokens.size()%2==1))
        return "-Error : HMSET requires key followed by field and value pairs\r\n";
}

std::string RedisCommandHandler::processCommand(const std::string &commandLine){
    // use RESP parser 
    auto tokens = parseRespCommand(commandLine);


    if(tokens.empty()) return "-Error: Empty command\r\n";
    std::string cmd = tokens[0];


    std::transform(cmd.begin(), cmd.end(), cmd.begin(), :: toupper);
    // connect to database
    RedisDatabase& db = RedisDatabase::getInstance();

    //check commands

    //check ping;

    if(cmd=="PING"){
        return handlePing(tokens, db);
    }
    else if(cmd== "ECHO"){
        return handleEcho(tokens, db); 
    }
      //key/value operations commands
    else if(cmd == "FLUSHALL"){
        return handleFlushAll(tokens, db);
    }
    else if (cmd == "SET"){
        return handleSet(tokens, db);
    }
    else if(cmd =="GET"){
        return handleGet(tokens, db);
    }
    else if(cmd=="KEYS"){
        return handleKeys(tokens, db);
    }
    else if(cmd=="DEL"|| cmd=="UNLINK"){
        return handleDel(tokens, db, cmd);
    }
    else if(cmd=="EXPIRE"){
        return handleExpire(tokens, db);

    }else if(cmd=="RENAME"){
        return handleRename(tokens, db);
    }
    // List Operation commands handling
    else if(cmd=="LLEN"){
        return handleLlen(tokens, db);
    }
    else if(cmd=="LPUSH"){
        return handleLpush(tokens, db);
    }
    else if(cmd=="RPUSH"){
        return handleRpush(tokens, db);
    }
    else if(cmd=="LPOP"){
        return handleLpop(tokens, db);
    }
    else if(cmd=="RPOP"){
        return handleRpop(tokens, db);
    }
    else if(cmd=="LREM"){
        return handleLrem(tokens, db);
    }
    else if(cmd=="LINDEX"){
        return handleLindex(tokens, db);
    }
    else if(cmd=="LSET"){
        return handleLset(tokens, db);
    }
    // Hash Operation commands handling
    else if(cmd=="HSET"){
        return handleHset(tokens, db);
    }
    else if(cmd=="HGET"){
        return handleHget(tokens, db);
    }
    else if(cmd=="HEXISTS"){
        return handleHexists(tokens, db);
    }
    else if(cmd=="HDEL"){
        return handleHdel(tokens, db);
    }
    else if(cmd=="HGETALL"){
        return handleHgetAll(tokens, db);
    }
    else if(cmd=="HKEYS"){
        return handleHkeys(tokens, db);
    }
    else if(cmd=="HVALS"){
        return handleHvals(tokens, db);
    }
    else if(cmd=="HLEN"){
        return handleHlen(tokens, db);
    }
    else if(cmd=="HMSET"){
        return handleHmset(tokens, db);
    }
    else{
        return  "-Error: Unknown command\r\n";
    }
  
    
    
}
