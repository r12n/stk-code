#stk_include "utils/culling_info.vert"

layout(points, invocations = 3) in;
layout(triangle_strip, max_vertices = 4) out;

flat in int cmd_id[];
flat in int instance_pos[];
flat in uint instance_type[];

flat out int f_instance_pos;

void main(void)
{
    f_instance_pos = instance_pos[0];
    if (instance_type[0] == 1)
    {
        gl_Position = vec4(-0.01, -0.01, 0.0, 1.0);
        EmitVertex();
        gl_Position = vec4(-0.01, 0.0, 0.0, 1.0);
        EmitVertex();
        gl_Position = vec4(0.01, -0.01, 0.0, 1.0);
        EmitVertex();
        gl_Position = vec4(0.01, 0.0, 0.0, 1.0);
        EmitVertex();
        EndPrimitive();
        return;
    }

    vec3 face_normal = vec3(0.0);
    vec3 edge_basis_0 = vec3(0.0);
    vec3 edge_basis_1 = vec3(0.0);
    vec4 bb_min = instances[cmd_id[0]].m_min;
    vec4 bb_max = vec4(instances[cmd_id[0]].m_max.xyz, 1.0);
    vec3 bb_center = ((bb_min + bb_max) * 0.5).xyz;
    vec3 bb_dim = ((bb_max - bb_min) * 0.5).xyz;

    if (gl_InvocationID == 0)
    {
        face_normal.x = bb_dim.x;
        edge_basis_0.y = bb_dim.y;
        edge_basis_1.z = bb_dim.z;
    }
    else if (gl_InvocationID == 1)
    {
        face_normal.y = bb_dim.y;
        edge_basis_1.x = bb_dim.x;
        edge_basis_0.z = bb_dim.z;
    }
    else if (gl_InvocationID == 2)
    {
        face_normal.z = bb_dim.z;
        edge_basis_0.x = bb_dim.x;
        edge_basis_1.y = bb_dim.y;
    }
    mat4 model_matrix = instance_objects[instance_pos[0]].m_model_matrix;
    vec3 world_center = (model_matrix * vec4(bb_center, 1.0)).xyz;
    vec3 world_normal = mat3(model_matrix) * face_normal;
    vec3 world_pos = world_center + world_normal;
    mat4 tim = transpose(inverse(ViewMatrix));
    vec3 view_pos = vec3(tim[0][3], tim[1][3], tim[2][3]);
    float proj = sign(dot(world_pos - view_pos, world_normal)) * -1.0;
    face_normal = mat3(model_matrix) * face_normal * proj;
    edge_basis_0 = mat3(model_matrix) * edge_basis_0;
    edge_basis_1 = mat3(model_matrix) * edge_basis_1 * proj;

    gl_Position = ProjectionViewMatrix * vec4(world_center +
        (face_normal - edge_basis_0 - edge_basis_1), 1.0);
    EmitVertex();
    gl_Position = ProjectionViewMatrix * vec4(world_center +
        (face_normal + edge_basis_0 - edge_basis_1), 1.0);
    EmitVertex();
    gl_Position = ProjectionViewMatrix * vec4(world_center +
        (face_normal - edge_basis_0 + edge_basis_1), 1.0);
    EmitVertex();
    gl_Position = ProjectionViewMatrix * vec4(world_center +
        (face_normal + edge_basis_0 + edge_basis_1), 1.0);
    EmitVertex();
    EndPrimitive();
}   // main
