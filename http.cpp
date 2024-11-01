#include <string>
#include <vector>
#include <unordered_map>

#include <stdbool.h>
#include <stdint.h>

#include "split_string.hpp"
#include "string_startswith.hpp"

#include "http.hpp"

std::unordered_map<std::string, std::string> LHTTP::mimeTable = {
    {"html", "text/html"},
    {"txt",  "text/plain"},
    {"css",  "text/css"},
    {"js",   "text/javascript"},
    {"png",  "image/png"},
    {"webp", "image/webp"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"ico",  "image/x-icon"}
};

std::vector<std::string> LHTTP::requestTypes = {"GET", "HEAD", "OPTIONS", "TRACE", "PUT", "DELETE", "POST", "PATCH", "CONNECT"};

// takes a file like "/images/screenshot.png" and gets
// a mime type out of it like "image/png"

// note this will break if i add support for more data in the url than just a path
std::string LHTTP::get_MIME_from_filename(std::string filename) {
    std::vector filepathVect = split_string(filename, "/");

    // if there is no file extension, return plaintext
    if (filepathVect[filepathVect.size()-1].find('.') == std::string::npos) 
        return "text/plain";

    std::vector<std::string> filenameVect = split_string(
        filepathVect[filepathVect.size()-1], "."
    );

    std::string fileExt = filenameVect[filenameVect.size()-1];

    auto element = mimeTable.find(fileExt);

    // if mime type not found in map, return plaintext
    if (element == mimeTable.end()) 
        return "text/plain";


    return mimeTable[fileExt];
}


// turns an http request into key-value pairs for each "variable"
// example

// GET /example/path HTTP/1.1
// User-Agent: MyUseragent
// Connection: keep-alive

// into

// {{"ReqMethod",  "GET"},
//  {"ReqPath",    "/example/path"},
//  {"ReqVersion", "HTTP/1.1"}
//  {"User-Agent", "MyUseragent"},
//  {"Connection", "keep-alive"}}

// if the request has an error, return empty map

std::unordered_map<std::string, std::string> LHTTP::get_HTTP_req_params(std::string httpReq) {
    std::unordered_map<std::string, std::string> resMap;

    // http requests using \r\n instead of just \n is so dumb...
    std::vector<std::string> reqLines = split_string(httpReq, "\r\n");

    // check that a valid request method is present at the beginning of the request and store method
    bool foundRequestType = false;
    for (uint8_t i = 0; i < requestTypes.size(); i++) {
        if (startsWith(reqLines[0].c_str(), requestTypes[i].c_str())) {
            foundRequestType = true;
            resMap["ReqMethod"] = requestTypes[i];
            break;
        }
    }
    // return empty map if no request type was found
    if (!foundRequestType) return resMap;

    // split http version from filepath 
    // (why did they have to make space a delimiter between the 3 parts of the first line when it can be incldued in the filepath....)
    // this protocol is more fit for human reading than computer reading....
    resMap["ReqVersion"] = split_string(reqLines[0], " ")[split_string(reqLines[0], " ").size()-1];


    // remove the method and http version from the start and end of string to get filepath like this
    // GET /hi.txt HTTP/1.1
    // XXXX       XXXXXXXXX
    reqLines[0].erase(0,resMap["ReqMethod"].length()+1);

    // i hate this line
    reqLines[0].erase(reqLines[0].size()-(resMap["ReqVersion"].size()+1), resMap["ReqVersion"].size()+1);
    
    resMap["ReqPath"] = reqLines[0];

    // todo parse rest of the lines


    return resMap;
}
