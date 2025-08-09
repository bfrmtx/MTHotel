# JSON.H

json.hpp is from nlohman
https://github.com/nlohmann/json

Even though the class looks "over engineered" it fulfills all requirements.

Use **header["sensor_type"].get<std::string>()** in order to get **MFS-06e** , *otherwise* you get *"MFS-06e"* with brackets included

"sensor": "MFS-06e" with quotes is the default JSON notation and that is what the class is made for
