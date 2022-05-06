#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int main()
{
    std::vector<int> v{10, 20, 10, 30, 27, 30, 10};
    std::vector<int> w;
    std::vector<int> x;

    for (auto i1 = v.begin(); i1 != v.end(); ++i1) {
            for (auto i2 = i1 + 1; i2 != v.end(); ++i2) {
                if (*i1 == *i2) w.push_back(*i1);
                else x.push_back(*i1);
             }
    }
    for (auto d : v) cout << d << " ";
    cout << endl;
    for (auto d : w) cout << d << " ";
    cout << endl;
    for (auto d : x) cout << d << " ";
    cout << endl;

    return 0;
}
