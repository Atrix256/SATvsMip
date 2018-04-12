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
    Gui::DropdownList cameraDropdown;
    cameraDropdown.push_back({ ModelViewCamera, "Model-View" });
    cameraDropdown.push_back({ FirstPersonCamera, "First-Person" });
    cameraDropdown.push_back({ SixDoFCamera, "6 DoF" });

    mpGui->addDropdown("Camera Type", cameraDropdown, (uint32_t&)mCameraType);
}

void ModelViewer::onLoad()
{
    m_mesh.Init(
        {
            {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        },
        "Mesh.vs.hlsl",
        "Mesh.ps.hlsl");

    mpCamera = Camera::create();

    mModelViewCameraController.attachCamera(mpCamera);
    mFirstPersonCameraController.attachCamera(mpCamera);
    m6DoFCameraController.attachCamera(mpCamera);

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);
}

void ModelViewer::onFrameRender()
{
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
    bool bHandled = getActiveCameraController().onKeyEvent(keyEvent);
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