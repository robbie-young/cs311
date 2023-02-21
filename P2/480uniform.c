


/* This file assumes that the global variables vul and swap have already been 
configured. This file is not written in an object-oriented style. I mean, the 
functions do not systematically act on any specific data type as their first 
argument. */



/*** GPU-SIDE UNIFORM BUFFERS (SINGLE OR ARRAY) *******************************/

/* Initializes one uniform buffer per swap chain element. Returns an error code 
(0 on success). On success, don't forget to unifFinalizeBuffers when you're 
done. */
int unifInitializeBuffers(
        VkBuffer **unifBufs, VkDeviceMemory **unifBufsMem, 
        VkDeviceSize bufferSize) {
    /* Allocate CPU-side memory to hold meta-data. */
    *unifBufs = malloc(swap.numImages * sizeof(VkBuffer));
    if (*unifBufs == NULL) {
        fprintf(stderr, "error: unifInitializeBuffers: malloc failed\n");
        return 3;
    }
    *unifBufsMem = malloc(swap.numImages * sizeof(VkDeviceMemory));
    if (*unifBufsMem == NULL) {
        fprintf(stderr, "error: unifInitializeBuffers: malloc failed\n");
        free(*unifBufs);
        return 2;
    }
    /* Allocate GPU-side memory to actually hold the buffers. */
    for (size_t i = 0; i < swap.numImages; i += 1) {
        int error = bufInitialize(
            bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &(*unifBufs)[i], 
            &(*unifBufsMem)[i]);
        if (error != 0) {
            for (size_t j = 0; j < i; j += 1)
                bufFinalize(&(*unifBufs)[j], &(*unifBufsMem)[j]);
            free(*unifBufsMem);
            free(*unifBufs);
            return 1;
        }
    }
    return 0;
}

/* Releases the resources backing the uniform buffers. */
void unifFinalizeBuffers(VkBuffer **unifBufs, VkDeviceMemory **unifBufsMem) {
    for (size_t i = 0; i < swap.numImages; i += 1)
        bufFinalize(&(*unifBufs)[i], &(*unifBufsMem)[i]);
    free(*unifBufsMem);
    free(*unifBufs);
};



/*** CPU-SIDE ARRAYS OF UNIFORM BUFFERS ***************************************/

/* For uniform buffers with dynamic offset (which we use for body-specific 
uniforms), we can't just pack elements of size uboSize into an array, because 
the GPU has certain alignment requirements. Each array element has to start at 
an offset that is a multiple of the GPU's minimum alignment. So this function 
returns the least multiple of the alignment that is greater than or equal to 
uboSize. */
int unifAlignment(int uboSize) {
    int alignment = 
        vul.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    if (uboSize % alignment == 0)
        return uboSize;
    else
        return (uboSize / alignment + 1) * alignment;
}

/* Feel free to read from this data structure's members, but don't write to them 
except through the accessors. */
typedef struct unifAligned unifAligned;
struct unifAligned {
    int uboNum, uboSize, alignedSize;
    char *data;
};

/* Initializes a CPU-side buffer big enough to hold an array of uboNum UBOs that 
conform to the GPU's minimum alignment. Returns an error code (0 on success). On 
success, don't forget to unifFinalizeAligned when you're done. */
int unifInitializeAligned(unifAligned *aligned, int uboNum, int uboSize) {
    aligned->uboNum = uboNum;
    aligned->uboSize = uboSize;
    aligned->alignedSize = unifAlignment(uboSize);
    aligned->data = malloc(uboNum * aligned->alignedSize);
    /* To align the memory allocation on the CPU side, we could use the POSIX 
    function aligned_alloc instead of malloc. But this seems unnecessary.
    aligned->data = aligned_alloc(
        aligned->alignedSize, uboNum * aligned->alignedSize);*/
    if (aligned->data == NULL) {
        fprintf(stderr, "error: unifInitializeAligned: malloc failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the CPU-side UBO array. */
void unifFinalizeAligned(unifAligned *aligned) {
    free(aligned->data);
}

/* Returns a pointer to the ith UBO. If the UBO's type is BodyUniforms (for 
example), then you can cast this pointer to a pointer of that type using 
    BodyUniforms *bodyUnif = (BodyUniforms *)unifGetAligned(...);
Then you can read from and write to bodyUnif. */
void *unifGetAligned(const unifAligned *aligned, int i) {
    return &(aligned->data[i * aligned->alignedSize]);
}


