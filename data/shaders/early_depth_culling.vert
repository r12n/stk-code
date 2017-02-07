layout(location = 0) in int i_cmd_id;

#stk_include "utils/culling_info.vert"

flat out int cmd_id;
flat out int instance_pos;
flat out uint instance_type;

void main(void)
{
    instance_objects[gl_VertexID].m_other_data.w = 0;
    cmd_id = i_cmd_id;
    instance_pos = gl_VertexID;
    instance_type = instance_objects[gl_VertexID].m_other_data.y;
}   // main
