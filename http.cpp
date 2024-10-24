#include <string>
#include <vector>
#include <unordered_map>

#include "split_string.hpp"

#include "http.hpp"

std::unordered_map<std::string, std::string> mimeTable = {
    {"html", "text/html"},
    {"txt",  "text/plain"},
    {"css",  "text/css"},
    {"js",   "text/javascript"},
    {"png",  "image/png"},
    {"ico",  "image/x-icon"}
};

// takes a file like "/images/screenshot.png" and gets
// a mime type out of it like "image/png"

// note this will break if i add support for more data in the url than just a path
std::string get_MIME_from_filename(std::string filename) {
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
