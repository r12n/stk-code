#stk_include "utils/culling_info.vert"

void main(void)
{
    int cmd_index = gl_VertexID;
    cmds[cmd_index].m_index_count = 0;
    cmds[cmd_index].m_instance_count = 0;
    cmds[cmd_index].m_first_index = 0;
    cmds[cmd_index].m_base_vertex = 0;
    cmds[cmd_index].m_base_instance = 0;
    uint cur_instance_id = 0;
    uint offset = instances[cmd_index].m_draw_cmd.x;
    uint instance_count = floatBitsToUint(instances[cmd_index].m_max.w);
    for (uint i = 0; i < instance_count; i++)
    {
        if (instance_objects[offset + i].m_other_data.w == 0)
            continue;
        cmds[cmd_index].m_index_count = instances[cmd_index].m_draw_cmd.y;
        atomicAdd(cmds[cmd_index].m_instance_count, 1);
        cmds[cmd_index].m_first_index = instances[cmd_index].m_draw_cmd.z;
        cmds[cmd_index].m_base_vertex = instances[cmd_index].m_draw_cmd.w;
        cmds[cmd_index].m_base_instance = cmd_index;
        instance_objects[offset + cur_instance_id++].m_other_data.w = i;
    }
}   // main
