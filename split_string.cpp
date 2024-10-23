#include <string>
#include <vector>

#include "split_string.hpp"

// for some reason splitting a string properly isnt a thing without external library......
// https://stackoverflow.com/questions/5607589/right-way-to-split-an-stdstring-into-a-vectorstring

// splits a string by a delimiter into a vector
std::vector<std::string> split_string(std::string str, std::string token){
    std::vector<std::string>result;
    while(str.size()){
        int index = str.find(token);
        if(index!=std::string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}
