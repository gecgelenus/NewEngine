#include "Interface.hpp"


VkDescriptorPool Interface::descriptorPool = nullptr;
vk_ctx Interface::context = {};


Interface::Interface(const vk_ctx& p_context)
{
	this->context = p_context;
}

Interface::~Interface()
{
}

void Interface::init()
{

    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;


    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 100 }, // For samplers directly (less common for ImGui but good to have)
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }, // **CRUCIAL for ImGui's font atlas and any images you render with ImGui**
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 }, // If you explicitly bind sampled images
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Allows individual descriptor sets to be freed
    pool_info.maxSets = 100 * 9; // Max number of descriptor sets that can be allocated
    pool_info.poolSizeCount = 9;
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(context.device, &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ImGui descriptor pool!");
    }


    ImGui_ImplGlfw_InitForVulkan(context.window, true);

    VkPipelineRenderingCreateInfoKHR pipeline_create{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    pipeline_create.pNext = VK_NULL_HANDLE;
    pipeline_create.colorAttachmentCount = 1;
    pipeline_create.pColorAttachmentFormats = &(context.swapchainImageFormat);
    pipeline_create.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
    pipeline_create.stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    ImGui_ImplVulkan_InitInfo initInfo;
    initInfo.DescriptorPool = descriptorPool;
    initInfo.DescriptorPoolSize = 100 * 9;

    initInfo.Instance = context.instance;
    initInfo.Queue = context.graphicsQueue;
    initInfo.QueueFamily = 0;
    initInfo.Subpass = 0;
    initInfo.CheckVkResultFn = NULL;
    initInfo.PipelineCache = nullptr;
    initInfo.UseDynamicRendering = false;
    initInfo.Allocator = nullptr;
    initInfo.RenderPass = nullptr;
    initInfo.Device = context.device;
    initInfo.PhysicalDevice = context.physicalDevice;
    initInfo.ImageCount = context.swapchainImageViews.size();
    initInfo.MinImageCount = context.minImage;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.MinAllocationSize = 1024 * 1024;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = pipeline_create;

    ImGui_ImplVulkan_Init(&initInfo);



    vkDeviceWaitIdle(context.device);
}

void Interface::start_frame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}


