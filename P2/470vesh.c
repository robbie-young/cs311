/*
    470vesh.c
    Creates the veshVesh struct as a way to store mesh information on the GPU using Vulkan.
    Designed by Josh Davis for Carleton College's CS311 - Computer Graphics.
    Implementations written by Cole Weinstein and Robbie Young.
*/


/* 'Vesh' means 'Vulkan mesh'. As soon as it's created, we load it into GPU 
memory, for fast rendering. */



/*** PRIVATE ******************************************************************/

/* Helper function for veshInitializeStyle. Encodes the ith attribute dimension in a 
way that's useful to Vulkan. */
VkVertexInputAttributeDescription veshGetAttributeDescription(
        int i, int numAttr, const int attrDims[]) {
    VkVertexInputAttributeDescription attrDesc = {0};
    attrDesc.binding = 0;
    attrDesc.location = i;
    if (attrDims[i] == 1)
        attrDesc.format = VK_FORMAT_R32_SFLOAT;
    else if (attrDims[i] == 2)
        attrDesc.format = VK_FORMAT_R32G32_SFLOAT;
    else if (attrDims[i] == 3)
        attrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    else
        attrDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrDesc.offset = 0;
    for (int j = 0; j < i; j += 1)
        attrDesc.offset += attrDims[j] * sizeof(float);
    return attrDesc;
}

/* A vertex buffer holds vertex attribute information for a vesh. This function 
loads the CPU-side verts data into a GPU-side vertex buffer. It returns an error 
code (0 on success). On success, remember to veshFinalizeVertexBuffer when 
you're done. */
int veshInitializeVertexBuffer(
        VkBuffer *vertBuf, VkDeviceMemory *vertBufMem, int totalAttrDim, 
        int numVerts, const float verts[]) {
    VkDeviceSize bufSize = numVerts * totalAttrDim * sizeof(float);
    /* Create a CPU-accessible staging buffer. */
    VkBuffer stagingBuf;
    VkDeviceMemory stagingBufMem;
    if (bufInitialize(
            bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuf, 
            &stagingBufMem) != 0)
        return 2;
    /* Copy data into the staging buffer. */
    void *data;
    vkMapMemory(vul.device, stagingBufMem, 0, bufSize, 0, &data);
    memcpy(data, verts, (size_t)bufSize);
    vkUnmapMemory(vul.device, stagingBufMem);
    /* Create a GPU buffer, from which to actually render. */
    if (bufInitialize(
            bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertBuf, vertBufMem) != 0) {
        bufFinalize(&stagingBuf, &stagingBufMem);
        return 1;
    }
    bufCopy(stagingBuf, *vertBuf, bufSize);
    bufFinalize(&stagingBuf, &stagingBufMem);
    return 0;
}

/* Releases the resources backing the vertex buffer. */
void veshFinalizeVertexBuffer(VkBuffer *vertBuf, VkDeviceMemory *vertBufMem) {
    bufFinalize(vertBuf, vertBufMem);
}

/* An index buffer holds the triangles of the vesh. That is, it holds the 
triples of indices into the vesh's vertex buffer. This function loads the CPU-
side tris data into a GPU-side index buffer. It returns an error code (0 on 
success). On success, remember to veshFinalizeIndexBuffer when you're done. */
int veshInitializeIndexBuffer(
        VkBuffer *indBuf, VkDeviceMemory *indBufMem, int numTris, 
        const uint16_t tris[]) {
    /* Compute the buffer size. */
    VkDeviceSize bufSize = numTris * 3 * sizeof(uint16_t);
    /* Create a CPU-accessible staging buffer. */
    VkBuffer stagBuf;
    VkDeviceMemory stagBufMem;
    if (bufInitialize(
            bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagBuf, &stagBufMem) != 0)
        return 2;
    /* Copy data into the staging buffer. */
    void *data;
    vkMapMemory(vul.device, stagBufMem, 0, bufSize, 0, &data);
    memcpy(data, tris, (size_t)bufSize);
    vkUnmapMemory(vul.device, stagBufMem);
    /* Create a GPU buffer, from which to actually render. */
    if (bufInitialize(
            bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indBuf, indBufMem) != 0) {
        bufFinalize(&stagBuf, &stagBufMem);
        return 1;
    }
    bufCopy(stagBuf, *indBuf, bufSize);
    bufFinalize(&stagBuf, &stagBufMem);
    return 0;
}

/* Releases the resources backing the index buffer. */
void veshFinalizeIndexBuffer(VkBuffer *indBuf, VkDeviceMemory *indBufMem) {
    bufFinalize(indBuf, indBufMem);
}



/*** PUBLIC *******************************************************************/

/* Feel free to read from this struct's members, but don't write to them except 
through their accessors. */
typedef struct veshStyle veshStyle;
struct veshStyle {
    VkVertexInputBindingDescription bindingDesc;
    VkVertexInputAttributeDescription *attrDescs;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
};

/* Initializes data structures that record the structure of the mesh attributes 
to be used. Returns an error code (0 on success). On success, don't forget to 
veshFinalizeStyle when you're done. */
int veshInitializeStyle(veshStyle *style, int numAttr, const int attrDims[]) {
    style->attrDescs = malloc(
        numAttr * sizeof(VkVertexInputAttributeDescription));
    if (style->attrDescs == NULL) {
        fprintf(stderr, "error: veshInitializeStyle: malloc failed\n");
        return 1;
    }
    /* Compute the total attribute dimension. */
    int attrDim = 0;
    for (int i = 0; i < numAttr; i += 1)
        attrDim += attrDims[i];
    /* Set the binding description. */
    VkVertexInputBindingDescription bindDesc = {0};
    bindDesc.binding = 0;
    bindDesc.stride = attrDim * sizeof(float);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    style->bindingDesc = bindDesc;
    /* Get the attribute descriptions. */
    for (int i = 0; i < numAttr; i += 1)
        style->attrDescs[i] = veshGetAttributeDescription(i, numAttr, attrDims);
    /* Get the vertex input info. */
    VkPipelineVertexInputStateCreateInfo vertInfo = {0};
    vertInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInfo.vertexBindingDescriptionCount = 1;
    vertInfo.pVertexBindingDescriptions = &(style->bindingDesc);
    vertInfo.vertexAttributeDescriptionCount = numAttr;
    vertInfo.pVertexAttributeDescriptions = style->attrDescs;
    style->vertexInputInfo = vertInfo;
    /* Get the input assembly. */
    VkPipelineInputAssemblyStateCreateInfo assembly = {0};
    assembly.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;
    style->inputAssembly = assembly;
    return 0;
}

/* Releases the resources underlying the style. */
void veshFinalizeStyle(veshStyle *style) {
    free(style->attrDescs);
}

/* Feel free to read from this struct's members, but don't write to them except 
through their accessors. */
typedef struct veshVesh veshVesh;
struct veshVesh {
    int triNum, vertNum, attrDim;
	VkBuffer vertBuf, triBuf;
    VkDeviceMemory vertBufMem, triBufMem;
};

/* Initializes the vesh from a CPU-side mesh. Returns an error code (0 on 
success). On success, don't forget to veshFinalize when you're done. After the 
vesh is initialized, the mesh can be finalized; the vesh doesn't need the mesh 
to be kept around long-term. */
int veshInitializeMesh(veshVesh *vesh, meshMesh *mesh) {
    if (veshInitializeIndexBuffer(&vesh->triBuf, &vesh->triBufMem, mesh->triNum, mesh->tri) != 0) {
        return 2;
    }
    if (veshInitializeVertexBuffer(&vesh->vertBuf, &vesh->vertBufMem, mesh->attrDim, mesh->vertNum, mesh->vert) != 0) {
        veshFinalizeIndexBuffer(&vesh->triBuf, &vesh->triBufMem);
        return 1;
    }

    vesh->triNum = mesh->triNum;
    vesh->vertNum = mesh->vertNum;
    vesh->attrDim = mesh->attrDim;
    
    return 0;
}

/* Releases the resources backing the vesh. */
void veshFinalize(veshVesh *vesh) {
    veshFinalizeVertexBuffer(&vesh->vertBuf, &vesh->vertBufMem);
    veshFinalizeIndexBuffer(&vesh->triBuf, &vesh->triBufMem);
}

/* Renders the vesh by sending commands to the given command buffer. */
void veshRender(const veshVesh *vesh, VkCommandBuffer cmdBuf) {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {vesh->vertBuf};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vesh->triBuf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmdBuf, (uint32_t)(vesh->triNum * 3), 1, 0, 0, 0);
}


