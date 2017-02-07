uniform vec3 windDir;

#ifdef GPU_CULLING

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;
layout(location = 7) in uint draw_id;
#ifdef Use_Bindless_Texture
layout(location = 8) in sampler2D Handle;
layout(location = 9) in sampler2D SecondHandle;
layout(location = 10) in sampler2D ThirdHandle;
#endif

#stk_include "utils/culling_info.vert"

#else

#ifdef Explicit_Attrib_Location_Usable
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;

layout(location = 7) in vec3 Origin;
layout(location = 8) in vec3 Orientation;
layout(location = 9) in vec3 Scale;
layout(location = 10) in vec4 misc_data;
#ifdef Use_Bindless_Texture
layout(location = 11) in sampler2D Handle;
layout(location = 12) in sampler2D SecondHandle;
layout(location = 13) in sampler2D ThirdHandle;
#endif

#else
in vec3 Position;
in vec3 Normal;
in vec4 Color;
in vec2 Texcoord;

in vec3 Origin;
in vec3 Orientation;
in vec3 Scale;
in vec4 misc_data;
#endif

#stk_include "utils/getworldmatrix.vert"

#endif

out vec3 nor;
out vec2 uv;
flat out vec2 color_change;
#ifdef Use_Bindless_Texture
flat out sampler2D handle;
flat out sampler2D secondhandle;
flat out sampler2D thirdhandle;
#endif

void main()
{
#ifdef GPU_CULLING

    uint index = instance_objects[draw_id + gl_InstanceID].m_other_data.w;
    mat4 ModelMatrix = instance_objects[draw_id + index].m_model_matrix;
    mat4 inverse_model_matrix = inverse(ModelMatrix);
    ModelMatrix[3].xyz += windDir * Color.r;
    inverse_model_matrix[3].xyz -= windDir * Color.r;
    mat4 TransposeInverseModelView = transpose(inverse_model_matrix * InverseViewMatrix);
    vec4 misc_data = instance_objects[draw_id + index].m_misc_data;

#else

    mat4 ModelMatrix = getWorldMatrix(Origin + windDir * Color.r, Orientation, Scale);
    mat4 TransposeInverseModelView = transpose(getInverseWorldMatrix(Origin + windDir * Color.r, Orientation, Scale) * InverseViewMatrix);

#endif

    gl_Position = ProjectionViewMatrix *  ModelMatrix * vec4(Position, 1.);
    nor = (TransposeInverseModelView * vec4(Normal, 0.)).xyz;
    uv = Texcoord;
    color_change = misc_data.zw;
#ifdef Use_Bindless_Texture
    handle = Handle;
    secondhandle = SecondHandle;
    thirdhandle = ThirdHandle;
#endif
}
