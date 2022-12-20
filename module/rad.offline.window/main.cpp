#include <rad/core/common.h>
#include <rad/core/logger.h>
#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/render/renderer.h>
#include <rad/realtime/window.h>
#include <rad/realtime/opengl_context.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

const char* toSrgbSource =
    "#version 450 core\n"
    "layout(local_size_x = 1, local_size_y = 1) in;\n"
    "layout(rgba32f, binding = 0) uniform image2D fb;\n"
    "float toSrgb(float v) {\n"
    "  return (v <= 0.0031308f) ? (12.92f * v) : ((1.0f + 0.055f) * pow(v, 1.0f / 2.4f) - 0.055f);\n"
    "}\n"
    "void main() {\n"
    "  ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);\n"
    "  vec3 raw = imageLoad(fb, texelCoord).xyz;"
    "  vec4 res = vec4(toSrgb(raw.x), toSrgb(raw.y), toSrgb(raw.z), 1.0f);"
    "  imageStore(fb, texelCoord, res);\n"
    "}";

int main(int argc, char** argv) {
  Rad::RadCoreInit();
  Rad::RadInitGlfw();
  try {
    // 创建渲染器
    Rad::Unique<Rad::LocationResolver> resolver;
    Rad::Unique<Rad::Renderer> renderer;
    {
      std::string scenePath;
      for (int i = 0; i < argc;) {
        std::string cmd(argv[i]);
        if (cmd == "--scene") {
          scenePath = std::string(argv[i + 1]);
          i += 2;
        } else {
          i++;
        }
      }
      if (scenePath.empty()) {
        throw Rad::RadArgumentException("should input cmd like \"--scene <scene.json>\"");
      }
      std::filesystem::path p(scenePath);
      if (!std::filesystem::exists(p)) {
        throw Rad::RadArgumentException("cannot open file: {}", scenePath);
      }
      nlohmann::json cfg;
      {
        std::ifstream cfgStream(p);
        if (!cfgStream.is_open()) {
          throw Rad::RadArgumentException("cannot open file: {}", scenePath);
        }
        cfg = nlohmann::json::parse(cfgStream);
      }
      Rad::BuildContext ctx{};
      ctx.SetFromJson(cfg);
      ctx.SetDefaultFactoryManager();
      ctx.SetDefaultAssetManager(p.parent_path().string());
      renderer = ctx.Build();
      resolver = std::make_unique<Rad::LocationResolver>(ctx.GetAssetManager().GetLocationResolver());
      resolver->SetSaveName(p.filename().replace_extension().string());
    }
    // 初始化窗口
    {
      Rad::GlfwWindowOptions opts{};
      opts.Title = "Rad Renderer";
      opts.EnableOpenGL = true;
#if defined(RAD_DEFINE_DEBUG)
      opts.EnableOpenGLDebugLayer = true;
#else
      opts.EnableOpenGLDebugLayer = false;
#endif
      opts.Size = Rad::Vector2i(1280, 720);
      Rad::RadCreateWindowGlfw(opts);
      Rad::RadShowWindowGlfw();
      Rad::RadInitContextOpenGL();
#if defined(RAD_DEFINE_DEBUG)
      Rad::RadSetupDebugLayerOpenGL();
#endif
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGui::StyleColorsLight();
      ImGui::GetStyle().ScaleAllSizes(1);
      ImGui_ImplGlfw_InitForOpenGL(Rad::RadWindowHandlerGlfw(), false);
      ImGui_ImplOpenGL3_Init("#version 450 core");
      Rad::AddWindowFocusEventGlfw(ImGui_ImplGlfw_WindowFocusCallback);
      Rad::AddCursorEnterEventGlfw(ImGui_ImplGlfw_CursorEnterCallback);
      Rad::AddCursorPosEventGlfw([](auto w, auto&& p) { ImGui_ImplGlfw_CursorPosCallback(w, p.x(), p.y()); });
      Rad::AddMouseButtonEventGlfw(ImGui_ImplGlfw_MouseButtonCallback);
      Rad::AddScrollEventGlfw([](auto w, auto&& p) { ImGui_ImplGlfw_ScrollCallback(w, p.x(), p.y()); });
      Rad::AddKeyEventGlfw(ImGui_ImplGlfw_KeyCallback);
      Rad::AddCharEventGlfw(ImGui_ImplGlfw_CharCallback);
      Rad::AddMonitorEventGlfw(ImGui_ImplGlfw_MonitorCallback);
    }
    auto&& scene = renderer->GetScene();
    auto&& camera = scene.GetCamera();
    auto&& rendererFb = camera.GetFrameBuffer();
    // 准备窗口
    GLuint rendererFbGL = 0;
    GLuint toSrgb = 0;
    {
      glCreateTextures(GL_TEXTURE_2D, 1, &rendererFbGL);
      glTextureParameteri(rendererFbGL, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(rendererFbGL, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureParameteri(rendererFbGL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTextureParameteri(rendererFbGL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureStorage2D(rendererFbGL, 1, GL_RGBA32F, (GLsizei)rendererFb.rows(), (GLsizei)rendererFb.cols());
      glTextureSubImage2D(
          rendererFbGL,
          0,
          0, 0,
          (GLsizei)rendererFb.rows(), (GLsizei)rendererFb.cols(),
          GL_RGB, GL_FLOAT,
          rendererFb.data());

      GLuint toSrgbShader = glCreateShader(GL_COMPUTE_SHADER);
      glShaderSource(toSrgbShader, 1, &toSrgbSource, nullptr);
      glCompileShader(toSrgbShader);
      int success;
      glGetShaderiv(toSrgbShader, GL_COMPILE_STATUS, &success);
      if (success) {
        toSrgb = glCreateProgram();
        glAttachShader(toSrgb, toSrgbShader);
        glLinkProgram(toSrgb);
        glGetProgramiv(toSrgb, GL_LINK_STATUS, &success);
        if (!success) {
          char infoLog[512];
          glGetProgramInfoLog(toSrgb, 512, NULL, infoLog);
          Rad::Logger::Get()->error("cannot link program, reason:\n{}", std::string(infoLog));
        }
      } else {
        char infoLog[512];
        glGetShaderInfoLog(toSrgbShader, 512, NULL, infoLog);
        Rad::Logger::Get()->error("cannot compile shader, reason:\n{}", std::string(infoLog));
      }

      Rad::RadSetWindowSizeGlfw({rendererFb.rows(), rendererFb.cols() + 70});
    }
    Rad::Logger::Get()->info("start rendering...");
    renderer->Start();
    {
      while (!Rad::RadShouldCloseWindowGlfw()) {
        Rad::RadPollEventGlfw();
        auto size = Rad::RadGetFrameBufferSizeGlfw();
        glViewport(0, 0, size.x(), size.y());
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glTextureSubImage2D(
            rendererFbGL,
            0,
            0, 0,
            (GLsizei)rendererFb.rows(), (GLsizei)rendererFb.cols(),
            GL_RGB, GL_FLOAT,
            rendererFb.data());
        if (toSrgb != 0) {
          glUseProgram(toSrgb);
          glBindImageTexture(0, rendererFbGL, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
          glDispatchCompute((GLuint)rendererFb.rows(), (GLuint)rendererFb.cols(), 1);
          glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
          const ImGuiWindowFlags windowFlag = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
          const ImGuiViewport* viewport = ImGui::GetMainViewport();
          ImGui::SetNextWindowPos(viewport->WorkPos);
          ImGui::SetNextWindowSize(viewport->WorkSize);
          if (ImGui::Begin("Preview", nullptr, windowFlag)) {
            float progress = (float)renderer->CompletedTaskCount() / renderer->AllTaskCount();
            float speed = (float)renderer->CompletedTaskCount() / renderer->ElapsedTime();
            float eta = speed * (renderer->AllTaskCount() - renderer->CompletedTaskCount());
            ImGui::ProgressBar(progress, ImVec2(-1.0f, 20.0f));
            ImGui::Text("Elapsed: %lld ms", renderer->ElapsedTime());
            ImGui::Text("ETA: %.2f ms", eta);
            ImGui::Image((void*)(intptr_t)rendererFbGL, ImVec2((float)rendererFb.rows(), (float)rendererFb.cols()));
          }
          ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        Rad::RadSwapBuffersGlfw();
        std::this_thread::yield();
      }
    }
    if (renderer->IsComplete()) {
      renderer->Wait();
      renderer->SaveResult(*resolver);
      Rad::Logger::Get()->info("render task done");
    } else {
      renderer->Stop();
      renderer->Wait();
      Rad::Logger::Get()->info("cancel render task...");
    }
    glDeleteTextures(1, &rendererFbGL);
    if (toSrgb != 0) {
      glDeleteProgram(toSrgb);
    }
  } catch (const std::exception& e) {
    Rad::Logger::Get()->error("unhandled exception: {}", e.what());
  } catch (...) {
    Rad::Logger::Get()->error("unknown exception");
  }
  Rad::RadShutdownGlfw();
  Rad::RadCoreShutdown();
}
