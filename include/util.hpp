#pragma once

#include <vulkan/vulkan.hpp>

#include <fstream>
#include "colorlog.h"
#include <stddef.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>         // For glm::make_mat4, glm::value_ptr (useful for copying to UBOs)
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#define UTIL_CTX "Utility"

#define SIZE_BYTE 1
#define SIZE_KB SIZE_BYTE*1024
#define SIZE_MB SIZE_KB*1024
#define SIZE_GB SIZE_MB*1024



struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};




namespace util{
	std::vector<char> readFile(const std::string& filename);
}