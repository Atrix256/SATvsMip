
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

        RasterizerState::Desc solidDesc;
        solidDesc.setCullMode(RasterizerState::CullMode::Back);
        m_rasterState = RasterizerState::create(solidDesc);
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

    ModelViewCameraController mModelViewCameraController;
    FirstPersonCameraController mFirstPersonCameraController;
    SixDoFCameraController m6DoFCameraController;

    Sampler::SharedPtr mpLinearSampler = nullptr;
    
    GraphicsState::SharedPtr mpGraphicsState = nullptr;

    enum
    {
        ModelViewCamera,
        FirstPersonCamera,
        SixDoFCamera
    } mCameraType = ModelViewCamera;

    CameraController& getActiveCameraController();

    Camera::SharedPtr mpCamera;


    float mNearZ;
    float mFarZ;

    CMesh m_mesh;
};

// TODO: clean out stuff you don't care about like cull mode

// TODO: make the mesh be affected by the camera