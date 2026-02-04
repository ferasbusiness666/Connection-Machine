#ifndef viewportRenderer_h
#define viewportRenderer_h

#include <glm/ext/matrix_float4x4.hpp>

#include "gpu/abstractions/vulkanImageSwapchain.h"
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

	// std::pair<VkDescriptorSet, std::shared_ptr<void>> getLatestImage();

	// float getFps() const { return fps.load(); }

	std::pair<VkDescriptorSet, VkSemaphore> startImageRender();

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
	// void renderLoop();

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

	// renderers
	VkRenderPass renderPass;
	GridRenderer gridRenderer;
	ChunkRenderer chunkRenderer;
	ElementRenderer elementRenderer;

	VkSampler sampler;

	std::mutex framesMutex;
	FrameManager frames;

	std::atomic<int> currentBorrowedImage = -1;
	AllocatedImage msaaImage;
	ImageSwapchain imageSwapchain;
	std::vector<VkDescriptorSet> imguiTextures;  // ImGui descriptor sets
	std::atomic<bool> imageReady = false;
	std::atomic<unsigned int> imagesReady = 0;
	std::atomic<bool> updateViewData = true;
	std::mutex imageMux;

	std::chrono::time_point<std::chrono::high_resolution_clock> lastUpdateRender;
};

#endif