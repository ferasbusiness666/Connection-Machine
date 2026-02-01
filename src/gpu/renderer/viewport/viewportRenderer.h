#ifndef viewportRenderer_h
#define viewportRenderer_h

#include <glm/ext/matrix_float4x4.hpp>

#include "logic/chunking/vulkanChunker.h"
#include "gpu/renderer/frameManager.h"
#include "gpu/mainRendererDefs.h"
#include "logic/chunking/chunkRenderer.h"
#include "elements/elementRenderer.h"
#include "grid/gridRenderer.h"

struct ViewportViewData {
	glm::mat4 viewportViewMat;
	std::pair<FPosition, FPosition> viewBounds;
	float viewScale;
	VkExtent2D viewportSize;
};

class ViewportRenderer {
public:
	ViewportRenderer(VulkanDevice* device);
	~ViewportRenderer();

	ViewportViewData getViewData();
	inline VulkanChunker& getChunker() { return chunker; }
	inline const EvalLogicSimulator* getSimulator() {
		std::lock_guard<std::mutex> lock(simulatorMux);
		return simulator;
	}
	inline const Address& getAddress() {
		std::lock_guard<std::mutex> lock(addressMux);
		return address;
	}

	std::vector<BlockPreviewRenderData> getBlockPreviews();
	std::vector<BoxSelectionRenderData> getBoxSelections();
	std::vector<ConnectionPreviewRenderData> getConnectionPreviews();
	std::vector<ArrowRenderData> getArrows();

	// main flow
	void setSimulator(const EvalLogicSimulator* simulator, const Address& address);

	void updateViewFrame(glm::vec2 size);
	void updateView(FPosition topLeft, FPosition bottomRight);

	VkDescriptorSet getLatestImage();

	// elements
	ElementId addSelectionObjectElement(const SelectionObjectElement& selection);
	ElementId addSelectionElement(const SelectionElement& selection);
	void removeSelectionElement(ElementId selection);

	ElementId addBlockPreview(BlockPreview&& blockPreview);
	void shiftBlockPreview(ElementId id, Vector shift);
	void removeBlockPreview(ElementId blockPreview);

	ElementId addConnectionPreview(const ConnectionPreview& connectionPreview);
	void removeConnectionPreview(ElementId connectionPreview);

	ElementId addHalfConnectionPreview(const HalfConnectionPreview& halfConnectionPreview);
	void removeHalfConnectionPreview(ElementId halfConnectionPreview);

private:
	void destroyImages();
	void createImages();
	void createRenderPass();
	void render(Frame& frame);
	void renderToCommandBuffer(Frame& frame, uint32_t imageIndex);
	void renderLoop();

	const EvalLogicSimulator* simulator = nullptr;
	std::mutex simulatorMux;
	Address address;
	std::mutex addressMux;

	// Vulkan
	VulkanChunker chunker;
	VulkanDevice* device;

	// Elements
	ElementId currentElementId = 0;
	std::unordered_multimap<ElementId, BlockPreviewRenderData> blockPreviews;
	std::unordered_map<ElementId, std::vector<BoxSelectionRenderData>> boxSelections;
	std::unordered_map<ElementId, ConnectionPreviewRenderData> connectionPreviews;
	std::unordered_map<ElementId, std::vector<ArrowRenderData>> arrows;
	std::mutex elementsMux;

	// View data
	ViewportViewData newViewData{};
	ViewportViewData viewData{};
	std::mutex viewMux;

	std::atomic<bool> recreateImages = false;

	// renderers
	VkRenderPass renderPass;
	GridRenderer gridRenderer;
	ChunkRenderer chunkRenderer;
	ElementRenderer elementRenderer;

	VkSampler sampler;

	FrameManager frames;
	std::thread renderThread;
	std::atomic<bool> running = false;

	// Double-buffered images for offscreen rendering
	std::vector<VkFramebuffer> framebuffers;
	std::vector<AllocatedImage> msaaImages;      // MSAA render targets
	std::vector<AllocatedImage> resolveImages;   // Resolved images for ImGui
	std::vector<VkDescriptorSet> imguiTextures;  // ImGui descriptor sets
	VkDescriptorSet imguiReadTexture = VK_NULL_HANDLE; // Current texture for ImGui to read
	std::mutex imageMux; // Protects imguiReadTexture
	std::atomic<bool> imagesReady = false; // Track if images are fully initialized
};

#endif