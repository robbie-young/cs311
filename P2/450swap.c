


/* This file assumes that the global variables gui and vul have already been 
declared and configured elsewhere. */

/* When your application shows an animation frame to the user, that frame is a 
raster image, and it is stored in a chunk of memory called a framebuffer. As we 
have discussed in class, most applications are double-buffered, meaning that 
they maintain two framebuffers. At any given time, one framebuffer is being 
shown to the user; the other framebuffer is being computed and will be shown 
next. Some applications are triple-buffered: one framebuffer for showing, 
another one being computed, and another that can start computation if the 
second one completes before it needs to be shown. In fact, there is no 
theoretical limit to how many of these framebuffers there might be.

The framebuffers form a kind of queue called the swap chain. In fact, each 
element in the swap chain contains not just a framebuffer but also a bunch of 
supporting machinery. Altogether the swap chain is a big thing with dozens of 
parts.

This file manages the swap chain. A key question is: How long should the swap 
chain be? Well, your GPU and its representation in Vulkan have a minimum swap 
chain length and a maximum swap chain length. We request one more than the 
minimum. That seems to work well.

The minimum and maximum swap chain lengths depend on how big of framebuffers you 
need, which in turn depends on the window size. Therefore, you need to 
re-construct the entire swap chain whenever the window changes size. (You also 
need to construct it once at the start of your program, after initializing the 
GUI and Vulkan, to get things going.) The window size can change because of two 
things. One is that the user drags the boundary of the window and changes its 
size. Another is that the user minimizes (hides) the window; then its size is 
set to 0x0. */

/* This data structure holds the crucial information about the swap chain. */
typedef struct swapChain swapChain;
struct swapChain {
    VkSwapchainKHR swapChain;
    VkFormat imageFormat;
    VkRenderPass renderPass;
    size_t curFrame;
    /* extent describes the width and height of the framebuffers. */
    VkExtent2D extent;
    /* MAXFRAMESINFLIGHT is defined in main.c. Semaphores and fences are 
    synchronization mechanisms. They prevent conflicts when multiple threads of 
    computation are simultaneously trying to alter the same chunk of memory. */
    VkSemaphore imageAvailSems[MAXFRAMESINFLIGHT];
    VkSemaphore renderDoneSems[MAXFRAMESINFLIGHT];
    VkFence inFlightFences[MAXFRAMESINFLIGHT];
    /* This is the depth buffer. */
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    /* numImages is the length of the swap chain. */
    int numImages;
    /* These four dynamically allocated arrays all have length numImages. */
    VkImage *images;
    VkImageView *imageViews;
    VkFramebuffer *framebuffers;
    VkFence *imagesInFlight;
};

/* This function is called by the frame presenter once per frame. */
void swapIncrementFrame(swapChain *swap) {
    swap->curFrame = (swap->curFrame + 1) % MAXFRAMESINFLIGHT;
}

/* This helper function inspects the available framebuffer formats and picks one 
that's good enough. */
VkSurfaceFormatKHR swapGetSurfaceFormat(
        int availCount, VkSurfaceFormatKHR *availFormats) {
    /* Try to find one that we like. */
    for (int i = 0; i < availCount; i += 1) {
        if (availFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
                availFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availFormats[i];
    }
    /* The first one is probably good enough. */
    return availFormats[0];
}

/* This helper function inspects the available presentation modes and picks one 
that's good enough. */
VkPresentModeKHR swapGetPresentMode(
        int availCount, VkPresentModeKHR *availModes) {
    /* Try to find one that we like. */
    for (int i = 0; i < availCount; i += 1) {
        if (availModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return availModes[i];
    }
    /* The guaranteed one is good enough. */
    return VK_PRESENT_MODE_FIFO_KHR;
}

/* This helper function clips the integer x to the interval [a, b]. */
uint32_t swapClip(uint32_t x, uint32_t a, uint32_t b) {
    if (x >= b)
        return b;
    else if (x <= a)
        return a;
    else
        return x;
}

/* This helper function gets the swap chain extent from the window. */
VkExtent2D swapGetExtent(VkSurfaceCapabilitiesKHR *capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(gui.window, &width, &height);
        VkExtent2D actualExtent = {(uint32_t)width, (uint32_t)height};
        actualExtent.width = swapClip(
            actualExtent.width, capabilities->minImageExtent.width, 
            capabilities->maxImageExtent.width);
        actualExtent.height = swapClip(
            actualExtent.height, capabilities->minImageExtent.height, 
            capabilities->maxImageExtent.height);
        return actualExtent;
    }
}

/* Each member of the swap chain needs an image to hold the color framebuffer. 
Returns an error code (0 on success). On success, don't forget to 
swapFinalizeImages when you're done. */
int swapInitializeImages(swapChain *swap) {
    /* Interrogate the swap chain support details. */
    vulSwapChainSupportDetails details;
    int error = vulInitializeSwapChainSupport(
        &vul, &details, vul.physicalDevice);
    if (error)
        return 3;
    VkSurfaceFormatKHR surfaceFormat = swapGetSurfaceFormat(
        details.formatCount, details.formats);
    VkPresentModeKHR presentMode = swapGetPresentMode(
        details.presentModeCount, details.presentModes);
    VkExtent2D extent = swapGetExtent(&details.capabilities);
    /* Get one more image than the minimum, if we're allowed to. */
    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && 
            imageCount > details.capabilities.maxImageCount)
        imageCount = details.capabilities.maxImageCount;
    /* Prepare to create the swap chain itself. */
    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vul.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vulQueueFamilyIndices indices = vulGetQueueFamilies(
        &vul, vul.physicalDevice);
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily, indices.presentFamily};
    /* Supposedly exclusive mode is faster but requires much configuration. */
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
    }
    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    /* Create the swap chain. */
    if (vkCreateSwapchainKHR(
            vul.device, &createInfo, NULL, &(swap->swapChain)) != VK_SUCCESS) {
        fprintf(stderr, "error: swapInitializeImages: ");
        fprintf(stderr, "vkCreateSwapchainKHR failed\n");
        vulFinalizeSwapChainSupport(&vul, &details);
        return 2;
    }
    /* Obtain the swap chain images and store other important data. */
    vkGetSwapchainImagesKHR(vul.device, swap->swapChain, &imageCount, NULL);
    swap->images = malloc(imageCount * sizeof(VkImage));
    if (swap->images == NULL) {
        fprintf(stderr, "error: swapInitializeImages: malloc failed\n");
        vulFinalizeSwapChainSupport(&vul, &details);
        return 1;
    }
    vkGetSwapchainImagesKHR(
        vul.device, swap->swapChain, &imageCount, swap->images);
    swap->imageFormat = surfaceFormat.format;
    swap->extent = extent;
    swap->numImages = imageCount;
    /* Clean up. */
    vulFinalizeSwapChainSupport(&vul, &details);
    if (VERBOSE) {
        fprintf(
            stderr, "info: swapInitializeImages: length of swap chain is %d\n", 
            swap->numImages);
    }
    return 0;
}

/* Releases the resources backing the swap chain images. */
void swapFinalizeImages(swapChain *swap) {
    vkDestroySwapchainKHR(vul.device, swap->swapChain, NULL);
    free(swap->images);
}

/* Each image in the swap chain needs an image view, so that we can access it. 
Returns an error code (0 on success). On success, don't forget to 
swapFinalizeViews when you're done. */
int swapInitializeViews(swapChain *swap) {
    /* Allocate space on the CPU side. */
    swap->imageViews = malloc(swap->numImages * sizeof(VkImageView));
    if (swap->imageViews == NULL) {
        fprintf(stderr, "error: swapInitializeViews: malloc failed\n");
        return 2;
    }
    /* Initialize each one on the GPU side. */
    for (size_t i = 0; i < swap->numImages; i += 1) {
        int error = imageInitializeView(
            swap->images[i], swap->imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 
            &(swap->imageViews[i]));
        if (error != 0) {
            for (size_t j = 0; j < i; j += 1)
                imageFinalizeView(&(swap->imageViews[j]));
            free(swap->imageViews);
            return 1;
        }
    }
    return 0;
}

/* Releases the resources backing the swap chain images. */
void swapFinalizeViews(swapChain *swap) {
    for (int i = 0; i < swap->numImages; i += 1)
        imageFinalizeView(&(swap->imageViews[i]));
    free(swap->imageViews);
}

/* This helper function scans through a bunch of possible image formats to find 
one meeting the criteria. Returns an error code (0 on success). */
int swapGetSupportedFormat(
        int numCandidates, VkFormat *candidates, VkImageTiling tiling, 
        VkFormatFeatureFlags features, int *elected) {
    for (int i = 0; i < numCandidates; i += 1) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(
            vul.physicalDevice, candidates[i], &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && 
                (props.linearTilingFeatures & features) == features) {
            *elected = i;
            return 0;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && 
                (props.optimalTilingFeatures & features) == features) {
            *elected = i;
            return 0;
        }
    }
    fprintf(stderr, "error: swapGetSupportedFormat: format not found\n");
    return 1;
}

/* This helper function tries to get a usable format for the depth buffer. */
int swapGetDepthFormat(VkFormat *format) {
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, 
        VK_FORMAT_D24_UNORM_S8_UINT};
    int elected;
    int error = swapGetSupportedFormat(
        3, candidates, VK_IMAGE_TILING_OPTIMAL, 
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, &elected);
    if (error == 0)
        *format = candidates[elected];
    return error;
}

/* The render pass tells Vulkan about the framebuffers that will be attached to 
the rendering process: color, depth, etc. This initializer returns an error code 
(0 on success). On success, don't forget to swapFinalizeRenderPass when you're 
done. */
int swapInitializeRenderPass(swapChain *swap) {
    /* Color framebuffer. */
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = swap->imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    /* Depth buffer. */
    VkAttachmentDescription depthAttachment = {0};
    if (swapGetDepthFormat(&depthAttachment.format) != 0)
        return 2;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthAttachmentRef = {0};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    /* Just one subpass. */
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    /* Render pass. */
    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renPassInfo = {0};
    renPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renPassInfo.attachmentCount = 2;
    renPassInfo.pAttachments = attachments;
    renPassInfo.subpassCount = 1;
    renPassInfo.pSubpasses = &subpass;
    renPassInfo.dependencyCount = 1;
    renPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(
            vul.device, &renPassInfo, NULL, 
            &(swap->renderPass)) != VK_SUCCESS) {
        fprintf(stderr, "error: swapInitializeRenderPass: ");
        fprintf(stderr, "vkCreateRenderPass failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the render pass. */
void swapFinalizeRenderPass(swapChain *swap) {
    vkDestroyRenderPass(vul.device, swap->renderPass, NULL);
}

/* A framebuffer is, as far as I can tell, an image specialized to holding 
color, depth, and/or stencil information, to be presented to the user on the 
window's surface. Anyway, this initializer returns an error code (0 on success). 
On success, don't forget to swapFinalizeFramebuffers when you're done. */
int swapInitializeFramebuffers(swapChain *swap) {
    /* Allocate space on the CPU side. */
    swap->framebuffers = malloc(swap->numImages * sizeof(VkFramebuffer));
    if (swap->framebuffers == NULL) {
        fprintf(stderr, "error: swapInitializeFramebuffers: malloc failed\n");
        return 2;
    }
    /* Initialize them on the GPU side. */
    for (size_t i = 0; i < swap->numImages; i += 1) {
        VkImageView attachments[] = {swap->imageViews[i], swap->depthImageView};
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = swap->renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swap->extent.width;
        framebufferInfo.height = swap->extent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(
                vul.device, &framebufferInfo, NULL, 
                &(swap->framebuffers[i])) != VK_SUCCESS) {
            fprintf(stderr, "error: swapInitializeFramebuffers: ");
            fprintf(stderr, "vkCreateFramebuffer failed\n");
            free(swap->framebuffers);
            return 1;
        }
    }
    return 0;
}

/* Releases the resources backing the framebuffers. */
void swapFinalizeFramebuffers(swapChain *swap) {
    for (int i = 0; i < swap->numImages; i += 1)
        vkDestroyFramebuffer(vul.device, swap->framebuffers[i], NULL);
    free(swap->framebuffers);
}

/* Initializes the synchronization primitives of the swap chain. They are 
responsible for ensuring that two pieces of code don't try to alter a single 
chunk of memory at the same time, for example. Returns an error code (0 on 
success). On success, don't forget to swapFinalizeSyncs later. */
int swapInitializeSyncs(swapChain *swap) {
    swap->imagesInFlight = malloc(swap->numImages * sizeof(VkFence));
    if (swap->imagesInFlight == NULL) {
        fprintf(stderr, "error: createSyncs: malloc failed\n");
        return 4;
    }
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < MAXFRAMESINFLIGHT; i += 1) {
        if (vkCreateSemaphore(
                vul.device, &semaphoreInfo, NULL, 
                &(swap->imageAvailSems[i])) != VK_SUCCESS) {
            fprintf(stderr, "error: swapInitializeSyncs: ");
            fprintf(stderr, "vkCreateSemaphore failed\n");
            for (size_t j = 0; j < i; j += 1) {
                vkDestroySemaphore(vul.device, swap->imageAvailSems[j], NULL);
                vkDestroySemaphore(vul.device, swap->renderDoneSems[j], NULL);
                vkDestroyFence(vul.device, swap->inFlightFences[j], NULL);
            }
            free(swap->imagesInFlight);
            return 3;
        }
        if (vkCreateSemaphore(
                vul.device, &semaphoreInfo, NULL, 
                &(swap->renderDoneSems[i])) != VK_SUCCESS) {
            fprintf(stderr, "error: swapInitializeSyncs: ");
            fprintf(stderr, "vkCreateSemaphore failed\n");
            for (size_t j = 0; j < i; j += 1) {
                vkDestroySemaphore(vul.device, swap->imageAvailSems[j], NULL);
                vkDestroySemaphore(vul.device, swap->renderDoneSems[j], NULL);
                vkDestroyFence(vul.device, swap->inFlightFences[j], NULL);
            }
            vkDestroySemaphore(vul.device, swap->imageAvailSems[i], NULL);
            free(swap->imagesInFlight);
            return 2;
        }
        if (vkCreateFence(
                vul.device, &fenceInfo, NULL, 
                &(swap->inFlightFences[i])) != VK_SUCCESS) {
            fprintf(
                stderr, "error: swapInitializeSyncs: vkCreateFence failed\n");
            for (size_t j = 0; j < i; j += 1) {
                vkDestroySemaphore(vul.device, swap->imageAvailSems[j], NULL);
                vkDestroySemaphore(vul.device, swap->renderDoneSems[j], NULL);
                vkDestroyFence(vul.device, swap->inFlightFences[j], NULL);
            }
            vkDestroySemaphore(vul.device, swap->imageAvailSems[i], NULL);
            vkDestroySemaphore(vul.device, swap->renderDoneSems[i], NULL);
            free(swap->imagesInFlight);
            return 1;
        }
    }
    return 0;
}

/* Releases the synchronization primitives. */
void swapFinalizeSyncs(swapChain *swap) {
    for (size_t i = 0; i < MAXFRAMESINFLIGHT; i += 1) {
        vkDestroySemaphore(vul.device, swap->imageAvailSems[i], NULL);
        vkDestroySemaphore(vul.device, swap->renderDoneSems[i], NULL);
        vkDestroyFence(vul.device, swap->inFlightFences[i], NULL);
    }
    free(swap->imagesInFlight);
}

/* Initializes the depth buffer. Returns an error code (0 on success). On 
success, don't forget to swapFinalizeDepthBuffer when you're done. */
int swapInitializeDepthBuffer(swapChain *swap) {
    VkFormat depthFormat;
    if (swapGetDepthFormat(&depthFormat) != 0)
        return 2;
    int error = imageInitialize(
        swap->extent.width, swap->extent.height, depthFormat, 
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(swap->depthImage), 
        &(swap->depthImageMemory));
    if (error != 0)
        return 1;
    error = imageInitializeView(
        swap->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 
        &(swap->depthImageView));
    if (error != 0) {
        imageFinalize(&(swap->depthImage), &(swap->depthImageMemory));
        return 1;
    }
    imageTransitionLayout(
        swap->depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    return 0;
}

/* Releases the resources backing the depth buffer. */
void swapFinalizeDepthBuffer(swapChain *swap) {
    imageFinalizeView(&(swap->depthImageView));
    imageFinalize(&(swap->depthImage), &(swap->depthImageMemory));
}

/* This umbrella function calls all of the above initializers in the correct 
order to initialize all of the swap chain machinery. Returns an error code (0 on 
success). On success, don't forget to call swapFinalize when you're done. */
int swapInitialize(swapChain *swap) {
    swap->curFrame = 0;
    if (swapInitializeImages(swap) != 0)
        return 6;
    if (swapInitializeViews(swap)) {
        swapFinalizeImages(swap);
        return 5;
    }
    if (swapInitializeDepthBuffer(swap) != 0) {
        swapFinalizeViews(swap);
        swapFinalizeImages(swap);
        return 4;
    }
    if (swapInitializeRenderPass(swap) != 0) {
        swapFinalizeDepthBuffer(swap);
        swapFinalizeViews(swap);
        swapFinalizeImages(swap);
        return 3;
    }
    if (swapInitializeFramebuffers(swap) != 0) {
        swapFinalizeRenderPass(swap);
        swapFinalizeDepthBuffer(swap);
        swapFinalizeViews(swap);
        swapFinalizeImages(swap);
        return 2;
    }
    if (swapInitializeSyncs(swap) != 0) {
        swapFinalizeFramebuffers(swap);
        swapFinalizeRenderPass(swap);
        swapFinalizeDepthBuffer(swap);
        swapFinalizeViews(swap);
        swapFinalizeImages(swap);
        return 1;
    }
    return 0;
}

/* Releases all of the resources backing the swap chain machinery. */
void swapFinalize(swapChain *swap) {
    swapFinalizeSyncs(swap);
    swapFinalizeFramebuffers(swap);
    swapFinalizeRenderPass(swap);
    swapFinalizeDepthBuffer(swap);
    swapFinalizeViews(swap);
    swapFinalizeImages(swap);
}


