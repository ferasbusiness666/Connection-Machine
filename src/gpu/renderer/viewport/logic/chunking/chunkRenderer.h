#ifndef chunkRenderer_h
#define chunkRenderer_h

#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "gpu/renderer/frameManager.h"
#include "gpu/abstractions/vulkanPipeline.h"
#include "vulkanChunker.h"

struct ChunkPushConstants {
	glm::mat4 mvp;
};

class ChunkRenderer {
public:
	void init(VulkanDevice& device, VkRenderPass& renderPass);
	void cleanup();

	void render(Frame& frame, const glm::mat4& viewMatrix, const EvalLogicSimulator* simulator, const Address& address, const std::vector<std::shared_ptr<VulkanLogicAllocation>>& chunks);

private:
	Pipeline blockPipeline;
	Pipeline wirePipeline;

	// state buffer descriptor layout
	VkDescriptorSetLayout stateBufferDescriptorSetLayout;

	// refs
	VulkanDevice* device;
};

#endif
