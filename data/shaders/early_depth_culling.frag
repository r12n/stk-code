#stk_include "utils/culling_info.vert"

layout(early_fragment_tests) in;

flat in int f_instance_pos;
out vec4 debug_color;
void main(void)
{
    instance_objects[f_instance_pos].m_other_data.w = 1;
    debug_color = vec4(0.0, 0.5, 1.0, 0.5);
}   // main
