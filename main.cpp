#include "include/imgui.h"
#include "include/imgui_impl_glfw.h"
#include "include/imgui_impl_vulkan.h"
#include "include/vulkan_setup.h"
#include <cmath>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main(int, char **) {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

  GLFWmonitor *primary_monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(primary_monitor);

  GLFWwindow *window =
      glfwCreateWindow(mode->width, mode->height, "SightX", nullptr, nullptr);
  if (!glfwVulkanSupported()) {
    printf("GLFW: Vulkan Not Supported\n");
    return 1;
  }

  glfwSetWindowOpacity(window, 0.0);

  ImVector<const char *> extensions;
  uint32_t extensions_count = 0;
  const char **glfw_extensions =
      glfwGetRequiredInstanceExtensions(&extensions_count);
  for (uint32_t i = 0; i < extensions_count; i++)
    extensions.push_back(glfw_extensions[i]);
  SetupVulkan(extensions);

  VkSurfaceKHR surface;
  VkResult err =
      glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
  check_vk_result(err);

  int w, h;
  glfwGetFramebufferSize(window, &w, &h);
  ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;
  SetupVulkanWindow(wd, surface, w, h);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = g_Instance;
  init_info.PhysicalDevice = g_PhysicalDevice;
  init_info.Device = g_Device;
  init_info.QueueFamily = g_QueueFamily;
  init_info.Queue = g_Queue;
  init_info.PipelineCache = g_PipelineCache;
  init_info.DescriptorPool = g_DescriptorPool;
  init_info.RenderPass = wd->RenderPass;
  init_info.Subpass = 0;
  init_info.MinImageCount = g_MinImageCount;
  init_info.ImageCount = wd->ImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = g_Allocator;
  init_info.CheckVkResultFn = check_vk_result;
  ImGui_ImplVulkan_Init(&init_info);

  class app_state {
  public:
    bool draw_menu;
    bool draw_crosshair;

    bool chroma_crosshair;

    int crosshair_index;
    int crosshair_size;
    int crosshair_gap;

    ImVec4 crosshair_color;

    app_state()
        : draw_menu(true), draw_crosshair(true), chroma_crosshair(false),
          crosshair_index(0), crosshair_size(3), crosshair_gap(1),
          crosshair_color(1.0f, 1.0f, 1.0f, 1.0f) {}
  };

  app_state app_state{};
  ImVec4 clear_color = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);

  while (!glfwWindowShouldClose(window)) {
    if (!io.WantCaptureKeyboard) {
      if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
        glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH,
                            app_state.draw_menu);
        app_state.draw_menu = !app_state.draw_menu;
      }
    }

    glfwPollEvents();

    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    if (fb_width > 0 && fb_height > 0 &&
        (g_SwapChainRebuild || g_MainWindowData.Width != fb_width ||
         g_MainWindowData.Height != fb_height)) {
      ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
      ImGui_ImplVulkanH_CreateOrResizeWindow(
          g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData,
          g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount);
      g_MainWindowData.FrameIndex = 0;
      g_SwapChainRebuild = false;
    }
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (app_state.chroma_crosshair) {
      float t = ImGui::GetTime();
      float r = sin(t * 2.0f) * 0.5f + 0.5f;
      float g = sin(t * 2.0f + 2.0f) * 0.5f + 0.5f;
      float b = sin(t * 2.0f + 4.0f) * 0.5f + 0.5f;
      app_state.crosshair_color = ImVec4(r, g, b, 1.0f);
    }

    if (app_state.draw_crosshair && ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("Designer")) {
        static const char *items[]{"Dot", "Cross"};
        (ImGui::Combo("Shape", &app_state.crosshair_index, items,
                      IM_ARRAYSIZE(items)));

        ImGui::SliderInt("Size", &app_state.crosshair_size, 1, 150);
        if (app_state.crosshair_index == 1)
          ImGui::SliderInt("Gap", &app_state.crosshair_gap, 0, 150);
        ImGui::ColorEdit4("Color", (float *)&app_state.crosshair_color);
        ImGui::Checkbox("Rainbow", &app_state.chroma_crosshair);
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings")) {
        ImGui::Checkbox("Render Crosshair", &app_state.draw_crosshair);
        if (ImGui::Button("Exit")) {
          exit(0);
        }
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    if (app_state.draw_crosshair) {
      ImVec2 center = ImGui::GetMainViewport()->GetCenter();
      ImDrawList *draw_list = ImGui::GetForegroundDrawList();

      unsigned int converted_crosshair_color =
          ImGui::ColorConvertFloat4ToU32(app_state.crosshair_color);
      switch (app_state.crosshair_index) {
      case 0: {
        draw_list->AddCircleFilled(center, app_state.crosshair_size,
                                   converted_crosshair_color);
        break;
      }
      case 1: {
        float cx = center.x + 0.5f;
        float cy = center.y + 0.5f;

        ImU32 col = converted_crosshair_color;

        draw_list->AddLine(
            {cx - app_state.crosshair_gap - app_state.crosshair_size, cy},
            {cx - app_state.crosshair_gap, cy}, col);

        draw_list->AddLine(
            {cx + app_state.crosshair_gap, cy},
            {cx + app_state.crosshair_gap + app_state.crosshair_size, cy}, col);

        draw_list->AddLine(
            {cx, cy - app_state.crosshair_gap - app_state.crosshair_size},
            {cx, cy - app_state.crosshair_gap}, col);

        draw_list->AddLine(
            {cx, cy + app_state.crosshair_gap},
            {cx, cy + app_state.crosshair_gap + app_state.crosshair_size}, col);
        break;
      }
      }
    }

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    const bool is_minimized =
        (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized) {
      wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
      wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
      wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
      wd->ClearValue.color.float32[3] = clear_color.w;
      FrameRender(wd, draw_data);
      FramePresent(wd);
    }
  }

  err = vkDeviceWaitIdle(g_Device);
  check_vk_result(err);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  CleanupVulkanWindow();
  CleanupVulkan();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
