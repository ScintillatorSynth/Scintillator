#ifndef SRC_CORE_FILE_SYSTEM_HPP_
#define SRC_CORE_FILE_SYSTEM_HPP_

// TODO: MacOS didn't start fully supporting <filesystem> fully until 10.15. Remove this third-party library once the
// minimum required version of Scintillator is 10.15
#if (__APPLE__)
#    include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#    if __has_include(<filesystem>)
#        include <filesystem>
namespace fs = std::filesystem;
#    else
#        include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#    endif
#endif // __OSX__

#endif // SRC_CORE_FILE_SYSTEM_HPP_
