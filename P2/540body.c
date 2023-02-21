


/* Feel free to read and write this struct's members. */
typedef struct bodyBody bodyBody;
struct bodyBody {
    isoIsometry isometry;
    BodyUniforms uniforms;
    veshVesh *vesh;
};

/* Like an initializer, this method sets the body into an acceptable initial 
state. Unlike an initializer, this method has no matching finalizer. */
void bodyConfigure(bodyBody *body, veshVesh *vesh) {
    float rotation[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    isoSetRotation(&(body->isometry), rotation);
    float translation[3] = {0.0, 0.0, 0.0};
    isoSetTranslation(&(body->isometry), translation);
    BodyUniforms unifs = {0};
    body->uniforms = unifs;
    body->vesh = vesh;
}

/* Called during command buffer construction. Renders the body's vesh. The index 
must count 0, 1, 2, ... as body after body is rendered into the scene. */
void bodyRender(
        const bodyBody *body, VkCommandBuffer *cmdBuf, 
        const VkPipelineLayout *pipelineLayout, 
        const VkDescriptorSet *descriptorSet, const unifAligned *aligned, 
        int index) {
    uint32_t offset = index * aligned->alignedSize;
    vkCmdBindDescriptorSets(
        *cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0, 1, 
        descriptorSet, 1, &offset);
    veshRender(body->vesh, *cmdBuf);
}

/* Called once per animation frame. The index must match the one passed to 
bodyRender. This method loads the body's isometry into the body's uniforms and 
then copies the body's uniforms into the indexth UBO array element. */
void bodySetUniforms(bodyBody *body, const unifAligned *aligned, int index) {
    float proper[4][4];
    isoGetHomogeneous(&(body->isometry), proper);
    mat44Transpose(proper, body->uniforms.modelingT);
    BodyUniforms *bodyUnif = (BodyUniforms *)unifGetAligned(aligned, index);
    *bodyUnif = body->uniforms;
}


