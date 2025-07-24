#pragma once
#include "util.hpp"
#include "vulkan_context.hpp"


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

class Interface {


public:

	Interface(const vk_ctx&);
	~Interface();


	static void init();
	static void start_frame();




	static VkDescriptorPool descriptorPool;
	static vk_ctx context;



};