


/* This file assumes that the global variable vul has already been configured. 
*/

/* A shader program consists of a vertex shader paired with a fragment shader. 
(Vulkan supports other kinds of shaders, but we ignore them.) The shaders are 
written in OpenGL Shading Language (GLSL). They are compiled to a byte code 
representation called SPIR-V. The compiled SPIR-V is what we load here to make a 
shader program. */
typedef struct shaProgram shaProgram;
struct shaProgram {
    char *vertSPIRV, *fragSPIRV;
    int vertLength, fragLength;
    VkShaderModule vertShaderModule, fragShaderModule;
    VkPipelineShaderStageCreateInfo shaderStages[2];
};

/* Loads a file of already compiled SPIR-V byte code into CPU memory. Returns an 
error code (0 on success). On success, don't forget to call shaFinalizeSPIRV 
when you're done. */
int shaInitializeSPIRVFromFile(char **byteCode, int *length, const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "error: shaInitializeSPIRVFromFile: fopen failed\n");
        return 3;
    }
    /* C standard does not require SEEK_END to work, so this is optimistic. */
    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    *byteCode = malloc(*length * sizeof(char));
    if (*byteCode == NULL) {
        fprintf(stderr, "error: shaInitializeSPIRVFromFile: malloc failed\n");
        fclose(file);
        return 2;
    }
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(*byteCode, sizeof(char), *length, file);
    if (bytesRead != *length) {
        fprintf(stderr, "error: shaInitializeSPIRVFromFile: expected to ");
        fprintf(
            stderr, "read %d bytes but instead read %lu bytes\n", *length, 
            bytesRead);
        free(*byteCode);
        fclose(file);
        return 1;
    }
    /* Future work: Check EOF return code signalling error? */
    fclose(file);
    return 0;
}

/* Frees the memory holding the SPIR-V byte code. */
void shaFinalizeSPIRV(char **byteCode) {
    free(*byteCode);
    *byteCode = NULL;
}

/* Builds a shader module for either a vertex shader or a fragment shader. 
Returns an error code (0 on success). On success, don't forget to call 
shaFinalizeModule when you're done. */
int shaInitializeModule(VkShaderModule *module, char *byteCode, int length) {
    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = length;
    createInfo.pCode = (uint32_t *)byteCode;
    if (vkCreateShaderModule(
            vul.device, &createInfo, NULL, module) != VK_SUCCESS) {
        fprintf(stderr, "error: shaInitializeModule: ");
        fprintf(stderr, "vkCreateShaderModule failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the shader module. */
void shaFinalizeModule(VkShaderModule *module) {
    vkDestroyShaderModule(vul.device, *module, NULL);
}

/* Builds a shader program from pre-compiled vertex and fragment shader files. 
Returns an error code (0 on success). On success, don't forget to call 
shaFinalize when you're done. */
int shaInitialize(
        shaProgram *shaProg, const char* vertSPIRVFileName, 
        const char* fragSPIRVFileName) {
    if (shaInitializeSPIRVFromFile(
            &(shaProg->vertSPIRV), &(shaProg->vertLength), vertSPIRVFileName)) {
        return 6;
    }
    if (shaInitializeSPIRVFromFile(
            &(shaProg->fragSPIRV), &(shaProg->fragLength), fragSPIRVFileName)) {
        shaFinalizeSPIRV(&(shaProg->vertSPIRV));
        return 5;
    }
    int error = shaInitializeModule(
        &(shaProg->vertShaderModule), shaProg->vertSPIRV, shaProg->vertLength);
    if (error) {
        shaFinalizeSPIRV(&(shaProg->fragSPIRV));
        shaFinalizeSPIRV(&(shaProg->vertSPIRV));
        return 4;
    }
    error = shaInitializeModule(
        &(shaProg->fragShaderModule), shaProg->fragSPIRV, shaProg->fragLength);
    if (error) {
        shaFinalizeModule(&(shaProg->vertShaderModule));
        shaFinalizeSPIRV(&(shaProg->fragSPIRV));
        shaFinalizeSPIRV(&(shaProg->vertSPIRV));
        return 3;
    }
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
    vertShaderStageInfo.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaProg->vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
    fragShaderStageInfo.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaProg->fragShaderModule;
    fragShaderStageInfo.pName = "main";
    shaProg->shaderStages[0] = vertShaderStageInfo;
    shaProg->shaderStages[1] = fragShaderStageInfo;
    return 0;
}

/* Releases the resources backing the shader program. */
void shaFinalize(shaProgram *shaProg) {
    shaFinalizeModule(&(shaProg->fragShaderModule));
    shaFinalizeModule(&(shaProg->vertShaderModule));
    shaFinalizeSPIRV(&(shaProg->fragSPIRV));
    shaFinalizeSPIRV(&(shaProg->vertSPIRV));
}


