#pragma once

#include <string>
#include <unordered_map>

extern std::unordered_map<std::string, std::string> mimeTable;

// takes a file like "/images/screenshot.png" and gets
// a mime type out of it like "image/png"
std::string get_MIME_from_filename(std::string filename);
