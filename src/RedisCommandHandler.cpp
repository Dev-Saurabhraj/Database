#include "RedisCommandHandler.h"
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


    if(tokens.empty()) return "Error: Empty command\r\n";
    std::string cmd = tokens[0];


    std::transform(cmd.begin(), cmd.end(), cmd.begin(), :: toupper);

    std::ostringstream response;

    // connect to database

    return response.str();
}
