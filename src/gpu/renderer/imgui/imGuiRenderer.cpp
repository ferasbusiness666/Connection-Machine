#include "imGuiRenderer.h"

#include "gpu/mainRenderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_vulkan.h"

std::map<SDL_Window*, ImGuiRenderer&> imGuiRenderers;
std::mutex m_mutex;

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
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.IniFilename = NULL;

	ImGui::StyleColorsDark();
	if (!ImGui_ImplSDL3_InitForVulkan(m_mainWindow)) {
		throw std::runtime_error("Failed to initialize ImGui SDL3 backend");
	}

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = MainRenderer::get().getVulkanInstance().getVkbInstance().instance;
	initInfo.PhysicalDevice = MainRenderer::get().getVulkanInstance().getDevice()->getDevice().physical_device;
	initInfo.Device = MainRenderer::get().getVulkanInstance().getDevice()->getDevice().device;
	initInfo.QueueFamily = MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueIndex();
	initInfo.Queue = MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueue().queue;
	initInfo.DescriptorPool = m_imguiDescriptorPool;
	initInfo.MinImageCount = m_framesInFlight;
	initInfo.ImageCount = m_framesInFlight;

	initInfo.PipelineInfoMain.RenderPass = m_renderPass;
	initInfo.PipelineInfoMain.Subpass = 0;
	initInfo.PipelineInfoMain.MSAASamples = MainRenderer::get().getVulkanInstance().getDevice()->getMaxUsableSampleCount();

	static bool vulkanFunctionsLoaded = false;
	if (!vulkanFunctionsLoaded) {
		ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, [](const char* name, void*) {
			return vkGetInstanceProcAddr(MainRenderer::get().getVulkanInstance().getVkbInstance(), name);
		});
		vulkanFunctionsLoaded = true;
	}

	if (!ImGui_ImplVulkan_Init(&initInfo)) {
		throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
	}
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

	if (m_surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(MainRenderer::get().getVulkanInstance().getVkbInstance().instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
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
	ImGui::SetCurrentContext(m_context);

	ImGui::Render();
	std::lock_guard guard(MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueLock());
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	m_mutex.unlock();
}

void ImGuiRenderer::allProcessEvent(const SDL_Event& e) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (std::pair<SDL_Window*, ImGuiRenderer&> imGuiRenderer : imGuiRenderers) {
		ImGui::SetCurrentContext(imGuiRenderer.second.m_context);
		ImGui_ImplSDL3_ProcessEvent(&e);
	}
}
