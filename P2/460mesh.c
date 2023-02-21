


/* A mesh essentially consists of a list of vertices and a list of triangles. 
Each triangle is three integer indices specifying which vertices make up the 
triangle. That's the idea. But Vulkan requires various buffers and other data 
structures to hold the mesh and its meta data. */

/* This file assumes that the global variable vul has already been configured. 
This file is not written in an object-oriented style. I mean, the functions do 
not systematically act on a mesh datatype as their first argument. */

/* Helper function for meshGetStyle. Encodes the ith attribute dimension in a 
way that's useful to Vulkan. */
VkVertexInputAttributeDescription meshGetAttributeDescription(
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

/* The input parameters are numAttr and attrDims. All other parameters are 
output. So basically this function encodes the number of attributes and the 
dimensions of the attributes into data structures that Vulkan likes. Along the 
way it encodes some of our other assumptions, such as the mesh's being a simple 
triangle list. */
void meshGetStyle(
        int numAttr, const int attrDims[], int *totalDim, 
        VkVertexInputBindingDescription *bindingDesc, 
        VkVertexInputAttributeDescription attrDescs[], 
        VkPipelineVertexInputStateCreateInfo *vertexInputInfo, 
        VkPipelineInputAssemblyStateCreateInfo *inputAssembly) {
    /* Compute the total attribute dimension. */
    *totalDim = 0;
    for (int i = 0; i < numAttr; i += 1)
        *totalDim += attrDims[i];
    /* Get the binding description. */
    VkVertexInputBindingDescription bindDesc = {0};
    bindDesc.binding = 0;
    bindDesc.stride = *totalDim * sizeof(float);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    *bindingDesc = bindDesc;
    /* Get the attribute descriptions. */
    for (int i = 0; i < numAttr; i += 1)
        attrDescs[i] = meshGetAttributeDescription(i, numAttr, attrDims);
    /* Get the vertex input info. */
    VkPipelineVertexInputStateCreateInfo vertInfo = {0};
    vertInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInfo.vertexBindingDescriptionCount = 1;
    vertInfo.pVertexBindingDescriptions = bindingDesc;
    vertInfo.vertexAttributeDescriptionCount = numAttr;
    vertInfo.pVertexAttributeDescriptions = attrDescs;
    *vertexInputInfo = vertInfo;
    /* Get the input assembly. */
    VkPipelineInputAssemblyStateCreateInfo assembly = {0};
    assembly.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;
    *inputAssembly = assembly;
}

/* A vertex buffer holds vertex attribute information for a mesh. This function 
loads the CPU-side verts data into a GPU-side vertex buffer. It returns an error 
code (0 on success). On success, remember to meshFinalizeVertexBuffer when 
you're done. */
int meshInitializeVertexBuffer(
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
void meshFinalizeVertexBuffer(VkBuffer *vertBuf, VkDeviceMemory *vertBufMem) {
    bufFinalize(vertBuf, vertBufMem);
}

/* An index buffer holds the triangles of the mesh. That is, it holds the 
triples of indices into the mesh's vertex buffer. This function loads the CPU-
side tris data into a GPU-side index buffer. It returns an error code (0 on 
success). On success, remember to meshFinalizeIndexBuffer when you're done. */
int meshInitializeIndexBuffer(
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
void meshFinalizeIndexBuffer(VkBuffer *indBuf, VkDeviceMemory *indBufMem) {
    bufFinalize(indBuf, indBufMem);
}


