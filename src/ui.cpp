#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <format>
#include <glad/glad.h>
#include <imgui.h>

#include "ui.h"

// Left aligned label, right aligned value
void labelled_value(std::string label, std::string value) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83, 0.83, 0.83, 1.0));
  ImGui::Text("%s", label.c_str());
  ImGui::PopStyleColor();

  float text_width = ImGui::CalcTextSize(value.c_str()).x;
  ImGui::SameLine(ImGui::GetWindowWidth() - text_width -
                  ImGui::GetStyle().WindowPadding.x);

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 0.0, 1.0));
  ImGui::Text("%s", value.c_str());
  ImGui::PopStyleColor();
}

InfoUI::~InfoUI() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

bool InfoUI::active() { return ImGui::GetIO().WantCaptureMouse; }

void InfoUI::init(GLFWwindow *window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  float scale =
      ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF("../assets/Roboto-Regular.ttf", 18.0f);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 460");
}

void InfoUI::render(Satellite satellite) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x, 0.0f),
                          ImGuiCond_Always,
                          ImVec2(1.0f, 0.0f)); // Pivot to the top right
  ImGui::SetNextWindowSize(ImVec2(350, 150), ImGuiCond_Always);
  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  ImGui::Begin("Window", nullptr, flags);

  ImGui::Text("%s", satellite.name.c_str());
  labelled_value("NORAD ID", satellite.norad_id);
  labelled_value(
      "Inclination",
      std::format("{}°", satellite.inclination * (180.0 / std::numbers::pi)));
  labelled_value("Eccentricity", std::format("{}", satellite.eccentricity));
  labelled_value("Mean motion",
                 std::format("{} rev/day", satellite.mean_motion));
  labelled_value("Epoch", satellite.epoch);

  ImGui::End();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
