#include "Common.h"

#if defined WIN32 || __APPLE__
#define GET_DISPLAY
// maybe it works for you already!
#else
#include <X11/X.h>
#include <X11/extensions/Xrandr.h>
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
#define GET_DISPLAY glConfig.OGL.Disp=glfwGetX11Display();
#endif

Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};

class DistortedExample : public RiftGlfwApp {
protected:
  gl::Texture2dPtr sceneTextures[2];
  ovrGLTexture eyeTextures[2];
  ovrEyeRenderDesc eyeRenderDescs[2];

public:
  virtual ~DistortedExample() {
  }

  void initGl() {
    RiftGlfwApp::initGl();

    for_each_eye([&](ovrEyeType eye){
      glm::uvec2 textureSize;
      GlUtils::getImageAsTexture(sceneTextures[eye], 
        SCENE_IMAGES[eye], textureSize);

      memset(eyeTextures + eye, 0,
        sizeof(eyeTextures[eye]));

      ovrTextureHeader & eyeTextureHeader =
        eyeTextures[eye].OGL.Header;

      eyeTextureHeader.TextureSize = Rift::toOvr(textureSize);
      eyeTextureHeader.RenderViewport.Size = 
        eyeTextureHeader.TextureSize;

      eyeTextureHeader.API = ovrRenderAPI_OpenGL;

      eyeTextures[eye].OGL.TexId =
        sceneTextures[eye]->texture;
    });

    ovrGLConfig glConfig;
    //ovrRenderAPIConfig cfg; 
    memset(&glConfig, 0, sizeof(glConfig));
    glConfig.OGL.Header.API = ovrRenderAPI_OpenGL;
    glConfig.OGL.Header.RTSize= Rift::toOvr(windowSize);
    glConfig.OGL.Header.Multisample = 1;
    GET_DISPLAY;

    int distortionCaps = ovrDistortionCap_Vignette
      | ovrDistortionCap_Chromatic
      | ovrDistortionCap_TimeWarp
      ;
    int configResult = ovrHmd_ConfigureRendering(hmd, &glConfig.Config,
      distortionCaps, hmdDesc.DefaultEyeFov, eyeRenderDescs);
    if (0 == configResult) {
      FAIL("Unable to configure rendering");
    }
  }

  virtual void finishFrame() {
  }

  void draw() {
    static int frameIndex = 0;
    ovrHmd_BeginFrame(hmd, frameIndex++);
    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmdDesc.EyeRenderOrder[i];
      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);
      ovrHmd_EndEyeRender(hmd, eye, renderPose,
        &eyeTextures[eye].Texture);
    }
    ovrHmd_EndFrame(hmd);
  }
};

RUN_OVR_APP(DistortedExample)
