#include "Common.h"
#include "CubeScene.h"

#if defined WIN32 || __APPLE__
#define GET_DISPLAY
// maybe it works for you already!
#else
#include <X11/X.h>
#include <X11/extensions/Xrandr.h>
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
#define GET_DISPLAY cfg.PlatformData[0]=(uintptr_t)glfwGetX11Display();

#endif

static const glm::uvec2 WINDOW_SIZE(1280, 800);

struct PerEyeArg {
  glm::mat4                     modelviewOffset;
  glm::mat4                     projection;
  gl::FrameBufferWrapper        frameBuffer;
  ovrGLTexture                  texture;
};

class CubeScene_Rift: public CubeScene {
  PerEyeArg eyes[2];
  int frameIndex{ 0 };

public:
  CubeScene_Rift() {
    windowSize = WINDOW_SIZE;
  }

  virtual void initGl() {
    CubeScene::initGl();

    ovrRenderAPIConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.RTSize = hmdDesc.Resolution;
    cfg.Header.Multisample = 1;
    GET_DISPLAY

    int distortionCaps = ovrDistortionCap_Chromatic;
    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
        distortionCaps, hmdDesc.DefaultEyeFov, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye){
      PerEyeArg & eyeArg = eyes[eye];
      ovrFovPort fov = hmdDesc.DefaultEyeFov[eye];
      ovrTextureHeader & textureHeader = eyeArg.texture.Texture.Header;
      ovrSizei texSize = ovrHmd_GetFovTextureSize(hmd, eye, fov, 1.0f);
      textureHeader.API = ovrRenderAPI_OpenGL;
      textureHeader.TextureSize = texSize;
      textureHeader.RenderViewport.Size = texSize;
      textureHeader.RenderViewport.Pos.x = 0;
      textureHeader.RenderViewport.Pos.y = 0;
      eyeArg.frameBuffer.init(Rift::fromOvr(texSize));
      eyeArg.texture.OGL.TexId = eyeArg.frameBuffer.color->texture;

      ovrVector3f offset = eyeRenderDescs[eye].ViewAdjust;
      ovrMatrix4f projection = ovrMatrix4f_Projection(fov, 0.01f, 100, true);

      eyeArg.projection = Rift::fromOvr(projection);
      eyeArg.modelviewOffset = glm::translate(glm::mat4(), Rift::fromOvr(offset));
    });
  }

  virtual void finishFrame() {
  }

  virtual void draw() {
    ovrHmd_BeginFrame(hmd, frameIndex++);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmdDesc.EyeRenderOrder[i];
      PerEyeArg & eyeArgs = eyes[eye];
      gl::Stacks::projection().top() = eyeArgs.projection;

      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);

      eyeArgs.frameBuffer.withFramebufferActive([&]{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::Stacks::with_push(mv, [&]{
          mv.preMultiply(eyeArgs.modelviewOffset);
          drawCubeScene();
        });
      });

      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeArgs.texture.Texture);
    }

    ovrHmd_EndFrame(hmd);
  }
};

RUN_OVR_APP(CubeScene_Rift);
