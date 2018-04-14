
#pragma once
#include "Falcor.h"
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
        m_vertexCount = (uint32_t)vertices.size();

        // create depth stencil state
        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthTest(true);
        dsDesc.setDepthWriteMask(true);
        dsDesc.setDepthFunc(DepthStencilState::Func::Less);
        dsDesc.setStencilTest(false);
        dsDesc.setStencilWriteMask(false);
        m_DepthStencilState = DepthStencilState::create(dsDesc);

        // create shader program and program vars
        std::string gs;
        Program::DefineList defs;
        const std::string vs(vsFile.empty() ? "Framework/Shaders/FullScreenPass.vs.slang" : vsFile);
        m_Program = GraphicsProgram::createFromFile(vs, psFile, gs, "", "", defs);
        m_ProgramVars = GraphicsVars::create(m_Program->getActiveVersion()->getReflector());

        // create vertex buffer
        const uint32_t vbSize = (uint32_t)(sizeof(Vertex)*vertices.size());
        m_VB = Buffer::create(vbSize, Buffer::BindFlags::Vertex, Buffer::CpuAccess::Write, (void*)&vertices[0]);

        // make the vertex array object (the description of the vertex buffer)
        VertexBufferLayout::SharedPtr pBufLayout = VertexBufferLayout::create();
        pBufLayout->addElement("POSITION", 0, ResourceFormat::RGB32Float, 1, 0);
        pBufLayout->addElement("TEXCOORD", 12, ResourceFormat::RG32Float, 1, 1);
        VertexLayout::SharedPtr pLayout = VertexLayout::create();
        pLayout->addBufferLayout(0, pBufLayout);
        Vao::BufferVec buffers{ m_VB };
        m_VAO = Vao::create(Vao::Topology::TriangleList, pLayout, buffers);

        // create raster state
        RasterizerState::Desc solidDesc;
        solidDesc.setCullMode(RasterizerState::CullMode::None);
        m_rasterState = RasterizerState::create(solidDesc);

        // create the graphics state and set it up
        m_PipelineState = GraphicsState::create();
        m_PipelineState->setProgram(m_Program);
        m_PipelineState->setVao(m_VAO);
        m_PipelineState->setRasterizerState(m_rasterState);
        m_PipelineState->setDepthStencilState(m_DepthStencilState);

    }

    void Render(RenderContext* pRenderContext, Fbo::SharedPtr& fbo, const GraphicsState::Viewport& viewport, const GraphicsState::Scissor& scissors) const
    {
        m_PipelineState->setFbo(fbo, false);
        m_PipelineState->setViewport(0, viewport, false);
        m_PipelineState->setScissors(0, scissors);

        // render
        pRenderContext->pushGraphicsState(m_PipelineState);
        pRenderContext->pushGraphicsVars(m_ProgramVars);
        pRenderContext->draw(m_vertexCount, 0);
        pRenderContext->popGraphicsState();
        pRenderContext->popGraphicsVars();
    }

    Buffer::SharedPtr               m_VB;
    Vao::SharedPtr                  m_VAO;
    GraphicsState::SharedPtr        m_PipelineState;
    DepthStencilState::SharedPtr    m_DepthStencilState;
    GraphicsProgram::SharedPtr      m_Program;
    uint32_t                        m_vertexCount;
    GraphicsVars::SharedPtr         m_ProgramVars;

    RasterizerState::SharedPtr      m_rasterState;
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
    FirstPersonCameraController mFirstPersonCameraController;
    Camera::SharedPtr mpCamera;

    Sampler::SharedPtr mpLinearSampler = nullptr;

    CMesh m_meshLeft;
    CMesh m_meshRight;
};

// TODO: clean out stuff you don't care about like cull mode

// TODO: maybe make the mesh class stuff part of the normal class?

// TODO: better understand the objects and make sure you are setting them all correctly

// TODO: sample textures.

// TODO: render side by side viewports, one for mip, one for SAT

// TODO: do the actual SAT vs MIP test

// TODO: hungarian notation and capitalization etc isn't great

// TODO: make two side by side viewports, so that they render the exact same.

// TODO: hide imgui by default in this demo?