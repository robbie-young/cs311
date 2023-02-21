


/* This file assumes that the global variable gui has already been declared and 
configured elsewhere. */

/* This data structure holds the crucial information about the Vulkan instance, 
physical devices, and logical devices. */
typedef struct vulVulkan vulVulkan;
struct vulVulkan {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkCommandPool commandPool;
    VkPhysicalDeviceProperties physicalDeviceProperties;
};

/* When you use Vulkan, you submit commands to certain queues. Those queues are 
obtained from queue families. GPUs may support various queue families. A key 
part of our Vulkan initialization process is obtaining a GPU that supports 
making graphics and presenting them to the screen. */
typedef struct vulQueueFamilyIndices vulQueueFamilyIndices;
struct vulQueueFamilyIndices {
    int hasGraphicsFamily;
    uint32_t graphicsFamily;
    int hasPresentFamily;
    uint32_t presentFamily;
};

/* Interrogates a physical device (GPU) to determine its support for graphics 
and presentation. */
vulQueueFamilyIndices vulGetQueueFamilies(
        vulVulkan *vul, VkPhysicalDevice device) {
    /* Initialize enough that we can communicate errors. */
    vulQueueFamilyIndices indices;
    indices.hasGraphicsFamily = 0;
    indices.hasPresentFamily = 0;
    /* Get the queue families for this device. */
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilies = malloc(
        queueFamilyCount * sizeof(VkQueueFamilyProperties));
    if (queueFamilies == NULL) {
        fprintf(stderr, "error: vulGetQueueFamilies: malloc failed\n");
        return indices;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queueFamilyCount, queueFamilies);
    /* Find queue families that support graphics and presentation. */
    for (int i = 0; i < queueFamilyCount; i += 1) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.hasGraphicsFamily = 1;
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            device, i, vul->surface, &presentSupport);
        if (presentSupport) {
            indices.hasPresentFamily = 1;
            indices.presentFamily = i;
        }
    }
    free(queueFamilies);
    return indices;
}

/* The swap chain is a sequence of framebuffers. At any given time, one is being 
shown on the screen, and one or more of the others is under construction. A key 
part of our Vulkan initialization process is finding a GPU that supports swap 
chains with the features that we need. */
typedef struct vulSwapChainSupportDetails vulSwapChainSupportDetails;
struct vulSwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR *formats;
    uint32_t presentModeCount;
    VkPresentModeKHR *presentModes;
};

/* Queries a physical device for its supported swap chain features. Returns an 
error code (0 on success). On success, don't forget to call 
vulFinalizeSwapChainSupport when you're done. */
int vulInitializeSwapChainSupport(
        vulVulkan *vul, vulSwapChainSupportDetails *details, 
        VkPhysicalDevice device) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device, vul->surface, &(details->capabilities));
    /* Get surface format modes. */
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, vul->surface, &(details->formatCount), NULL);
    if (details->formatCount != 0) {
        details->formats = malloc(
            details->formatCount * sizeof(VkSurfaceFormatKHR));
        if (details->formats == NULL) {
            fprintf(stderr, "error: vulInitializeSwapChainSupport: ");
            fprintf(stderr, "malloc failed\n");
            return 2;
        }
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, vul->surface, &(details->formatCount), details->formats);
    } else
        details->formats = NULL;
    /* Get surface presentation modes. */
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, vul->surface, &(details->presentModeCount), NULL);
    if (details->presentModeCount != 0) {
        details->presentModes = malloc(
            details->presentModeCount * sizeof(VkPresentModeKHR));
        if (details->presentModes == NULL) {
            fprintf(stderr, "error: initSwapChainSupport: malloc failed\n");
            if (details->formats != NULL)
                free(details->formats);
            return 1;
        }
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, vul->surface, &(details->presentModeCount), 
            details->presentModes);
    } else
        details->presentModes = NULL;
    return 0;
}

/* Releases the resources backing the swap chain support queries. */
void vulFinalizeSwapChainSupport(
        vulVulkan *vul, vulSwapChainSupportDetails *details) {
    if (details->formats != NULL) {
        free(details->formats);
        details->formats = NULL;
    }
    if (details->presentModes != NULL) {
        free(details->presentModes);
        details->presentModes = NULL;
    }
}

/* Scans the physical device's memory types, trying to find one that satisfies 
the stated requirements. Returns an error code (0 on success). */
int vulGetMemoryType(
        vulVulkan *vul, uint32_t typeFilter, VkMemoryPropertyFlags props, 
        uint32_t *memType) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(vul->physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i += 1) {
        if ((typeFilter & (1 << i)) && 
                (memProps.memoryTypes[i].propertyFlags & props) == props) {
            *memType = i;
            return 0;
        }
    }
    fprintf(stderr, "error: vulGetMemoryType: failed to find suitable type\n");
    return 1;
}

/* A validation layer is a bug-detection tool. This function scans Vulkan's 
available validation layers, making sure that all of the ones that we need are 
present. It returns an error code (0 on success). */
int vulValLayerSupportError(vulVulkan *vul) {
    if (NUMVALLAYERS > MAXVALLAYERS) {
        fprintf(
            stderr, "error: vulValLayerSupportError: NUMVALLAYERS > %d\n", 
            MAXVALLAYERS);
        return 2;
    }
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties *availLayers = malloc(
        layerCount * sizeof(VkLayerProperties));
    if (availLayers == NULL) {
        fprintf(stderr, "error: vulValLayerSupportError: malloc failed\n");
        return 1;
    }
    vkEnumerateInstanceLayerProperties(&layerCount, availLayers);
    for (int i = 0; i < NUMVALLAYERS; i += 1) {
        int layerFound = 0;
        for (int j = 0; j < layerCount; j += 1) {
            if (strcmp(valLayers[i], availLayers[j].layerName) == 0) {
                layerFound = 1;
                break;
            }
        }
        if (!layerFound) {
            fprintf(
                stderr, "error: vulValLayerSupportError: %s not available\n", 
                valLayers[i]);
            free(availLayers);
            return 1;
        }
    }
    free(availLayers);
    return 0;
}

/* An instance is your application's connection to the Vulkan system. Your 
application's instance is independent of instances being used by other running 
applications. This function initializes the instance with some meta-data about 
your application. It enables certain extensions and validation layers. (An 
extension is a feature that is not in core Vulkan, but which can be requested, 
if it is supported by the device.) Returns an error code (0 on success). On 
success, don't forget to vulFinalizeInstance when you're done. */
int vulInitializeInstance(vulVulkan *vul) {
    /* Check availability of validation layers. */
    if (NUMVALLAYERS >= 1 && vulValLayerSupportError(vul))
        return 3;
    /* Specify application information. */
    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Carleton College 2022 Fall CS 311";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    appInfo.pNext = NULL;
    /* Specify global (not device-specific) instance information. */
    VkInstanceCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount + NUMINSTANCEEXT;
    char **ext = malloc(createInfo.enabledExtensionCount * sizeof(char *));
    if (ext == NULL) {
        fprintf(stderr, "error: vulInitializeInstance: malloc failed\n");
        return 2;
    }
    for (int i = 0; i < glfwExtensionCount; i += 1)
        ext[i] = (char *)glfwExtensions[i];
    for (int i = 0; i < NUMINSTANCEEXT; i += 1)
        ext[glfwExtensionCount + i] = (char *)instanceExtensions[i];
    createInfo.ppEnabledExtensionNames = (const char *const *)ext;
    if (NUMVALLAYERS >= 1) {
        createInfo.enabledLayerCount = (uint32_t)NUMVALLAYERS;
        createInfo.ppEnabledLayerNames = valLayers;
    } else
        createInfo.enabledLayerCount = 0;
    /* Check for extension support. */
    if (VERBOSE) {
        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(NULL, &extCount, NULL);
        VkExtensionProperties *extensions = malloc(
            extCount * sizeof(VkExtensionProperties));
        if (extensions == NULL) {
            fprintf(stderr, "error: vulInitializeInstance: malloc failed\n");
            return 2;
        }
        vkEnumerateInstanceExtensionProperties(NULL, &extCount, extensions);
        fprintf(stderr, "info: vulInitializeInstance: available extensions:\n");
        for (int i = 0; i < extCount; i += 1)
            fprintf(stderr, "\t%s\n", extensions[i].extensionName);
        free(extensions);
    }
    /* Create the Vulkan instance. */
    VkResult result = vkCreateInstance(&createInfo, NULL, &(vul->instance));
    if (result != VK_SUCCESS) {
        fprintf(
            stderr, "error: vulInitializeInstance: vkCreateInstance failed\n");
        free(ext);
        return 1;
    }
    free(ext);
    return 0;
}

/* Releases the resources backing the Vulkan instance. */
void vulFinalizeInstance(vulVulkan *vul) {
    vkDestroyInstance(vul->instance, NULL);
}

/* The surface is the part of the window, into which Vulkan displays. This 
initializer returns an error code (0 on success). On success, don't forget to 
vulFinalizeSurface when you're done. */
int vulInitializeSurface(vulVulkan *vul) {
    if (glfwCreateWindowSurface(
            vul->instance, gui.window, NULL, &(vul->surface)) != VK_SUCCESS) {
        fprintf(stderr, "error: vulInitializeSurface: ");
        fprintf(stderr, "glfwCreateWindowSurface failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the surface. */
void vulFinalizeSurface(vulVulkan *vul) {
    vkDestroySurfaceKHR(vul->instance, vul->surface, NULL);
}

/* Queries the physical device to make sure that all of our necessary device 
extensions are supported. Returns an error code (0 on success). */
int vulExtensionSupportError(vulVulkan *vul, VkPhysicalDevice device) {
    /* Get a list of available extensions. */
    uint32_t extCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extCount, NULL);
    VkExtensionProperties *availExt = malloc(
        extCount * sizeof(VkExtensionProperties));
    if (availExt == NULL) {
        fprintf(stderr, "error: vulExtensionSupportError: malloc failed\n");
        return 2;
    }
    vkEnumerateDeviceExtensionProperties(device, NULL, &extCount, availExt);
    /* Make sure each device extension is in the available extensions. */
    for (int i = 0; i < NUMDEVICEEXT; i += 1) {
        int found = 0;
        for (int j = 0; j < extCount; j += 1) {
            if (strcmp(deviceExtensions[i], availExt[j].extensionName) == 0)
                found = 1;
        }
        if (!found) {
            fprintf(
                stderr, 
                "error: vulExtensionSupportError: extension %s not available\n", 
                deviceExtensions[i]);
            free(availExt);
            return 1;
        }
    }
    free(availExt);
    return 0;
}

/* Combines several of the query functions above to determine whether the 
physical device meets all of our requirements. Returns 1 if the device is 
suitable and 0 if not. */
int vulIsDeviceSuitable(vulVulkan *vul, VkPhysicalDevice device) {
    vulQueueFamilyIndices indices = vulGetQueueFamilies(vul, device);
    int isComplete = indices.hasGraphicsFamily && indices.hasPresentFamily;
    int extSuppError = vulExtensionSupportError(vul, device);
    int swapChainAdequate = 0;
    if (!extSuppError) {
        vulSwapChainSupportDetails details;
        int swapError = vulInitializeSwapChainSupport(vul, &details, device);
        swapChainAdequate = !swapError && details.formatCount >= 1 && 
            details.presentModeCount >= 1;
        if (!swapError)
            vulFinalizeSwapChainSupport(vul, &details);
    }
    VkPhysicalDeviceFeatures suppFeatures;
    vkGetPhysicalDeviceFeatures(device, &suppFeatures);
    if (VERBOSE) {
        fprintf(
            stderr, "info: vulIsDeviceSuitable:\n    isComplete = %d\n", 
            isComplete);
        fprintf(stderr, "    !extSuppError = %d\n", !extSuppError);
        fprintf(stderr, "    swapChainAdequate = %d\n", swapChainAdequate);
        fprintf(
            stderr, "    anisotropy = %d\n", suppFeatures.samplerAnisotropy);
    }
    if (ANISOTROPY)
        return isComplete && !extSuppError && swapChainAdequate && 
            suppFeatures.samplerAnisotropy;
    else
        return isComplete && !extSuppError && swapChainAdequate;
}

/* Scans the physical devices and selects one that's suitable for our needs. 
Returns an error code (0 on success). */
int vulSetPhysicalDevice(vulVulkan *vul) {
    vul->physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vul->instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "error: vulSetPhysicalDevice: ");
        fprintf(stderr, "no Vulkan devices detected\n");
        return 3;
    }
    VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    if (devices == NULL) {
        fprintf(stderr, "error: vulSetPhysicalDevice: malloc failed\n");
        return 2;
    }
    vkEnumeratePhysicalDevices(vul->instance, &deviceCount, devices);
    for (int i = 0; i < deviceCount; i += 1) {
        if (vulIsDeviceSuitable(vul, devices[i])) {
            vul->physicalDevice = devices[i];
            break;
        }
    }
    if (vul->physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "error: vulSetPhysicalDevice: no suitable devices\n");
        return 1;
    }
    free(devices);
    vkGetPhysicalDeviceProperties(
        vul->physicalDevice, &(vul->physicalDeviceProperties));
    return 0;
}

/* While a physical device is a physical GPU that you can hold in your hand, a 
logical device is a representation of that device in your application. Our later 
function calls to Vulkan tend to specify the logical device rather than the 
physical device or instance. In fact, the logical device is so central that it 
is often just called the 'device'. This function also obtains the graphics queue 
and the presentation queue. Returns an error code (0 on success). On success, 
don't forget to vulFinalizeLogicalDevice when you're done. */
int vulInitializeLogicalDevice(vulVulkan *vul) {
    /* Find all distinct queue families in an unfortunately hands-on way. */
    vulQueueFamilyIndices indices = vulGetQueueFamilies(
        vul, vul->physicalDevice);
    int uniqueQueueFamiliesCount = 2;
    uint32_t uniqueQueueFamilies[2] = {
        indices.graphicsFamily, indices.presentFamily};
    if (uniqueQueueFamilies[1] == uniqueQueueFamilies[0])
        uniqueQueueFamiliesCount = 1;
    /* Create a VkDeviceQueueCreateInfo for each distinct queue family. */
    VkDeviceQueueCreateInfo *queueCreateInfos = malloc(
        uniqueQueueFamiliesCount * sizeof(VkDeviceQueueCreateInfo));
    if (queueCreateInfos == NULL) {
        fprintf(stderr, "error: vulInitializeLogicalDevice: malloc failed\n");
        return 2;
    }
    float queuePriority = 1.0;
    for (int i = 0; i < uniqueQueueFamiliesCount; i += 1) {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
        queueCreateInfos[i].pNext = NULL;
        queueCreateInfos[i].flags = 0;
    }
    /* Create a VkPhysicalDeviceFeatures. */
    VkPhysicalDeviceFeatures deviceFeatures = {0};
    if (ANISOTROPY)
        deviceFeatures.samplerAnisotropy = VK_TRUE;
    else
        deviceFeatures.samplerAnisotropy = VK_FALSE;
    /* Create a VkDeviceCreateInfo. */
    VkDeviceCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = uniqueQueueFamiliesCount;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = NUMDEVICEEXT;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    if (NUMVALLAYERS >= 1) {
        createInfo.enabledLayerCount = NUMVALLAYERS;
        createInfo.ppEnabledLayerNames = valLayers;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = NULL;
    }
    if (vkCreateDevice(
            vul->physicalDevice, &createInfo, NULL, 
            &(vul->device)) != VK_SUCCESS) {
        fprintf(stderr, "error: vulInitializeLogicalDevice: ");
        fprintf(stderr, "vkCreateDevice failed\n");
        free(queueCreateInfos);
        return 1;
    }
    vkGetDeviceQueue(
        vul->device, indices.graphicsFamily, 0, &(vul->graphicsQueue));
    vkGetDeviceQueue(
        vul->device, indices.presentFamily, 0, &(vul->presentQueue));
    free(queueCreateInfos);
    return 0;
}

/* Releases the resources backing the logical device. */
void vulFinalizeLogicalDevice(vulVulkan *vul) {
    vkDestroyDevice(vul->device, NULL);
}

/* Command buffers are allocated from command pools. So we must allocate command 
pools pretty early in the process. This initializer returns an error code (0 on 
success). On success, don't forget to vulFinalizeCommandPool when you're done. */
int vulInitializeCommandPool(vulVulkan *vul) {
    vulQueueFamilyIndices qfIndices = vulGetQueueFamilies(
        vul, vul->physicalDevice);
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = qfIndices.graphicsFamily;
    poolInfo.flags = 0;
    if (vkCreateCommandPool(
            vul->device, &poolInfo, NULL, &(vul->commandPool)) != VK_SUCCESS) {
        fprintf(stderr, "error: vulInitializeCommandPool: ");
        fprintf(stderr, "vkCreateCommandPool failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the command pool. */
void vulFinalizeCommandPool(vulVulkan *vul) {
    vkDestroyCommandPool(vul->device, vul->commandPool, NULL);
}

/* Combines all of the above initializers into one big function. So it 
initializes the instance, physical device, logical device, and command pool. 
Returns an error code (0 on success). On success, don't forget to vulFinalize 
when you're done. */
int vulInitialize(vulVulkan *vul) {
    if (vulInitializeInstance(vul) != 0)
        return 5;
    if (vulInitializeSurface(vul) != 0) {
        vulFinalizeInstance(vul);
        return 4;
    }
    if (vulSetPhysicalDevice(vul) != 0) {
        vulFinalizeSurface(vul);
        vulFinalizeInstance(vul);
        return 3;
    }
    if (vulInitializeLogicalDevice(vul) != 0) {
        vulFinalizeSurface(vul);
        vulFinalizeInstance(vul);
        return 2;
    }
    if (vulInitializeCommandPool(vul) != 0) {
        vulFinalizeLogicalDevice(vul);
        vulFinalizeSurface(vul);
        vulFinalizeInstance(vul);
        return 1;
    }
    return 0;
}

/* Releases the resources backing the instance, etc. */
void vulFinalize(vulVulkan *vul) {
    vulFinalizeCommandPool(vul);
    vulFinalizeLogicalDevice(vul);
    vulFinalizeSurface(vul);
    vulFinalizeInstance(vul);
}


