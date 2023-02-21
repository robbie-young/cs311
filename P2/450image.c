


/* An image is a chunk of GPU memory, like a buffer but with more structure. I 
think that it's because of how texture mapping accesses memory. Some ways of 
laying out image memory are more efficient for texture mapping than others. 
Anyway, an image view represents part of an image or a reinterpretation of an 
image's format. To use an image, often you must go through an image view. */

/* This file assumes that the global variable vul has already been configured. 
This file is not written in an object-oriented style. I mean, the functions do 
not systematically act on an image datatype as their first argument. */

/* Transitions an image from one layout to another. As you can see in the code, 
this operation requires synchronizations, etc. */
int imageTransitionLayout(
        VkImage image, VkFormat format, VkImageLayout oldLayout, 
        VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = bufBeginSingleTimeCommands();
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
                format == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
            newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | 
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        fprintf(stderr, "error: imageTransitionLayout: ");
        fprintf(stderr, "unsupported layout transition\n");
        return 1;
    }
    vkCmdPipelineBarrier(
        commandBuffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, 
        &barrier);
    bufEndSingleTimeCommands(commandBuffer);
    return 0;
}

/* Copies a buffer into an image. */
void imageCopyBufferToImage(
        VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = bufBeginSingleTimeCommands();
    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(
        commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 
        &region);
    bufEndSingleTimeCommands(commandBuffer);
}

/* Initializes an image. Returns an error code (0 on success). On success, don't 
forget to imageFinalize when you're done. */
int imageInitialize(
        uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
        VkImage *image, VkDeviceMemory *imageMemory) {
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(vul.device, &imageInfo, NULL, image) != VK_SUCCESS) {
        fprintf(stderr, "error: imageInitialize: vkCreateImage failed\n");
        return 3;
    }
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vul.device, *image, &memReqs);
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    uint32_t memType;
    if (vulGetMemoryType(
            &vul, memReqs.memoryTypeBits, properties, &memType) != 0) {
        vkDestroyImage(vul.device, *image, NULL);
        return 2;
    }
    allocInfo.memoryTypeIndex = memType;
    if (vkAllocateMemory(
            vul.device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
        fprintf(stderr, "error: imageInitialize: vkAllocateMemory failed\n");
        vkDestroyImage(vul.device, *image, NULL);
        return 1;
    }
    vkBindImageMemory(vul.device, *image, *imageMemory, 0);
    return 0;
}

/* Releases the resources backing the image. */
void imageFinalize(VkImage *image, VkDeviceMemory *imageMemory) {
    vkFreeMemory(vul.device, *imageMemory, NULL);
    vkDestroyImage(vul.device, *image, NULL);
}

/* Initializes an image view. Returns an error code (0 on success). On success, 
don't forget to imageFinalizeView when you're done. */
int imageInitializeView(
        VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, 
        VkImageView *imageView) {
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(
            vul.device, &viewInfo, NULL, imageView) != VK_SUCCESS) {
        fprintf(
            stderr, "error: imageInitializeView: vkCreateImageView failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the image view. */
void imageFinalizeView(VkImageView *imageView) {
    vkDestroyImageView(vul.device, *imageView, NULL);
}


