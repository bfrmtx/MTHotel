#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
using namespace std;

int main()
{
    cout << "Hello World!" << endl;

    std::vector<double> f(3);
    std::vector<double> a(3);
    std::vector<double> p(3);

    f[0] = 1;
    f[1] = 3;
    f[2] = 2;

    a[0] = 1;
    a[1] = 3;
    a[2] = 2;

    p[0] = 10;
    p[1] = 30;
    p[2] = 20;


    std::vector<int> indices(f.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [&f](size_t i1, size_t i2) {
        return f[i1] < f[i2];
    });
    for (const auto d : indices) std::cout << d << " ";
    std::cout << std::endl;

    std::vector<double> out;
    out.reserve(indices.size());
    for (auto j: indices) {
        out.emplace_back(f[j]);
    }
    std::swap(f, out);
    for (const auto d : f) std::cout << d << " ";
    std::cout << std::endl;
    return 0;


}
