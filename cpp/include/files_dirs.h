#ifndef FILES_DIRS_H
#define FILES_DIRS_H

#include <iostream>
#include <string>
#include "strings_etc.h"
#include <filesystem>
#include <functional>
// code snipp recursive copy
//try
//{
//    std::filesystem::copy(src, target, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
//}
//catch (std::exception& e)
//{
//    std::cout << e.what();
//}

// code snipp recursive predicate from strackoverflow
// Copy only those files which contain "Sub" in their stem.
//const auto filter = [](const std::filesystem::path& p) -> bool
//{
//    return p.stem().generic_string().find("Sub") != std::string::npos;
//};

// from stackoverflow
// Recursively copies those files and folders from src to target which matches
// predicate, and overwrites existing files in target.

namespace fdirs {

const auto no_ats_filter = [](const std::filesystem::path& p) -> bool
{
    return p.stem().generic_string().find("Sub") != std::string::npos;
};

const auto no_measdir_filter = [](const std::filesystem::path& p) -> bool
{
    return p.stem().generic_string().find("Sub") != std::string::npos;
};

template <typename T>
T first_gap( std::vector<T> &v )
{
    // Handle the special case of an empty vector.  Return 1.
    if( v.empty() ) return T(1);

    // Sort the vector
    std::sort( v.begin(), v.end() );

    // Find the first adjacent pair that differ by more than 1.
    auto i = std::adjacent_find( v.begin(), v.end(), [](int l, int r){return l+1<r;} );

    // Handle the special case of no gaps.  Return the last value + 1.
    if ( i == v.end() ) --i;

    return 1 + *i;
}

/*!
 * \brief scan_runs returns the firts free number of run_001 ... run_003, run_004
 * \param station_dir
 * \return here it would be 2
 */
size_t scan_runs(const std::filesystem::path& station_dir) {
    std::vector<size_t> iruns;
    for (const auto & entry : std::filesystem::directory_iterator(station_dir)) {
        //std::cout << entry.path() << std::endl;
        if (std::filesystem::is_directory(entry)) {
            auto i = mstr::string2run(entry.path().filename().c_str());
            if (i != SIZE_MAX) iruns.push_back(i);
            else return SIZE_MAX;
        }
    }
    return fdirs::first_gap(iruns);
}

std::filesystem::path meta_dir(const std::filesystem::path &dir ) {
    std::filesystem::path p;
    for (const auto &e : dir) {
        if (e == "stations") p /= "meta";
        else p /= e;
    }
    return p;

}


void CopyRecursive(const std::filesystem::path& src, const std::filesystem::path& target,
                   const std::function<bool(std::filesystem::path)>& predicate /* or use template */) noexcept
{
    try
    {
        for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(src))
        {
            const auto& p = dirEntry.path();
            if (predicate(p))
            {
                // Create path in target, if not existing.
                const auto relativeSrc = std::filesystem::relative(p, src);
                const auto targetParentPath = target / relativeSrc.parent_path();
                std::filesystem::create_directories(targetParentPath);

                // Copy to the targetParentPath which we just created.
                std::filesystem::copy(p, targetParentPath, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
    }
}



/*
// https://stackoverflow.com/questions/51431425/how-to-recursively-copy-files-and-directories
#include <filesystem>
#include <iostream>
#include <functional>
namespace fs = std::filesystem;

int main()
{
    const auto root = fs::current_path();
    const auto src = root / "src";
    const auto target = root / "target";

    // Copy only those files which contain "Sub" in their stem.
    const auto filter = [](const fs::path& p) -> bool
    {
        return p.stem().generic_string().find("Sub") != std::string::npos;
    };
    CopyRecursive(src, target, filter);
}
*/
}
#endif // FILES_DIRS_H
