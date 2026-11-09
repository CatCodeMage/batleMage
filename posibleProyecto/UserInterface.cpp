#include "UserInterface.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "Paths.h"

constexpr const char* GLSL_VERSION = "#version 330 core";
constexpr bool INSTALL_GLFW_CALLBACKS = true;   // ImGui guarda los de Input y los sigue llamando

namespace {
	const char* const FONT_FILE = "fonts/Roboto-Medium.ttf";
	constexpr float FONT_SIZE_PIXELS = 16.0f;

	// Colores al estilo del editor erhe: gris oscuro, cabeceras azules, deslizadores verde azulado
	const ImVec4 PANEL_BACKGROUND(0.12f, 0.12f, 0.13f, 1.00f);
	const ImVec4 TITLE_BACKGROUND(0.15f, 0.16f, 0.18f, 1.00f);
	const ImVec4 FIELD_BACKGROUND(0.20f, 0.20f, 0.22f, 1.00f);
	const ImVec4 BUTTON_GREY(0.26f, 0.26f, 0.28f, 1.00f);
	const ImVec4 HEADER_BLUE(0.16f, 0.32f, 0.60f, 1.00f);
	const ImVec4 HEADER_BLUE_HOVER(0.22f, 0.40f, 0.72f, 1.00f);
	const ImVec4 ACCENT_BLUE(0.26f, 0.46f, 0.82f, 1.00f);
	const ImVec4 DOCKING_PREVIEW_BLUE(0.26f, 0.46f, 0.82f, 0.70f);   // semitransparente: se ve lo que hay debajo
	const ImVec4 SLIDER_TEAL(0.24f, 0.62f, 0.52f, 1.00f);
	const ImVec4 CHECK_GREEN(0.45f, 0.85f, 0.35f, 1.00f);
	const ImVec4 TEXT_LIGHT(0.90f, 0.90f, 0.92f, 1.00f);

	void applyBattleMageStyle() {
		ImGui::StyleColorsDark();   // base: solo se cambia lo que distingue al estilo
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Text] = TEXT_LIGHT;
		colors[ImGuiCol_WindowBg] = PANEL_BACKGROUND;
		colors[ImGuiCol_TitleBg] = TITLE_BACKGROUND;
		colors[ImGuiCol_TitleBgActive] = TITLE_BACKGROUND;
		colors[ImGuiCol_FrameBg] = FIELD_BACKGROUND;
		colors[ImGuiCol_Button] = BUTTON_GREY;
		colors[ImGuiCol_ButtonHovered] = HEADER_BLUE_HOVER;
		colors[ImGuiCol_ButtonActive] = ACCENT_BLUE;
		colors[ImGuiCol_Header] = HEADER_BLUE;
		colors[ImGuiCol_HeaderHovered] = HEADER_BLUE_HOVER;
		colors[ImGuiCol_HeaderActive] = ACCENT_BLUE;
		colors[ImGuiCol_Tab] = TITLE_BACKGROUND;
		colors[ImGuiCol_TabHovered] = HEADER_BLUE_HOVER;
		colors[ImGuiCol_TabSelected] = HEADER_BLUE;
		colors[ImGuiCol_SliderGrab] = SLIDER_TEAL;
		colors[ImGuiCol_SliderGrabActive] = CHECK_GREEN;
		colors[ImGuiCol_CheckMark] = CHECK_GREEN;
		colors[ImGuiCol_DockingPreview] = DOCKING_PREVIEW_BLUE;

		style.WindowRounding = 0.0f;
		style.FrameRounding = 2.0f;
		style.TabRounding = 2.0f;
		style.FramePadding = ImVec2(8.0f, 4.0f);
		style.ItemSpacing = ImVec2(8.0f, 5.0f);
	}

	void loadInterfaceFont() {
		const std::filesystem::path fontFile = dataPath(FONT_FILE);
		if (!std::filesystem::exists(fontFile))   // si no, ImGui solo da un assert sin decir que fichero falta
			throw std::runtime_error("UserInterface: no se encuentra la fuente " + fontFile.string());
		ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFile.string().c_str(), FONT_SIZE_PIXELS);
	}
}

void UserInterface::init(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	applyBattleMageStyle();
	loadInterfaceFont();
	ImGui_ImplGlfw_InitForOpenGL(window, INSTALL_GLFW_CALLBACKS);
	ImGui_ImplOpenGL3_Init(GLSL_VERSION);
}

void UserInterface::beginFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	// el centro queda transparente: se ve la escena
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);   
}

void UserInterface::endFrame() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UserInterface::destroy() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}