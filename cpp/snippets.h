#include <cstdio>
#include <iostream>

try {
    std::cout << "hello";
    }

catch (const std::string &error ) {
        std::cerr << error <<std::endl;
        return EXIT_FAILURE;
    }
catch (std::filesystem::filesystem_error& e) {
       std::cerr <<  e.what() << std::endl;
    }
catch (...) {
    std::cerr << "could not execute all threads" << std::endl;
    return EXIT_FAILURE;
}



fs::path outdir;
//   USERPROFILE=C:\Users\bfr
//   HOMEDRIVE=C:
//   HOMEPATH=\Users\bfr
fs::path home_dir(getenv("HOME"));
std::cout << "Temp directory is " << fs::temp_directory_path() << '\n';
std::FILE* tmpf = std::tmpfile(); // auto delete
