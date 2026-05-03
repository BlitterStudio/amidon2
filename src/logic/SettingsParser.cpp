#include "SettingsParser.h"

#include <cstring>

namespace SettingsParser {

std::string ExtractInstance(const std::string& fileContent) {
    std::string instance;
    const char* keys[] = { "\"server\"", "\"instance\"", NULL };

    for (int k = 0; keys[k] && instance.empty(); k++) {
        const char* pos = strstr(fileContent.c_str(), keys[k]);
        if (pos) {
            pos += strlen(keys[k]);
            while (*pos == ' ' || *pos == ':' || *pos == '\t') pos++;
            if (*pos == '"') {
                pos++;
                const char* end = strchr(pos, '"');
                if (end) {
                    instance.assign(pos, end - pos);
                }
            }
        }
    }

    return instance;
}

}
