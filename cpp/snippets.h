try {

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
