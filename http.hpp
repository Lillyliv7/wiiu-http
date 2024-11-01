#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace LHTTP {
    extern std::unordered_map<std::string, std::string> mimeTable;
    extern std::vector<std::string> requestTypes;

    // takes a file like "/images/screenshot.png" and gets
    // a mime type out of it like "image/png"

    // note this will break if i add support for more data in the url than just a path
    std::string get_MIME_from_filename(std::string filename);




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

    std::unordered_map<std::string, std::string> get_HTTP_req_params(std::string httpReq);
}
