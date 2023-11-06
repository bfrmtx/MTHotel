#ifndef JSON_TEXT_H
#define JSON_TEXT_H

#include "json.h"
#include <string>

using jsn = nlohmann::ordered_json;

std::string to_text(auto &it) {
  if (it.value().type() == jsn::value_t::string) {
    std::string str = it.value().dump();
    // remove quotes
    str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
    // remove backslashes
    str.erase(std::remove(str.begin(), str.end(), '\\'), str.end());
    return str;
  } else {
    return it.value().dump();
  }
}

#endif // JSON_TEXT_H