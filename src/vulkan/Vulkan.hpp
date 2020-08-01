#ifndef SRC_SCIN_INCLUDE_VULKAN_HPP_
#define SRC_SCIN_INCLUDE_VULKAN_HPP_

#if _MSC_VER
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define GLFW_INCLUDE_NONE // Keep GLFW from including GL headers
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#endif // SRC_SCIN_INCLUDE_VULKAN_HPP_
