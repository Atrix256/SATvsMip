#include "satvsmip.h"


void ModelViewer::onGuiRender()
{
}

void ModelViewer::onLoad()
{
    std::vector<Vertex> vertices =
    {
        {{-1.0f, -1.0f, -5.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -5.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, -5.0f}, {1.0f, 1.0f}},
    };

    for (uint i = 0; i < vertices.size(); ++i)
    {
        vertices[i].position.x /= 10.0f;
        vertices[i].position.y /= 10.0f;
        vertices[i].position.z /= 10.0f;
    }

    m_mesh.Init(
        vertices,
        "Mesh.vs.hlsl",
        "Mesh.ps.hlsl");

    mpCamera = Camera::create();
    mFirstPersonCameraController.attachCamera(mpCamera);

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);
}

void ModelViewer::onFrameRender()
{
    mFirstPersonCameraController.update();

    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpRenderContext->clearFbo(mpDefaultFBO.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    m_mesh.m_ProgramVars["PerFrameCB"]["vpMtx"] = mpCamera->getViewProjMatrix();

    m_mesh.Render(mpRenderContext.get(), nullptr);
}

void ModelViewer::onShutdown()
{

}

bool ModelViewer::onKeyEvent(const KeyboardEvent& keyEvent)
{
    bool bHandled = mFirstPersonCameraController.onKeyEvent(keyEvent);
    if (bHandled == false)
    {
        if (keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            switch (keyEvent.key)
            {
            case KeyboardEvent::Key::R:
                bHandled = true;
                break;
            }
        }
    }
    return bHandled;
}

bool ModelViewer::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mFirstPersonCameraController.onMouseEvent(mouseEvent);
}

void ModelViewer::onResizeSwapChain()
{
    float height = (float)mpDefaultFBO->getHeight();
    float width = (float)mpDefaultFBO->getWidth();

    mpCamera->setFocalLength(21.0f);
    float aspectRatio = (width / height);
    mpCamera->setAspectRatio(aspectRatio);

    mpCamera->setDepthRange(0.1f, 100.0f);
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