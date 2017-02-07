#ifdef GPU_CULLING

layout(location = 0) in vec3 Position;
layout(location = 7) in uint draw_id;

#stk_include "utils/culling_info.vert"

#else

#ifdef Explicit_Attrib_Location_Usable
layout(location = 0) in vec3 Position;
layout(location = 7) in vec3 Origin;
layout(location = 8) in vec3 Orientation;
layout(location = 9) in vec3 Scale;
layout(location = 10) in vec4 misc_data;

#else
in vec3 Position;
in vec3 Origin;
in vec3 Orientation;
in vec3 Scale;
in vec4 misc_data;
#endif

#stk_include "utils/getworldmatrix.vert"

#endif

flat out vec4 glowColor;

void main(void)
{
#ifdef GPU_CULLING

    uint index = instance_objects[draw_id + gl_InstanceID].m_other_data.w;
    mat4 ModelMatrix = instance_objects[draw_id + index].m_model_matrix;
    vec4 misc_data = instance_objects[draw_id + index].m_misc_data;
    mat4 TransposeInverseModelView = transpose(inverse(ModelMatrix) * InverseViewMatrix);

#else

    mat4 ModelMatrix = getWorldMatrix(Origin, Orientation, Scale);
    mat4 TransposeInverseModelView = transpose(getInverseWorldMatrix(Origin, Orientation, Scale) * InverseViewMatrix);

#endif

    gl_Position = ProjectionViewMatrix * ModelMatrix * vec4(Position, 1.);
    glowColor = misc_data;
}
