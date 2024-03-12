#ifndef BASE_XML_H
#define BASE_XML_H
#include <cfloat>
#include <climits>
#include <memory>
#include <sstream>
#include <string>

#include "../xml/tinyxml2/tinyxml2.h"

tinyxml2::XMLElement *open_node(tinyxml2::XMLElement *top_node, const std::string node, const bool can_ignore = false) {
  auto new_node = top_node->FirstChildElement(node.c_str());
  if ((new_node == nullptr) && !can_ignore) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << ":: XML_ERROR_PARSING_ELEMENT -> <" << top_node->Value() << "><" << node << ">";
    throw std::runtime_error(err_str.str());
  }
  return new_node;
}

std::string xml_svalue(tinyxml2::XMLElement *top_node, const std::string node, std::string *attr = nullptr, const std::string attr_name = "") {
  std::string str;
  auto new_node = top_node->FirstChildElement(node.c_str());
  if (new_node != nullptr) {
    str = new_node->GetText();
    if (attr != nullptr && (attr_name.size())) {
      const char *cattr = new_node->Attribute(attr_name.c_str());
      if (cattr != nullptr) {
        *attr = std::string(cattr);
      }
    }
  }
  return str;
}

double xml_dvalue(tinyxml2::XMLElement *top_node, const std::string node, std::string *attr = nullptr, const std::string attr_name = "") {
  double d = DBL_MAX;
  auto new_node = top_node->FirstChildElement(node.c_str());
  if (new_node != nullptr) {
    std::string str(new_node->GetText());
    // d = std::stod(str); can't debug this line
    std::stringstream ss(str);
    ss >> d;
    if (attr != nullptr && (d != DBL_MAX) && (attr_name.size())) {
      const char *cattr = new_node->Attribute(attr_name.c_str());
      if (cattr != nullptr) {
        *attr = std::string(cattr);
      }
    }
  }

  return d;
}

int64_t xml_ivalue(tinyxml2::XMLElement *top_node, const std::string node, std::string *attr = nullptr, const std::string attr_name = "") {
  int64_t i = INT64_MAX;
  auto new_node = top_node->FirstChildElement(node.c_str());
  if (new_node != nullptr) {
    std::string str(new_node->GetText());
    i = std::stol(str);
    if (attr != nullptr && (i != INT64_MAX) && (attr_name.size())) {
      const char *cattr = new_node->Attribute(attr_name.c_str());
      if (cattr != nullptr) {
        *attr = std::string(cattr);
      }
    }
  }
  return i;
}

#endif // BASE_XML_H
