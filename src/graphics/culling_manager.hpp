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

#ifndef HEADER_CULLING_MANAGER_HPP
#define HEADER_CULLING_MANAGER_HPP

#include "graphics/gl_headers.hpp"
#include "graphics/draw_tools.hpp"
#include "utils/no_copy.hpp"
#include "utils/singleton.hpp"
#include "utils/tuple.hpp"

#include "aabbox3d.h"
#include "irrList.h"
#include "S3DVertex.h"

#include <algorithm>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace irr
{
    namespace video { class IVideoDriver; class ITexture; }
    namespace scene { class IMesh; class IMeshBuffer; class ISceneNode; }
}
using namespace irr;
struct GLMesh;

class CullingManager : public Singleton<CullingManager>, NoCopy
{
private:
    struct InstanceData
    {
        std::vector<std::pair<scene::ISceneNode*, GLMesh*> > m_obj;
    };

    GLuint m_track_vao, m_track_vbo, m_instance_list_vao,
        m_draw_command, m_instances, m_instance_objects;

    unsigned m_track_vertex_count, m_total_instance_count,
        m_total_instance_object_count;

    uint32_t* m_instances_ptr;
    uint32_t* m_instance_objects_ptr;

    bool m_visualization, m_bb_viz, m_bindless_texture;

    std::unordered_map<scene::IMeshBuffer*, InstanceData> m_mesh_lists[11];

    int m_mesh_list_offsets[11];

public:
    // ------------------------------------------------------------------------
    CullingManager();
    // ------------------------------------------------------------------------
    ~CullingManager()                                                        {}
    // ------------------------------------------------------------------------
    void addTrackMesh(scene::IMesh* mesh);
    // ------------------------------------------------------------------------
    void removeTrackMesh();
    // ------------------------------------------------------------------------
    void toggleViz()                    { m_visualization = !m_visualization; }
    // ------------------------------------------------------------------------
    bool getViz() const                             { return m_visualization; }
    // ------------------------------------------------------------------------
    void toggleBBViz()                                { m_bb_viz = !m_bb_viz; }
    // ------------------------------------------------------------------------
    bool getBBViz() const                                  { return m_bb_viz; }
    // ------------------------------------------------------------------------
    void resetDebug()            { m_visualization = false; m_bb_viz = false; }
    // ------------------------------------------------------------------------
    void cleanMeshList();
    // ------------------------------------------------------------------------
    void addMesh(scene::ISceneNode* node, GLMesh* mesh, int material_id);
    // ------------------------------------------------------------------------
    void generateDrawCall();
    // ------------------------------------------------------------------------
    bool bindInstanceVAO(video::E_VERTEX_TYPE vt);
    // ------------------------------------------------------------------------
    template<typename Material, typename...Uniforms>
    void drawSolidFirstPass(Uniforms...uniforms) const
    {
        Material::InstancedFirstPassShader::getInstance()->use();
        Material::InstancedFirstPassShader::getInstance()
            ->setUniforms(uniforms...);
        const unsigned int mat_int = unsigned(Material::MaterialType);
        unsigned int count = 0;
        for (auto &p : m_mesh_lists[mat_int])
        {
            GLMesh* mesh = p.second.m_obj[0].second;
            TexExpander<typename Material::InstancedFirstPassShader>::template
                expandTex(*mesh, Material::FirstPassTextures);
            if (!mesh->mb->getMaterial().BackfaceCulling)
                glDisable(GL_CULL_FACE);
            glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
                (GLvoid*)((m_mesh_list_offsets[mat_int] + count) *
                sizeof(DrawElementsIndirectCommand)));
            if (!mesh->mb->getMaterial().BackfaceCulling)
                glEnable(GL_CULL_FACE);
            count++;
        }
    }
    // ------------------------------------------------------------------------
    template<typename Material, typename...Uniforms>
    void drawSolidSecondPass(const std::vector<GLuint>& prefilled_tex,
                             const std::vector<uint64_t>& handles,
                             Uniforms...uniforms) const
    {
        Material::InstancedSecondPassShader::getInstance()->use();
        Material::InstancedSecondPassShader::getInstance()
            ->setUniforms(uniforms...);
        const unsigned int mat_int = unsigned(Material::MaterialType);
        unsigned int count = 0;
        for (auto &p : m_mesh_lists[mat_int])
        {
            GLMesh* mesh = p.second.m_obj[0].second;
            expandTexSecondPass<Material>(*mesh, prefilled_tex);
            if (!mesh->mb->getMaterial().BackfaceCulling)
                glDisable(GL_CULL_FACE);
            glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
                (GLvoid*)((m_mesh_list_offsets[mat_int] + count) *
                sizeof(DrawElementsIndirectCommand)));
            if (!mesh->mb->getMaterial().BackfaceCulling)
                glEnable(GL_CULL_FACE);
            count++;
        }
    }
    // ------------------------------------------------------------------------
    void drawGlow();
    // ------------------------------------------------------------------------
    GLuint getInstances() const                         { return m_instances; }
};

#endif
