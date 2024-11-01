#include <string.h>

#include "string_startswith.hpp"

int startsWith(const char *str, const char *prefix) {
    // Check if the length of the prefix is greater than the string length
    if (strlen(prefix) > strlen(str)) {
        return 0;
    }
    // Compare the prefix with the start of the string
    return strncmp(str, prefix, strlen(prefix)) == 0;
}
