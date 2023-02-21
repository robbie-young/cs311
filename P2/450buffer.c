


/* A buffer is a chunk of GPU memory with very little structure. We use buffers 
to store data such as meshes and textures on the GPU, so that Vulkan can access 
them quickly when they're needed. (For textures we also use images, which are 
like buffers but with more features.) */

/* This file assumes that the global variable vul has already been configured. 
This file is not written in an object-oriented style. I mean, the functions do 
not systematically act on a buffer datatype as their first argument. */

/* Begins recording an ad hoc command buffer. */
VkCommandBuffer bufBeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vul.commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vul.device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

/* Finishes recording an ad hoc command buffer. */
void bufEndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(vul.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vul.graphicsQueue);
    vkFreeCommandBuffers(vul.device, vul.commandPool, 1, &commandBuffer);
}

/* This function initializes a buffer object, including its backing memory. 
Returns an error code (0 on success). On success, don't forget to call 
bufFinalize when you're done. */
int bufInitialize(
        VkDeviceSize size, VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties, VkBuffer *buf, 
        VkDeviceMemory *bufMem) {
    VkBufferCreateInfo bufInfo = {0};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = size;
    bufInfo.usage = usage;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(vul.device, &bufInfo, NULL, buf) != VK_SUCCESS) {
        fprintf(stderr, "error: bufInitialize: vkCreateBuffer failed\n");
        return 3;
    }
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vul.device, *buf, &memReqs);
    uint32_t memType;
    if (vulGetMemoryType(
            &vul, memReqs.memoryTypeBits, properties, &memType) != 0) {
        vkDestroyBuffer(vul.device, *buf, NULL);
        return 2;
    }
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memType;
    if (vkAllocateMemory(vul.device, &allocInfo, NULL, bufMem) != VK_SUCCESS) {
        fprintf(stderr, "error: bufInitialize: vkAllocateMemory failed\n");
        vkDestroyBuffer(vul.device, *buf, NULL);
        return 1;
    }
    vkBindBufferMemory(vul.device, *buf, *bufMem, 0);
    return 0;
}

/* Copies data from one buffer to another. */
void bufCopy(VkBuffer srcBuf, VkBuffer dstBuf, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = bufBeginSingleTimeCommands();
    VkBufferCopy copyRegion = {0};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuf, dstBuf, 1, &copyRegion);
    bufEndSingleTimeCommands(commandBuffer);
}

/* Releases the resources backing the buffer. */
void bufFinalize(VkBuffer *buf, VkDeviceMemory *bufMem) {
    vkFreeMemory(vul.device, *bufMem, NULL);
    vkDestroyBuffer(vul.device, *buf, NULL);
}


