#include <iostream>

#include <memory>
using namespace std;

#include "../../xml/tinyxmlwriter/tinyxmlwriter.h"

int main() {
  try {
    auto xml = make_unique<tinyxmlwriter>(true);

    string a = "channel";
    string b = "id";
    auto n = 1;

    xml->push("writer");
    xml->push(a, b, n);
    xml->element("dip_angles", 91.1, 6, true);
    xml->element("sensor", "MFS-06e");
    xml->element_attr("sensor", "type", "coil", "MFS-06e");
    xml->push_caldata(1, 3, 2, 1.7, complex<double>(1, 3));
    xml->element_attr("c2", "unit", "V/(nT*Hz)", 2.003400e-01, 6, true);
    xml->pop("caldata");
    xml->pop("channel");
    xml->pop("writer");
    cout << xml->xml.str() << endl;
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
  }
  return 0;
}
