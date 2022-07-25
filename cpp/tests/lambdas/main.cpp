#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>


struct calibration {
    std::string sensor;
    std::uint64_t serial = 0;
    std::uint64_t chopper = 0;
    std::uint64_t other = 0;

    calibration(std::string sensor, uint64_t serial, uint64_t chopper) : sensor(sensor), serial(serial), chopper(chopper) {;}

};

bool operator == (const std::shared_ptr<calibration>& lhs, const std::shared_ptr<calibration>& rhs) {
    if (lhs->chopper != rhs->chopper) return false;
    if (lhs->serial != rhs->serial) return false;
    if (lhs->sensor != rhs->sensor) return false;
    if (lhs->other != rhs->other) return false;

    return true;
}

// lamda

auto compare_serial = [](const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) -> bool {
    return lhs->serial < rhs->serial;
};

auto find_serial = [](const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) -> bool {
    return ((lhs->serial == rhs->serial) && (lhs->chopper == rhs->chopper));
};

struct s_find_serial {
    std::uint64_t serial = 0;
    std::uint64_t chopper = 0;
    s_find_serial(const std::shared_ptr<calibration> &lhs) : serial(lhs->serial), chopper(lhs->chopper){;}
    bool operator()(std::shared_ptr<calibration> &rhs) const {
        return ((this->serial == rhs->serial) && (this->chopper == rhs->chopper));
    }
};

int main()
{
    std::string sensor("MFS-06e");
    size_t i, n = 5;
    std::vector<std::shared_ptr<calibration>> cals;
    auto find_me = std::make_shared<calibration>("MFS-06e", 3, 1);

    for (i = n; i > 0; --i) {
        cals.emplace_back(std::make_shared<calibration>("MFS-06e", i, 0));
        cals.emplace_back(std::make_shared<calibration>("MFS-06e", i, 1));

    }

    for (const auto &cal : cals) {
        std::cout << cal->serial << " ";
    }
    std::cout << std::endl;

    std::sort(cals.begin(), cals.end(), compare_serial);

    for (const auto &cal : cals) {
        std::cout << cal->serial << " ";
    }
    std::cout << std::endl;


    i = 0;
    for (const auto &cal : cals) {
        if (find_serial(find_me, cal)) std::cout << cal->serial << " found at " << i;
        ++i;
    }
    std::cout << std::endl;

    // can a lambda be initialized? ... no?
    //std::find_if(cals.begin(), cals.end(), [&find_me]find_serial);


    auto found = std::find_if(cals.begin(), cals.end(), s_find_serial(find_me));
    if (found != cals.end()) {
        std::cout << "first (" << (*found)->serial << ") found at "  << found - cals.begin();
    }
    std::cout << std::endl;


    size_t chunk_size = 5;
    std::vector<std::int32_t> ints(chunk_size);
    std::vector<double> dbls;
    double lsb = 2.2;
    for (int32_t i = 0; i < ints.size(); ++i) {
        ints[i] = i + 1;
    }


    dbls.resize(ints.size());
    std::transform(ints.begin(), ints.end(), dbls.begin(),
                   [lsb](double v) {
                       return (lsb * v);
                   }
                   );



    for (auto v: dbls) std::cout << v << std::endl;
    std::cout << std::endl;
    return 0;
}
