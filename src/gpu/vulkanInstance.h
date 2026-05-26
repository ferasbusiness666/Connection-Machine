#ifndef vulkanInstance_h
#define vulkanInstance_h

#include "gpu/vulkanCommon.h"
#include <VkBootstrap.h>

#include "vulkanDevice.h"

class VulkanInstance {
public:
	VulkanInstance();
	~VulkanInstance();

	VulkanDevice& getDevice();
	VulkanDevice& createOrGetDevice(vk::SurfaceKHR surfaceForPresenting);

	inline vkb::Instance getVkbInstance() { return instance; };
	inline vk::Instance getInstance() { return vk::Instance(instance.instance); }

private:
	vkb::Instance instance;
	std::optional<VulkanDevice> device;
};

#endif /* vulkanInstance_h */
