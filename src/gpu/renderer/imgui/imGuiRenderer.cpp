#include "imGuiRenderer.h"

#include "app.h"
#include "gpu/mainRenderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_vulkan.h"

std::map<SDL_Window*, ImGuiRenderer&> imGuiRenderers;
std::mutex m_mutex;

static void check_vk_result(VkResult err) {
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

ImGuiRenderer::ImGuiRenderer(SDL_Window* window, VkRenderPass renderPass, uint32_t framesInFlight) :
	m_mainWindow(window), m_renderPass(renderPass), m_framesInFlight(framesInFlight), m_context(nullptr), m_imguiDescriptorPool(VK_NULL_HANDLE) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto result = imGuiRenderers.emplace(m_mainWindow, *this);
	assert(result.second);

	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000 * std::size(poolSizes);
	poolInfo.poolSizeCount = std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(MainRenderer::get().getVulkanInstance().getDevice()->getDevice().device, &poolInfo, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create ImGui descriptor pool");
	}

	// init ImGui
	m_context = ImGui::CreateContext();

	ImGui::SetCurrentContext(m_context);
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	// When viewports are enabled, tweak WindowRounding/WindowBg for platform windows
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;

		// Ensure platform windows can be created properly
		// io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
		// io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
	}

	if (!ImGui_ImplSDL3_InitForVulkan(m_mainWindow)) {
		throw std::runtime_error("Failed to initialize ImGui SDL3 backend");
	}

	// initInfo.Instance = MainRenderer::get().getVulkanInstance().getVkbInstance().instance;
	// initInfo.PhysicalDevice = MainRenderer::get().getVulkanInstance().getDevice()->getDevice().physical_device;
	// initInfo.Device = MainRenderer::get().getVulkanInstance().getDevice()->getDevice().device;
	// initInfo.QueueFamily = MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueIndex();
	// initInfo.Queue = MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueue().queue;
	// initInfo.DescriptorPool = ;
	// initInfo.MinImageCount = ;
	// initInfo.ImageCount = ;

	// initInfo.PipelineInfoMain.RenderPass = m_renderPass;
	// initInfo.PipelineInfoMain.Subpass = 0;
	// initInfo.PipelineInfoMain.MSAASamples = MainRenderer::get().getVulkanInstance().getDevice()->getMaxUsableSampleCount();

	// For platform windows (viewports), we need to specify UseDynamicRendering or provide a compatible render pass
	// ImGui will create surfaces and swapchains for secondary viewports automatically
	// Platform windows don't use MSAA - they use simple 1-sample rendering
	// initInfo.UseDynamicRendering = false;

	// Set up info for platform windows (secondary viewports) - no MSAA
	// initInfo.PipelineInfoForViewports.RenderPass = VK_NULL_HANDLE; // Will be created by ImGui
	// initInfo.PipelineInfoForViewports.Subpass = 0;
	// initInfo.PipelineInfoForViewports.MSAASamples = MainRenderer::get().getVulkanInstance().getDevice()->getMaxUsableSampleCount(); // Platform windows don't use MSAA

	static bool vulkanFunctionsLoaded = false;
	if (!vulkanFunctionsLoaded) {
		ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, [](const char* name, void*) {
			return vkGetInstanceProcAddr(MainRenderer::get().getVulkanInstance().getVkbInstance(), name);
		});
		vulkanFunctionsLoaded = true;
	}

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = MainRenderer::get().getVulkanInstance().getVkbInstance();
    init_info.PhysicalDevice = MainRenderer::get().getVulkanInstance().getDevice()->getDevice().physical_device;
    init_info.Device = MainRenderer::get().getVulkanInstance().getDevice()->getDevice().device;
    init_info.QueueFamily = MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueIndex();
    init_info.Queue = MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueue().queue;
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.MinImageCount = m_framesInFlight;
    init_info.ImageCount = m_framesInFlight;
    init_info.PipelineInfoMain.RenderPass = renderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = MainRenderer::get().getVulkanInstance().getDevice()->getMaxUsableSampleCount();

	// init_info.UseDynamicRendering = true;
	// init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	// init_info.PipelineInfoForViewports.RenderPass = renderPass;
	// init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	// std::array<VkFormat, 1> swapchainImageFormat = {VK_FORMAT_B8G8R8A8_UNORM};
	// init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo.pColorAttachmentFormats = swapchainImageFormat.data();
    ImGui_ImplVulkan_Init(&init_info);
}

ImGuiRenderer::~ImGuiRenderer() {
	std::lock_guard<std::mutex> lock(m_mutex);
	imGuiRenderers.erase(m_mainWindow);

	if (!m_context) return;

	ImGui::SetCurrentContext(m_context);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();

	ImGui::DestroyContext(m_context);
	m_context = nullptr;

	// Clean up Vulkan resources
	if (m_imguiDescriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(MainRenderer::get().getVulkanInstance().getDevice()->getDevice().device, m_imguiDescriptorPool, nullptr);
		m_imguiDescriptorPool = VK_NULL_HANDLE;
	}
}

void ImGuiRenderer::beginFrame() {
	m_mutex.lock();

	ImGui::SetCurrentContext(m_context);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void ImGuiRenderer::endFrame(VkCommandBuffer cmd) {
	m_mutex.unlock();

	App::runOnMain_blocking([this]() {
		ImGui::SetCurrentContext(m_context);
		ImGui::Render();
	});


	std::lock_guard lock(m_mutex);

	ImGui::SetCurrentContext(m_context);

	// Render main window
	{
		std::lock_guard guard(MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueLock());
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	}

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void ImGuiRenderer::allProcessEvent(const SDL_Event& e) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (std::pair<SDL_Window*, ImGuiRenderer&> imGuiRenderer : imGuiRenderers) {
		ImGui::SetCurrentContext(imGuiRenderer.second.m_context);
		ImGui_ImplSDL3_ProcessEvent(&e);
	}
}