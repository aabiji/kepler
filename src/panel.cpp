#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "misc.h"
#include "panel.h"

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

bool search_bar(std::string &value) {
  value = trim(value);
  float button_width =
      ImGui::CalcTextSize("GO").x + ImGui::GetStyle().FramePadding.x * 2.0f;
  ImGui::SetNextItemWidth(-button_width - ImGui::GetStyle().ItemSpacing.x);
  ImGui::InputTextWithHint("##input", "Search by satellite name or NORAD ID",
                           &value, ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::SameLine();
  return ImGui::Button("GO");
}

InfoPanel::~InfoPanel() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

bool InfoPanel::active() { return ImGui::GetIO().WantCaptureMouse; }

void InfoPanel::init(GLFWwindow *window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  float scale =
      ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF(font_path().c_str(), 18.0f);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 460");
}

bool InfoPanel::render(Satellite *satellite, std::string error) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x, 0.0f),
                          ImGuiCond_Always,
                          ImVec2(1.0f, 0.0f)); // Fix to the top right
  ImGui::SetNextWindowSize(ImVec2(350, satellite ? 175 : 55), ImGuiCond_Always);
  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  ImGui::Begin("Window", nullptr, flags);

  bool searching = search_bar(search_term);
  if (error.length() > 0) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
    ImGui::Text("%s", error.c_str());
    ImGui::PopStyleColor();
  }

  if (satellite != nullptr) {
    ImGui::Separator();
    ImGui::Text("%s", satellite->name.c_str());
    labelled_value("NORAD ID", satellite->norad_id);
    labelled_value("Inclination", std::format("{}°", satellite->inclination));
    labelled_value("Eccentricity", std::format("{}", satellite->eccentricity));
    labelled_value("Mean motion",
                   std::format("{} rev/day", satellite->mean_motion));
    labelled_value("Epoch", satellite->epoch);
  }

  ImGui::End();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  return searching;
}
