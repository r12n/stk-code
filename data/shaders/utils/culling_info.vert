struct InstanceObject
{
    mat4 m_model_matrix;
    vec4 m_misc_data;
    // x: store the offset in instances[]
    // y: tells if this instance is an occluder
    // z: store the skinning offset for skinned mesh
    // w: store the culling result of solid pass
    uvec4 m_other_data;
    // store the culling results of 4 shadow cams
    uvec4 m_other_data_1;
};

layout(std140, binding = 0) buffer _instance_objects
{
    InstanceObject instance_objects[];
};

struct DrawElementsIndirectCommand
{
    uint m_index_count;
    uint m_instance_count;
    uint m_first_index;
    uint m_base_vertex;
    uint m_base_instance;
};

layout(std430, binding = 1) buffer _cmds
{
    DrawElementsIndirectCommand cmds[];
};

struct Instance
{
    // Min and max corner in bounding box in local coordinates
    vec4 m_min;
    // w of m_max (uint bits): store the instance count
    vec4 m_max;
    // x: offset in list of instances (m_model_matrix, m_misc_data...)
    // y: index_count in DrawElementsIndirectCommand
    // z: first_index in DrawElementsIndirectCommand
    // w: base_vertex in DrawElementsIndirectCommand
    uvec4 m_draw_cmd;
    // Padding for texture handle
    uvec4 m_texture_handle[2];
};

layout(std140, binding = 2) readonly buffer _instances
{
    Instance instances[];
};
