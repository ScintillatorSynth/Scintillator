#ifndef SRC_FILE_SYSTEM_HPP_
#define SRC_FILE_SYSTEM_HPP_

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem
#endif

#endif  // SRC_FILE_SYSTEM_HPP_
