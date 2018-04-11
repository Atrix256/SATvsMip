/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once
#include "Falcor.h"
#include "SampleTest.h"
#include "Utils/Picking/Picking.h"
#include <vector>

using namespace Falcor;

struct Vertex
{
    glm::vec3 position;
    glm::vec2 texCoord;
};

class CMesh
{
public:
    void Init(const std::vector<Vertex> vertices, const std::string& vsFile, const std::string& psFile)
    {
        m_PipelineState = GraphicsState::create();

        m_vertexCount = (uint32_t)vertices.size();

        // create depth stencil state
        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthTest(true);
        dsDesc.setDepthWriteMask(true);
        dsDesc.setDepthFunc(DepthStencilState::Func::LessEqual);    // Equal is needed to allow overdraw when z is enabled (e.g., background pass etc.)
        dsDesc.setStencilTest(false);
        dsDesc.setStencilWriteMask(false);
        m_DepthStencilState = DepthStencilState::create(dsDesc);

        std::string gs;
        Program::DefineList defs;
        const std::string vs(vsFile.empty() ? "Framework/Shaders/FullScreenPass.vs.slang" : vsFile);
        m_Program = GraphicsProgram::createFromFile(vs, psFile, gs, "", "", defs);
        m_ProgramVars = GraphicsVars::create(m_Program->getActiveVersion()->getReflector());
        m_PipelineState->setProgram(m_Program);


        // create vertex buffer
        const uint32_t vbSize = (uint32_t)(sizeof(Vertex)*vertices.size());
        m_VB = Buffer::create(vbSize, Buffer::BindFlags::Vertex, Buffer::CpuAccess::Write, (void*)&vertices[0]);

        // create vertex array object
        VertexLayout::SharedPtr pLayout = VertexLayout::create();
        VertexBufferLayout::SharedPtr pBufLayout = VertexBufferLayout::create();
        pBufLayout->addElement("POSITION", 0, ResourceFormat::RGB32Float, 1, 0);
        pBufLayout->addElement("TEXCOORD", 12, ResourceFormat::RG32Float, 1, 1);
        pLayout->addBufferLayout(0, pBufLayout);
        Vao::BufferVec buffers{ m_VB };
        m_VAO = Vao::create(Vao::Topology::TriangleStrip, pLayout, buffers);

        m_PipelineState->setVao(m_VAO);
    }

    void Render(RenderContext* pRenderContext, DepthStencilState::SharedPtr pDsState) const
    {
        m_PipelineState->pushFbo(pRenderContext->getGraphicsState()->getFbo(), false);
        m_PipelineState->setViewport(0, pRenderContext->getGraphicsState()->getViewport(0), false);
        m_PipelineState->setScissors(0, pRenderContext->getGraphicsState()->getScissors(0));

        m_PipelineState->setVao(m_VAO);
        m_PipelineState->setDepthStencilState(pDsState ? pDsState : m_DepthStencilState);
        pRenderContext->pushGraphicsState(m_PipelineState);
        pRenderContext->pushGraphicsVars(m_ProgramVars);
        pRenderContext->draw(m_vertexCount, 0);
        pRenderContext->popGraphicsState();
        m_PipelineState->popFbo(false);
    }

    Buffer::SharedPtr               m_VB;
    Vao::SharedPtr                  m_VAO;
    GraphicsState::SharedPtr        m_PipelineState;
    DepthStencilState::SharedPtr    m_DepthStencilState;
    GraphicsProgram::SharedPtr      m_Program;
    uint32_t                        m_vertexCount;
    GraphicsVars::SharedPtr         m_ProgramVars;
};

class ModelViewer : public Sample
{
public:
    void onLoad() override;
    void onFrameRender() override;
    void onShutdown() override;
    void onResizeSwapChain() override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;
    void onGuiRender() override;

private:

    ModelViewCameraController mModelViewCameraController;
    FirstPersonCameraController mFirstPersonCameraController;
    SixDoFCameraController m6DoFCameraController;

    bool mUseTriLinearFiltering = true;
    Sampler::SharedPtr mpPointSampler = nullptr;
    Sampler::SharedPtr mpLinearSampler = nullptr;

    GraphicsProgram::SharedPtr mpProgram = nullptr;
    
    GraphicsState::SharedPtr mpGraphicsState = nullptr;

    enum
    {
        ModelViewCamera,
        FirstPersonCamera,
        SixDoFCamera
    } mCameraType = ModelViewCamera;

    CameraController& getActiveCameraController();

    Camera::SharedPtr mpCamera;

    bool mDrawWireframe = false;
    bool mAnimate = false;
    bool mGenerateTangentSpace = true;
    glm::vec3 mAmbientIntensity = glm::vec3(0.1f, 0.1f, 0.1f);

    uint32_t mActiveAnimationID = kBindPoseAnimationID;
    static const uint32_t kBindPoseAnimationID = AnimationController::kBindPoseAnimationId;

    RasterizerState::SharedPtr mpWireframeRS = nullptr;
    RasterizerState::SharedPtr mpCullRastState[3]; // 0 = no culling, 1 = backface culling, 2 = frontface culling
    uint32_t mCullMode = 1;

    DepthStencilState::SharedPtr mpNoDepthDS = nullptr;
    DepthStencilState::SharedPtr mpDepthTestDS = nullptr;

    DirectionalLight::SharedPtr mpDirLight;
    PointLight::SharedPtr mpPointLight;

    float mNearZ;
    float mFarZ;

    CMesh m_mesh;
};

// TODO: clean out stuff you don't care about like cull mode