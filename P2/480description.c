


/* The descriptor machinery specifies how data is passed from the CPU-side 
program into uniform variables in the shaders. */

/* This file assumes that the global variables vul and swap have already been 
configured. */



/*** PUBLIC *******************************************************************/

/* Feel free to read from this struct's members, but write to them only through 
their accessor functions. */
typedef struct descDescription descDescription;
struct descDescription {
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    /* This dynamically allocated array has length swap.numImages. */
    VkDescriptorSet *descriptorSets;
};



/*** PRIVATE ******************************************************************/

/* Initializes the descriptor set layout. Returns an error code (0 on success). 
On success, don't forget to descFinalizeLayout when you're done. */
int descInitializeLayout(
        VkDescriptorSetLayout *descriptorSetLayout, int descrNum, 
        int descrCounts[], VkDescriptorType descrTypes[], 
        VkShaderStageFlags descrStageFlagss[], int descrBindings[]) {
    /* Temporarily allocate memory to hold the layout bindings. */
    VkDescriptorSetLayoutBinding *layoutBindings = 
        malloc(descrNum * sizeof(VkDescriptorSetLayoutBinding));
    if (layoutBindings == NULL) {
        fprintf(stderr, "error: descInitializeLayout: malloc failed\n");
        return 2;
    }
    /* Configure each layout binding. */
    for (int i = 0; i < descrNum; i += 1) {
        VkDescriptorSetLayoutBinding binding = {0};
        binding.descriptorType = descrTypes[i];
        binding.binding = descrBindings[i];
        binding.descriptorCount = descrCounts[i];
        binding.stageFlags = descrStageFlagss[i];
        layoutBindings[i] = binding;
    }
    /* Collect all of the layout bindings together. */
    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = descrNum;
    layoutInfo.pBindings = layoutBindings;
    if (vkCreateDescriptorSetLayout(
            vul.device, &layoutInfo, NULL, descriptorSetLayout) != VK_SUCCESS) {
        fprintf(stderr, "error: descInitializeLayout: ");
        fprintf(stderr, "vkCreateDescriptorSetLayout failed\n");
        free(layoutBindings);
        return 1;
    }
    free(layoutBindings);
    return 0;
}

/* Releases the resources backing the descriptor set layout. */
void descFinalizeLayout(VkDescriptorSetLayout *descriptorSetLayout) {
    vkDestroyDescriptorSetLayout(vul.device, *descriptorSetLayout, NULL);
}

/* Returns an error code (0 on success). On success, don't forget to 
descFinalizePool when you're done. */
int descInitializePool(
        VkDescriptorPool *descPool, int descrNum, int descrCounts[], 
        VkDescriptorType descrTypes[]) {
    /* Temporarily allocate memory to hold the pool sizes. */
    VkDescriptorPoolSize *poolSizes = 
        malloc(descrNum * sizeof(VkDescriptorPoolSize));
    if (poolSizes == NULL) {
        fprintf(stderr, "error: descInitializePool: malloc failed\n");
        return 2;
    }
    /* Configure each pool size. */
    for (int i = 0; i < descrNum; i += 1) {
        VkDescriptorPoolSize poolSize = {0};
        poolSize.type = descrTypes[i];
        poolSize.descriptorCount = (uint32_t)(swap.numImages * descrCounts[i]);
        poolSizes[i] = poolSize;
    }
    /* Collect all of the pool sizes together. */
    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = descrNum;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = (uint32_t)swap.numImages;
    if (vkCreateDescriptorPool(
            vul.device, &poolInfo, NULL, descPool) != VK_SUCCESS) {
        fprintf(stderr, "error: descInitializePool: ");
        fprintf(stderr, "vkCreateDescriptorPool failed\n");
        free(poolSizes);
        return 1;
    }
    free(poolSizes);
    return 0;
}

/* Releases the resources backing the descriptor pool. */
void descFinalizePool(VkDescriptorPool *descPool) {
    vkDestroyDescriptorPool(vul.device, *descPool, NULL);
}

/* Returns an error code (0 on success). On success, don't forget to 
descFinalizeSets when you're done. The setDescriptorSet argument has to 
configure the descriptor sets with specific actual information. The i argument 
specifies which element of the swap chain we're targeting. */
int descInitializeSets(
        descDescription *desc, 
        void (*setDescriptorSet)(descDescription *desc, int i)) {
    /* Temporarily make multiple copies of the descriptor set layout. */
    VkDescriptorSetLayout *layouts;
    layouts = malloc(swap.numImages * sizeof(VkDescriptorSetLayout));
    if (layouts == NULL) {
        fprintf(stderr, "error: descInitializeSets: malloc failed\n");
        return 3;
    }
    for (int i = 0; i < swap.numImages; i += 1)
        layouts[i] = desc->descriptorSetLayout;
    /* Get ready to allocate multiple descriptor sets from the pool. */
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = desc->descriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)swap.numImages;
    allocInfo.pSetLayouts = layouts;
    desc->descriptorSets = malloc(swap.numImages * sizeof(VkDescriptorSet));
    if (desc->descriptorSets == NULL) {
        fprintf(stderr, "error: descInitializeSets: malloc failed\n");
        free(layouts);
        return 2;
    }
    if (vkAllocateDescriptorSets(
            vul.device, &allocInfo, desc->descriptorSets) != VK_SUCCESS) {
        fprintf(stderr, "error: descInitializeSets: ");
        fprintf(stderr, "vkAllocateDescriptorSets failed\n");
        free(desc->descriptorSets);
        free(layouts);
        return 1;
    }
    /* Configure each descriptor set just allocated. */
    for (size_t i = 0; i < swap.numImages; i += 1)
        setDescriptorSet(desc, i);
    free(layouts);
    return 0;
}

/* Releases the resources backing the descriptor sets. */
void descFinalizeSets(descDescription *desc) {
    /* The descriptor sets created with vkAllocateDescriptorSets are 
    automatically deallocated when the descriptor pool is destroyed. So we must 
    just free the memory on the CPU side. */
    free(desc->descriptorSets);
}



/*** PUBLIC *******************************************************************/

/* Initializes the description of how the uniforms connect to the shaders. 
Returns an error code (0 on success). On success, don't forget to descFinalize 
when you're done. */
int descInitialize(
        descDescription *desc, int num, int counts[], VkDescriptorType types[], 
        VkShaderStageFlags stageFlagss[], int bindings[], 
        void (*setDescriptorSet)(descDescription *desc, int i)) {
    if (descInitializeLayout(
            &(desc->descriptorSetLayout), num, counts, types, stageFlagss, 
            bindings) != 0)
        return 3;
    if (descInitializePool(&(desc->descriptorPool), num, counts, types) != 0) {
        descFinalizeLayout(&(desc->descriptorSetLayout));
        return 2;
    }
    if (descInitializeSets(desc, setDescriptorSet) != 0) {
        descFinalizePool(&(desc->descriptorPool));
        descFinalizeLayout(&(desc->descriptorSetLayout));
        return 1;
    }
    return 0;
}

/* Releases the resources backing the description. */
void descFinalize(descDescription *desc) {
    descFinalizeSets(desc);
    descFinalizePool(&(desc->descriptorPool));
    descFinalizeLayout(&(desc->descriptorSetLayout));
}


