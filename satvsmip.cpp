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
#include "satvsmip.h"

CameraController& ModelViewer::getActiveCameraController()
{
    switch (mCameraType)
    {
    case ModelViewer::ModelViewCamera:
        return mModelViewCameraController;
    case ModelViewer::FirstPersonCamera:
        return mFirstPersonCameraController;
    case ModelViewer::SixDoFCamera:
        return m6DoFCameraController;
    default:
        should_not_get_here();
        return m6DoFCameraController;
    }
}

void ModelViewer::onGuiRender()
{
    mpGui->addCheckBox("Wireframe", mDrawWireframe);
    mpGui->addCheckBox("TriLinear Filtering", mUseTriLinearFiltering);

    Gui::DropdownList cullList;
    cullList.push_back({ 0, "No Culling" });
    cullList.push_back({ 1, "Backface Culling" });
    cullList.push_back({ 2, "Frontface Culling" });
    mpGui->addDropdown("Cull Mode", cullList, mCullMode);

    if (mpGui->beginGroup("Lights"))
    {
        mpGui->addRgbColor("Ambient intensity", mAmbientIntensity);
        if (mpGui->beginGroup("Directional Light"))
        {
            mpDirLight->renderUI(mpGui.get());
            mpGui->endGroup();
        }
        if (mpGui->beginGroup("Point Light"))
        {
            mpPointLight->renderUI(mpGui.get());
            mpGui->endGroup();
        }
        mpGui->endGroup();
    }

    Gui::DropdownList cameraDropdown;
    cameraDropdown.push_back({ ModelViewCamera, "Model-View" });
    cameraDropdown.push_back({ FirstPersonCamera, "First-Person" });
    cameraDropdown.push_back({ SixDoFCamera, "6 DoF" });

    mpGui->addDropdown("Camera Type", cameraDropdown, (uint32_t&)mCameraType);
}

void ModelViewer::onLoad()
{
    mpCamera = Camera::create();
    mpProgram = GraphicsProgram::createFromFile("", appendShaderExtension("ModelViewer.ps"));

    // create rasterizer state
    RasterizerState::Desc wireframeDesc;
    wireframeDesc.setFillMode(RasterizerState::FillMode::Wireframe);
    wireframeDesc.setCullMode(RasterizerState::CullMode::None);
    mpWireframeRS = RasterizerState::create(wireframeDesc);

    RasterizerState::Desc solidDesc;
    solidDesc.setCullMode(RasterizerState::CullMode::None);
    mpCullRastState[0] = RasterizerState::create(solidDesc);
    solidDesc.setCullMode(RasterizerState::CullMode::Back);
    mpCullRastState[1] = RasterizerState::create(solidDesc);
    solidDesc.setCullMode(RasterizerState::CullMode::Front);
    mpCullRastState[2] = RasterizerState::create(solidDesc);

    // Depth test
    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthTest(false);
    mpNoDepthDS = DepthStencilState::create(dsDesc);
    dsDesc.setDepthTest(true);
    mpDepthTestDS = DepthStencilState::create(dsDesc);

    mModelViewCameraController.attachCamera(mpCamera);
    mFirstPersonCameraController.attachCamera(mpCamera);
    m6DoFCameraController.attachCamera(mpCamera);

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point);
    mpPointSampler = Sampler::create(samplerDesc);
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);

    mpDirLight = DirectionalLight::create();
    mpPointLight = PointLight::create();
    mpDirLight->setWorldDirection(glm::vec3(0.13f, 0.27f, -0.9f));

    mpProgramVars = GraphicsVars::create(mpProgram->getActiveVersion()->getReflector());
    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);

    // create a model
    {
        struct Vertex
        {
            glm::vec4 position;
        };

        static const Vertex kVertices[] =
        {
            { glm::vec4(0.5, 0.5, 0.5, 1.0f) },
            { glm::vec4(0.7, 0.5, 0.5, 1.0f) },
            { glm::vec4(0.7, 0.7, 0.5, 1.0f) },
        };

        static const uint32_t kIndices[] =
        {
            0, 1, 2
        };

        // create vertex buffer
        const uint32_t vbSize = (uint32_t)(sizeof(Vertex)*arraysize(kVertices));
        spVertexBuffer = Buffer::create(vbSize, Buffer::BindFlags::Vertex, Buffer::CpuAccess::Write, (void*)kVertices);

        // create index buffer
        const uint32_t ibSize = (uint32_t)sizeof(kIndices);
        spIndexBuffer = Buffer::create(ibSize, Buffer::BindFlags::Index, Buffer::CpuAccess::Write, (void*)kIndices);

        // create VAO
        VertexLayout::SharedPtr pLayout = VertexLayout::create();
        VertexBufferLayout::SharedPtr pBufLayout = VertexBufferLayout::create();
        pBufLayout->addElement("POSITION", 0, ResourceFormat::RGBA32Float, 1, 0);
        pLayout->addBufferLayout(0, pBufLayout);

        Vao::BufferVec buffers{ spVertexBuffer };
        spVao = Vao::create(Vao::Topology::TriangleStrip, pLayout, buffers);

        Material::SharedPtr material = Material::create("moof");

        BoundingBox boundingBox = BoundingBox::fromMinMax(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.7f, 0.7f, 0.7f));

        mpMesh = Mesh::create(buffers, arraysize(kVertices), spIndexBuffer, arraysize(kIndices), pLayout, Vao::Topology::TriangleList, material, boundingBox, false);

        mpModel = Model::create();
        mpModel->addMeshInstance(mpMesh, glm::mat4());
        int ijkl = 0;

        // TODO: clean up unused stuff that is created
    }

    /*
    // TODO: create a model!
    {
        auto vertexBufferLayout = VertexBufferLayout::create();
        vertexBufferLayout->addElement("POSITION", 0, ResourceFormat::RGBA32Float, 1, 0);

        auto vertexLayout = VertexLayout::create();
        vertexLayout->addBufferLayout(0, vertexBufferLayout);

        float4 vertices[] = 
        {
            { 0.5f, 0.5f, 0.0f, 1.0f },
            { 0.6f, 0.5f, 0.0f, 1.0f },
            { 0.6f, 0.6f, 0.0f, 1.0f },
        };

        Vao::BufferVec bufferVec;
        bufferVec.push_back(

        Vao::BufferVec vertexBuffer = Vao::create(Topology::TriangleList, vertexLayout, vertices, );

        //mpMesh = Mesh::create();
        mpModel = Model::create();
    }
    */
}

void ModelViewer::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearFbo(mpDefaultFBO.get(), clearColor, 1.0f, 0, FboAttachmentType::All);
    mpGraphicsState->setFbo(mpDefaultFBO);

    if (mpModel)
    {
        mpCamera->setDepthRange(mNearZ, mFarZ);
        getActiveCameraController().update();

        // Animate
        if (mAnimate)
        {
            PROFILE(animate);
            mpModel->animate(mCurrentTime);
        }

        // Set render state
        if (mDrawWireframe)
        {
            mpGraphicsState->setRasterizerState(mpWireframeRS);
            mpGraphicsState->setDepthStencilState(mpNoDepthDS);
            mpProgramVars["PerFrameCB"]["gConstColor"] = true;
        }
        else
        {
            mpGraphicsState->setRasterizerState(mpCullRastState[mCullMode]);
            mpGraphicsState->setDepthStencilState(mpDepthTestDS);
            mpProgramVars["PerFrameCB"]["gConstColor"] = false;

            mpDirLight->setIntoConstantBuffer(mpProgramVars["PerFrameCB"].get(), "gDirLight");
            mpPointLight->setIntoConstantBuffer(mpProgramVars["PerFrameCB"].get(), "gPointLight");
        }

        if (mUseTriLinearFiltering)
        {
            mpModel->bindSamplerToMaterials(mpLinearSampler);
        }
        else
        {
            mpModel->bindSamplerToMaterials(mpPointSampler);
        }

        mpProgramVars["PerFrameCB"]["gAmbient"] = mAmbientIntensity;
        mpGraphicsState->setProgram(mpProgram);
        mpRenderContext->setGraphicsState(mpGraphicsState);
        mpRenderContext->setGraphicsVars(mpProgramVars);
        ModelRenderer::render(mpRenderContext.get(), mpModel, mpCamera.get());
    }
}

void ModelViewer::onShutdown()
{

}

bool ModelViewer::onKeyEvent(const KeyboardEvent& keyEvent)
{
    bool bHandled = getActiveCameraController().onKeyEvent(keyEvent);
    if (bHandled == false)
    {
        if (keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            switch (keyEvent.key)
            {
            case KeyboardEvent::Key::R:
                resetCamera();
                bHandled = true;
                break;
            }
        }
    }
    return bHandled;
}

bool ModelViewer::onMouseEvent(const MouseEvent& mouseEvent)
{
    return getActiveCameraController().onMouseEvent(mouseEvent);
}

void ModelViewer::onResizeSwapChain()
{
    float height = (float)mpDefaultFBO->getHeight();
    float width = (float)mpDefaultFBO->getWidth();

    mpCamera->setFocalLength(21.0f);
    float aspectRatio = (width / height);
    mpCamera->setAspectRatio(aspectRatio);
}

void ModelViewer::resetCamera()
{
    if (mpModel)
    {
        // update the camera position
        float Radius = mpModel->getRadius();
        const glm::vec3& ModelCenter = mpModel->getCenter();
        glm::vec3 CamPos = ModelCenter;
        CamPos.z += Radius * 5;

        mpCamera->setPosition(CamPos);
        mpCamera->setTarget(ModelCenter);
        mpCamera->setUpVector(glm::vec3(0, 1, 0));

        // Update the controllers
        mModelViewCameraController.setModelParams(ModelCenter, Radius, 3.5f);
        mFirstPersonCameraController.setCameraSpeed(Radius*0.25f);
        m6DoFCameraController.setCameraSpeed(Radius*0.25f);

        mNearZ = std::max(0.1f, mpModel->getRadius() / 750.0f);
        mFarZ = Radius * 10;
    }
}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
#else
int main(int argc, char** argv)
#endif
{
    ModelViewer modelViewer;
    SampleConfig config;
    config.windowDesc.title = "Falcor Model Viewer";
    config.windowDesc.resizableWindow = true;
#ifdef _WIN32
    modelViewer.run(config);
#else
    modelViewer.run(config, (uint32_t)argc, argv);
#endif
    return 0;
}