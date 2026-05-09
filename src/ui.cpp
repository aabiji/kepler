#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <format>
#include <glad/glad.h>
#include <imgui.h>
#include <numbers>

#include "ui.h"

// Left aligned label, right aligned value
void labelled_value(std::string label, std::string value) {
  ImGui::Text("%s", label.c_str());

  float text_width = ImGui::CalcTextSize(value.c_str()).x;
  ImGui::SameLine(ImGui::GetWindowWidth() - text_width -
                  ImGui::GetStyle().WindowPadding.x);

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 0.0, 1.0));
  ImGui::Text("%s", value.c_str());
  ImGui::PopStyleColor();
}

UI::~UI() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

bool UI::active() { return ImGui::GetIO().WantCaptureMouse; }

void UI::init(GLFWwindow *window) {
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

void UI::render(Satellite satellite) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();

  // Right aligned window
  ImVec2 size = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(ImVec2(size.x - 10, 10), ImGuiCond_Always,
                          ImVec2(1.0f, 0.0f));
  ImGui::NewFrame();
  ImGui::Begin("Container");

  labelled_value(satellite.name, satellite.norad_id);
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
