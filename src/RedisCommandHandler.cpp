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

std::string RedisCommandHandler::processCommand(const std::string &commandLine){
    // use RESP parser 
    auto tokens = parseRespCommand(commandLine);


    if(tokens.empty()) return "-Error: Empty command\r\n";
    std::string cmd = tokens[0];


    std::transform(cmd.begin(), cmd.end(), cmd.begin(), :: toupper);

    std::ostringstream response;

    // connect to database
    RedisDatabase& db = RedisDatabase::getInstance();

    //check commands

    //check ping;

    if(cmd=="PING"){
        response<<"+PONG\r\n";
    }else if(cmd== "ECHO"){
        if(tokens.size()<2)
            response<<"-Error : ECHO requrires a message\r\n";
        else 
            response <<"+"<<tokens[1]<<"\r\n";  
    }
    else if(cmd == "FLUSHALL"){
            db.flushAll();
            response <<"+OK\r\n";
    }else if (cmd == "SET"){
        if(tokens.size() < 3){
            response << "-Error: SET requires key and value \r\n";
        }else {
            db.set(tokens[1], tokens[2]);
            response <<"+OK\r\n";
        }
    }else if(cmd =="GET"){
        if(tokens.size() < 2){
            response <<"-Error: GET requires key \r\n";
        }else{
            std::string value;
            if(db.get(tokens[1], value)){
                response << "$"<< value.size()<<"\r\n"<<value<<"\r\n";
            }else{
                response <<"$-1\r\n";
            }
            
        }
    }else if(cmd=="KEYS"){
        std::vector<std::string> allKeys = db.keys();
        response <<"*"<<allKeys.size() <<"\r\n";
        for(const auto& key : allKeys){
            response <<"$"<<key.size()<<"\r\n"<<key<<"\r\n";
        }
    }else if(cmd=="TYPE"){
        if(tokens.size() < 2){
            response <<"-Error : TYPE requires key\r\n";
        }else{
            response <<"+"<<db.type(tokens[1])<<"\r\n";   
        }

    }else if(cmd=="DEL"|| cmd=="UNLINK"){
        if(tokens.size() < 2){
            response<<"-Error : "<<cmd<<" requires keys\r\n";
        }else{
            bool res = db.del(tokens[1]);
            response <<":"<<(res?1:0)<<"\r\n";
            
        }

    }
    else{
        response << "-Error: Unknow command\r\n";
    }
    //key/value operations commands
    // List Operation commands handling

    // Hash Operation commands handling



    return response.str();
}
