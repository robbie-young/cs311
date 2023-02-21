


/* This file assumes that the global variable vul has already been configured. 
This file is not written in an object-oriented style. I mean, the functions do 
not systematically act on any specific data type as their first argument. */

/* A sampler holds texture-mapping settings such as which kind of filtering to 
use and whether to repeat or clamp (clip) at the boundaries. A single sampler 
can be used with multiple textures. This function initializes a sampler. Popular 
values for the address modes are VK_SAMPLER_ADDRESS_MODE_REPEAT, 
VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE. 
Returns an error code (0 on success). On success, don't forget to 
texFinalizeSampler when you're done. */
int texInitializeSampler(
        VkSampler *sampler, VkSamplerAddressMode addrModeU, 
        VkSamplerAddressMode addrModeV) {
    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = addrModeU;
    samplerInfo.addressModeV = addrModeV;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0;
    samplerInfo.minLod = 0.0;
    samplerInfo.maxLod = 0.0;
    if (vkCreateSampler(
            vul.device, &samplerInfo, NULL, sampler) != VK_SUCCESS) {
        fprintf(stderr, "error: sampInitialize: vkCreateSampler failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the sampler. */
void texFinalizeSampler(VkSampler *sampler) {
    vkDestroySampler(vul.device, *sampler, NULL);
}

/* Initializes a texture from a file. Uses the STB image library, which doesn't 
handle every image format that exists. So, if your program isn't working, then 
consider trying a different image file. Returns an error code (0 on success). On 
success, don't forget to texFinalize when you're done.*/
int texInitializeFile(
        VkImage *texIm, VkDeviceMemory *texImMem, VkImageView *texImView, 
        const char *fileName) {
    /* Load the image from file. */
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(
        fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (pixels == NULL) {
        fprintf(stderr, "error: texInitializeFile: stbi_load failed\n");
        return 6;
    }
    /* Copy the image into a staging buffer. */
    VkBuffer stagBuf;
    VkDeviceMemory stagBufMem;
    int error = bufInitialize(
        imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagBuf, &stagBufMem);
    if (error != 0) {
        stbi_image_free(pixels);
        return 5;
    }
    void *data;
    vkMapMemory(vul.device, stagBufMem, 0, imageSize, 0, &data);
    memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(vul.device, stagBufMem);
    stbi_image_free(pixels);
    /* Copy the staging buffer into an image suitable for texture sampling. */
    error = imageInitialize(
            texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
            VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            texIm, texImMem);
    if (error != 0) {
        bufFinalize(&stagBuf, &stagBufMem);
        return 4;
    }
    error = imageTransitionLayout(
        *texIm, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (error != 0) {
        bufFinalize(&stagBuf, &stagBufMem);
        return 3;
    }
    imageCopyBufferToImage(
        stagBuf, *texIm, (uint32_t)texWidth, (uint32_t)texHeight);
    error = imageTransitionLayout(
        *texIm, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (error != 0) {
        bufFinalize(&stagBuf, &stagBufMem);
        return 2;
    }
    bufFinalize(&stagBuf, &stagBufMem);
    error = imageInitializeView(
        *texIm, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texImView);
    if (error != 0)
        return 1;
    return 0;
}

/* Releases the resources backing the texture. */
void texFinalize(
        VkImage *texIm, VkDeviceMemory *texImMem, VkImageView *texImView) {
    imageFinalizeView(texImView);
    imageFinalize(texIm, texImMem);
}


