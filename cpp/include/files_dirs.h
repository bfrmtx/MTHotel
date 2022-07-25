#ifndef FILES_DIRS_H
#define FILES_DIRS_H

#include <string>
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

const auto no_ats_filter = [](const std::filesystem::path& p) -> bool
{
    return p.stem().generic_string().find("Sub") != std::string::npos;
};

const auto no_measdir_filter = [](const std::filesystem::path& p) -> bool
{
    return p.stem().generic_string().find("Sub") != std::string::npos;
};



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

#endif // FILES_DIRS_H
