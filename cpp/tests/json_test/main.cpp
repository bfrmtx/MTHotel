
#include <../../include/json.h>
#include <iostream>
#include <iomanip>


int main(int argc, char **argv)
{
    using namespace std;

    using jsn = nlohmann::ordered_json;

    double mp(3.14159265350005E-1);


//    tinyjsonwriter jsn(true);

    std::vector<double> f(4), a(4), p(4);
    for (size_t i = 0; i < f.size(); ++i) f[i] = 1 + i;
    for (size_t i = 0; i < a.size(); ++i) a[i] = 10 + i;
    for (size_t i = 0; i < p.size(); ++i) p[i] = 100 + i;

    a[2] = 1;
    jsn head;

    head["date"] = "2021-02-12";

    head["system"]["system"] = "adu-08e";
    head["system"]["serial"] = 522;
    head["system"]["tag"] = "";


    head["calibration_sensor"]["sensor"] = "MFS-06e";
    head["calibration_sensor"]["serial"] = 129;

    head["calibration_sensor"]["frequency"] = f;
    head["calibration_sensor"]["amplitude"] = a;
    head["calibration_sensor"]["phase"] = p;
    head["calibration_sensor"]["chopper"] = 1;
    head["calibration_sensor"]["tag"] = "free string";

    head["calibration_sensor"]["unit_amplitude"] = "V/(nT * Hz)";
    head["calibration_sensor"]["unit_phase"] = "deg";
    head["calibration_sensor"]["unit_frequency"] = "Hz";



    head["calibration_info"]["atV"] = "0";
    head["calibration_info"]["gain_stage1"] = "0";
    head["calibration_info"]["gain_stage2"] = "0";
    head["calibration_info"]["gain_stage3"] = "0";
    head["calibration_info"]["impedance"] = "(0,0)";
    head["calibration_info"]["equipment"] = "Solartron";
    head["calibration_info"]["datetime"] = "2020-09-14T17:30Z";
    head["calibration_info"]["operator"] = "metronix";

    head["system_header"]["gain_stage1"] = 1.0;
    head["system_header"]["gain_stage2"] = 1.0;
    head["system_header"]["filter_type"] = "off";

    head["system_header"]["dac_val"] = 1.0;
    head["system_header"]["dac_on"] = 1.0;
    head["system_header"]["input_divider"] = 1;


    std::cout << "sens: " << head["calibration_sensor"]["sensor"] << " and ser:" << head["calibration_sensor"]["serial"] << std::endl;
    std::string wh = head["calibration_sensor"]["sensor"];
    std::cout << "mit Gänsefüsschen?  :" << wh << " <" << std::endl;

    //double gx = head["calibration_sensor"]["frequency"];

std::cout << std::setw(8) << head << std::endl;

std::cout << "finish" << endl;

return 0;
}
