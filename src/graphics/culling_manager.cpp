//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2017 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#include "graphics/culling_manager.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/command_buffer.hpp"
#include "graphics/draw_tools.hpp"
#include "graphics/glwrap.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/materials.hpp"
#include "graphics/render_info.hpp"
#include "graphics/rtts.hpp"
#include "graphics/shaders.hpp"
#include "graphics/shader_files_manager.hpp"
#include "graphics/stk_animated_mesh.hpp"
#include "graphics/stk_mesh.hpp"
#include "graphics/stk_mesh_scene_node.hpp"
#include "graphics/vao_manager.hpp"
#include "utils/profiler.hpp"

#include <algorithm>
#include <iterator>
#include <IMesh.h>
#include <IMeshBuffer.h>

// ----------------------------------------------------------------------------
class TrackDepthShader : public Shader<TrackDepthShader>
{
private:
    GLuint m_id[5];
public:
    TrackDepthShader()
    {
        loadProgram(OBJECT, GL_VERTEX_SHADER, "track_depth.vert");
        glUseProgram(m_program);
        m_id[0] = glGetSubroutineIndex(m_program, GL_VERTEX_SHADER, "normal");
        m_id[1] = glGetSubroutineIndex(m_program, GL_VERTEX_SHADER, "shadow1");
        m_id[2] = glGetSubroutineIndex(m_program, GL_VERTEX_SHADER, "shadow2");
        m_id[3] = glGetSubroutineIndex(m_program, GL_VERTEX_SHADER, "shadow3");
        m_id[4] = glGetSubroutineIndex(m_program, GL_VERTEX_SHADER, "shadow4");
    }
    void setPVM(unsigned int n) const
    {
        glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &m_id[n]);
    }
};   // TrackDepthShader

// ----------------------------------------------------------------------------
class DrawCommandGenerator : public Shader<DrawCommandGenerator>
{
public:
    DrawCommandGenerator()
    {
        loadProgram(OBJECT, GL_VERTEX_SHADER, "draw_command_generator.vert");
    }   // DrawCommandGenerator
};

// ----------------------------------------------------------------------------
class EarlyDepthCulling : public Shader<EarlyDepthCulling>
{
public:
    EarlyDepthCulling()
    {
        loadProgram(OBJECT, GL_VERTEX_SHADER, "early_depth_culling.vert",
                            GL_GEOMETRY_SHADER, "early_depth_culling.geom",
                            GL_FRAGMENT_SHADER, "early_depth_culling.frag");
    }   // EarlyDepthCulling
};

// ----------------------------------------------------------------------------
CullingManager::CullingManager()
{
    m_visualization = false;
    m_bb_viz = false;
    m_bindless_texture = CVS->isARBBindlessTextureUsable();
    m_track_vao = 0;
    m_track_vbo = 0;

    cleanMeshList();
    glGenBuffers(1, &m_instances);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instances);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_instances);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, 1000 * 5 * 4 * sizeof(uint32_t),
        NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
    m_instances_ptr = (uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
        1000 * 5 * 4 * sizeof(uint32_t),
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    glGenBuffers(1, &m_draw_command);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_draw_command);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_draw_command);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, 1000 * 5 * sizeof(uint32_t),
        NULL, GL_MAP_READ_BIT);

    glGenBuffers(1, &m_instance_objects);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instance_objects);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instance_objects);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, 10000 * 28 * sizeof(uint32_t),
        NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
    m_instance_objects_ptr = (uint32_t*)
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
        0, 10000 * 28 * sizeof(uint32_t),
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    glGenVertexArrays(1, &m_instance_list_vao);
    glBindVertexArray(m_instance_list_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_instance_objects);
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 1, GL_INT, 28 * sizeof(uint32_t),
        (GLvoid*)(20 * sizeof(uint32_t)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_command);
}   // CullingManager

// ----------------------------------------------------------------------------
void CullingManager::addMesh(scene::ISceneNode* node, GLMesh* mesh,
                             int material_id)
{
    if (m_visualization) return;
    if (mesh->mb == NULL)
        return;
    m_mesh_lists[material_id][mesh->mb].m_obj.emplace_back(node, mesh);
}   // addMesh

// ----------------------------------------------------------------------------
void CullingManager::generateDrawCall()
{
    if (m_visualization) return;
    PROFILER_PUSH_CPU_MARKER("- GPU culling", 0x0, 0x50, 0xFF);
    for (int i = 0; i < 11; i++)
    {
        STK::tuple_get<0>(m_mesh_infos[i]) = m_mesh_lists[i].size();
        STK::tuple_get<1>(m_mesh_infos[i]) = i == 0 ? 0 :
            STK::tuple_get<0>(m_mesh_infos[i - 1]) +
            STK::tuple_get<1>(m_mesh_infos[i - 1]);
        for (auto& p : m_mesh_lists[i])
            STK::tuple_get<2>(m_mesh_infos[i]) += p.second.m_obj.size();
        m_total_instance_count += m_mesh_lists[i].size();
        m_total_instance_object_count += STK::tuple_get<2>(m_mesh_infos[i]);
    }

    int offest_1 = 0;
    int offest_2 = 0;
    int instance_obj_offset = 0;
    for (int i = 0; i < 11; i++)
    {
        if (m_mesh_lists[i].empty())
            continue;
        for (auto& p : m_mesh_lists[i])
        {
            const core::aabbox3df& b = p.first->getBoundingBox();
            static const float one = 1.0f;
            memcpy(m_instances_ptr + offest_1 * 20, &b.MinEdge,
                3 * sizeof(float));
            memcpy(m_instances_ptr + 3 + offest_1 * 20, &one, sizeof(float));
            memcpy(m_instances_ptr + 4 + offest_1 * 20, &b.MaxEdge,
                3 * sizeof(float));
            m_instances_ptr[7 + offest_1 * 20] = p.second.m_obj.size();
            m_instances_ptr[8 + offest_1 * 20] = instance_obj_offset;
            m_instances_ptr[9 + offest_1 * 20] =
                p.second.m_obj[0].second->IndexCount;
            m_instances_ptr[10 + offest_1 * 20] =
                p.second.m_obj[0].second->vaoOffset / 2;
            m_instances_ptr[11 + offest_1 * 20] =
                p.second.m_obj[0].second->vaoBaseVertex;
            if (m_bindless_texture && i != 10)
            {
                memcpy(m_instances_ptr + 12 + offest_1 * 20,
                    p.second.m_obj[0].second->TextureHandles,
                    sizeof(uint64_t));
                memcpy(m_instances_ptr + 14 + offest_1 * 20,
                    p.second.m_obj[0].second->TextureHandles + 1,
                    sizeof(uint64_t));
                memcpy(m_instances_ptr + 16 + offest_1 * 20,
                    p.second.m_obj[0].second->TextureHandles + 2,
                    sizeof(uint64_t));
                memcpy(m_instances_ptr + 18 + offest_1 * 20,
                    p.second.m_obj[0].second->TextureHandles + 3,
                    sizeof(uint64_t));
            }
            instance_obj_offset += p.second.m_obj.size();
            for (auto& q : p.second.m_obj)
            {
                static const float zero[2] = { 0.0f, 0.0f };
                memcpy(m_instance_objects_ptr + offest_2 * 28,
                    q.first->getAbsoluteTransformation().pointer(),
                    16 * sizeof(float));
                if (i == 10)
                {
                    video::SColorf cf(dynamic_cast<STKMeshSceneNode*>(q.first)
                        ->getGlowColor());
                    memcpy(m_instance_objects_ptr + 16 + offest_2 * 28, &cf.b,
                        sizeof(float));
                    memcpy(m_instance_objects_ptr + 17 + offest_2 * 28, &cf.g,
                        sizeof(float));
                    memcpy(m_instance_objects_ptr + 18 + offest_2 * 28, &cf.r,
                        sizeof(float));
                    memcpy(m_instance_objects_ptr + 19 + offest_2 * 28, &cf.a,
                        sizeof(float));
                }
                else
                {
                    if (q.second->texture_trans.X != 0 &&
                        q.second->texture_trans.Y != 0)
                    {
                        memcpy(m_instance_objects_ptr + 16 + offest_2 * 28,
                            &q.second->texture_trans, 2 * sizeof(float));
                    }
                    else
                    {
                        memcpy(m_instance_objects_ptr + 16 + offest_2 * 28,
                            zero, 2 * sizeof(float));
                    }
                    if (q.second->m_render_info &&
                        q.second->m_render_info->getHue() != 0.0f &&
                        q.second->m_material)
                    {
                        memcpy(m_instance_objects_ptr + 18 + offest_2 * 28,
                            q.second->m_render_info->getHuePtr(),
                            sizeof(float));
                        memcpy(m_instance_objects_ptr + 19 + offest_2 * 28,
                            q.second->m_material->getColorizationFactorPtr(),
                            sizeof(float));
                    }
                    else
                    {
                        memcpy(m_instance_objects_ptr + 18 + offest_2 * 28,
                            zero, 2 * sizeof(float));
                    }
                }
                m_instance_objects_ptr[20 + offest_2 * 28] = offest_1;
                m_instance_objects_ptr[21 + offest_2 * 28] =
                    dynamic_cast<STKMeshCommon*>(q.first)->getMeshIdent() ==
                    "track_main" ? 1 : 0;
                STKAnimatedMesh* am = dynamic_cast<STKAnimatedMesh*>(q.first);
                if (am)
                {
                    m_instance_objects_ptr[22 + offest_2 * 28] =
                        am->getSkinningOffset();
                }
                offest_2++;
            }
            offest_1++;
        }
    }
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

    FrameBuffer* fbo = irr_driver->getFBO(FBO_NORMAL_AND_DEPTHS);
    fbo->bind();
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDrawBuffer(GL_NONE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    if (m_track_vao)
    {
        TrackDepthShader::getInstance()->use();
        TrackDepthShader::getInstance()->setPVM(0);
        glBindVertexArray(m_track_vao);
        glDrawArrays(GL_TRIANGLES, 0, m_track_vertex_count);
    }

    glDepthMask(GL_FALSE);
    if (m_bb_viz)
    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        fbo = irr_driver->getFBO(FBO_TMP2_WITH_DS);
        fbo->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindVertexArray(m_instance_list_vao);
    EarlyDepthCulling::getInstance()->use();
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1, -1);
    glDrawArrays(GL_POINTS, 0, m_total_instance_object_count);
    glPolygonOffset(0, 0);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    DrawCommandGenerator::getInstance()->use();
    glDrawArrays(GL_POINTS, 0, m_total_instance_count);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    GLenum reason = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    if (reason != GL_ALREADY_SIGNALED)
    {
        do
        {
            reason = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT,
                1000000);
        }
        while (reason == GL_TIMEOUT_EXPIRED);
    }
    glDeleteSync(sync);

#undef DEBUG_OUTPUT
#ifdef DEBUG_OUTPUT
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_draw_command);
    void* buf = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
        m_total_instance_count * 20, GL_MAP_READ_BIT);
    std::vector<DrawElementsIndirectCommand> dc(m_total_instance_count);
    memcpy(dc.data(), buf, m_total_instance_count * 20);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    dc[0];
#endif
    PROFILER_POP_CPU_MARKER();

}   // generateDrawCall

// ----------------------------------------------------------------------------
void CullingManager::cleanMeshList()
{
    if (m_visualization) return;
    for (int i = 0; i < 11; i++)
        m_mesh_lists[i].clear();
    for (auto &p : m_mesh_infos)
        p = {0, 0, 0};
    m_total_instance_count = 0;
    m_total_instance_object_count = 0;
}   // cleanMeshList

// ----------------------------------------------------------------------------
void CullingManager::addTrackMesh(irr::scene::IMesh* mesh)
{
    std::vector<core::vector3df> pos;
    for (unsigned i = 0; i < mesh->getMeshBufferCount(); i++)
    {
        scene::IMeshBuffer* mb = mesh->getMeshBuffer(i);
        if (!mb) continue;
        Material* m = material_manager->getMaterialFor(mb
            ->getMaterial().getTexture(0), mb);
        if (!m || m->getShaderType() != Material::SHADERTYPE_SOLID) continue;
        u16* indices = mb->getIndices();
        for (unsigned j = 0; j < mb->getIndexCount(); j++)
        {
            switch (mb->getVertexType())
            {
                case video::EVT_STANDARD:
                {
                    video::S3DVertex* v = (video::S3DVertex*)mb->getVertices();
                    pos.push_back(v[indices[j]].Pos);
                    break;
                }
                case video::EVT_2TCOORDS:
                {
                    video::S3DVertex2TCoords* v =
                        (video::S3DVertex2TCoords*)mb->getVertices();
                    pos.push_back(v[indices[j]].Pos);
                    break;
                }
                default:
                    break;
            }
        }
    }
    removeTrackMesh();
    glGenVertexArrays(1, &m_track_vao);
    glGenBuffers(1, &m_track_vbo);
    glBindVertexArray(m_track_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_track_vbo);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * 3 * sizeof(float), pos.data(),
        GL_STATIC_COPY);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_track_vertex_count = pos.size();
}   // addTrackMesh

// ----------------------------------------------------------------------------
void CullingManager::removeTrackMesh()
{
    if (m_track_vao)
    {
        glDeleteVertexArrays(1, &m_track_vao);
        m_track_vao = 0;
    }
    if (m_track_vbo)
    {
        glDeleteBuffers(1, &m_track_vbo);
        m_track_vao = 0;
    }
}   // removeTrackMesh

// ----------------------------------------------------------------------------
bool CullingManager::bindInstanceVAO(video::E_VERTEX_TYPE vt)
{
    bool need_bind = false;
    switch (vt)
    {
        case video::EVT_STANDARD:
        {
            for (int i = 0; i <= 3; i++)
            {
                if (!m_mesh_lists[i].empty())
                {
                    need_bind = true;
                    break;
                }
            }
            break;
        }
        case video::EVT_TANGENTS:
        {
            if (!m_mesh_lists[4].empty())
                need_bind = true;
            break;
        }
        case video::EVT_SKINNED_MESH:
        {
            for (int i = 5; i <= 8; i++)
            {
                if (!m_mesh_lists[i].empty())
                {
                    need_bind = true;
                    break;
                }
            }
            break;
        }
        case video::EVT_2TCOORDS:
        {
            if (!m_mesh_lists[9].empty())
                need_bind = true;
            break;
        }
    }
    if (need_bind)
    {
        glBindVertexArray(VAOManager::getInstance()
            ->getInstanceVAO(vt, InstanceTypeCulling));
    }
    return need_bind;
}   // bindInstanceVAO

// ----------------------------------------------------------------------------
void CullingManager::drawGlow()
{
    if (m_mesh_lists[10].empty()) return;
    glBindVertexArray(VAOManager::getInstance()
        ->getInstanceVAO(video::EVT_STANDARD, InstanceTypeCulling));
    InstancedColorizeShader::getInstance()->use();
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
        (GLvoid*)(STK::tuple_get<1>(m_mesh_infos[10]) *
        sizeof(DrawElementsIndirectCommand)),
        STK::tuple_get<0>(m_mesh_infos[10]),
        sizeof(DrawElementsIndirectCommand));
}   // drawGlow
