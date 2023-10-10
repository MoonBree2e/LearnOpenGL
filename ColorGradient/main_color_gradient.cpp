#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ImGradientHDR.h"

#include <string>
#include <shader.h>

/**
 * DebugGroup
 */
class DebugGroupOverrride {
public:
	DebugGroupOverrride(const std::string& message) {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, message.c_str());
	}
	~DebugGroupOverrride() {
		Pop();
	}
	void Pop() {
		glPopDebugGroup();
	}
};


static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		return 1;
	}

	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 460 core";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

	GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGradient", NULL, NULL);
	if (window == NULL)
	{
		return 1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		return -1;
	}

	IMGUI_CHECKVERSION();
	auto gui_context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	int32_t stateID = 10;

	ImGradientState state;
	ImGradientTemporaryState tempState;

	state.AddColorMarker(0.0f, { 1.0f, 1.0f, 1.0f }, 1.0f);
	state.AddColorMarker(1.0f, { 1.0f, 1.0f, 1.0f }, 1.0f);
	state.AddAlphaMarker(0.0f, 1.0f);
	state.AddAlphaMarker(1.0f, 1.0f);

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glObjectLabel(GL_FRAMEBUFFER, framebuffer, -1, "framebuffer");

	int f_width = 1280;
	int f_height = 720;
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, f_width, f_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glObjectLabel(GL_TEXTURE, texture, -1, "texture");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	//unsigned int RBO;
	//glGenRenderbuffers(1, &RBO);
	//glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, f_height, f_width);
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	unsigned int texture2;
	glGenTextures(1, &texture2);
	glBindTexture(GL_TEXTURE_2D, texture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	

	float tri[] = { -1.0f, -1.0f,
					3.0f, -1.0f,
					-1.0f, 3.0f };
	
	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri), &tri, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glBindVertexArray(0);

	Shader shader("color_gradient.vert", "color_gradient.frag");

	struct ControlPoint {
		glm::vec4 position;
		glm::vec4 color;
	};

	int numPoints = 3;

	std::vector<ControlPoint> controlPointsData(16);

	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ControlPoint) * numPoints, controlPointsData.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo); // 绑定到binding=0的位置

	GLuint ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(int), &numPoints, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo); // 绑定到binding=1的位置



	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

#if 0
		{
			DebugGroupOverrride d("framebuffer");

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, f_width, f_height);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			{
				ImVec2 size(200, 50); // 渐变矩形的大小
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				
				//ImVec2 p0 = ImGui::GetCursorScreenPos(); // 渐变矩形的起点坐标
				ImVec2 p0 = ImVec2(0, 0);
				ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y); // 渐变矩形的终点坐标

				// 自定义渐变颜色
				ImU32 colors[] = { IM_COL32(255, 0, 0, 255), IM_COL32(0, 255, 0, 255), IM_COL32(0, 0, 255, 255) };

				draw_list->AddRectFilledMultiColor(p0, p1, colors[0], colors[1], colors[2], colors[2]);
			}
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
#endif
		{
			DebugGroupOverrride d("framebuffer");

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, f_width, f_height);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);

			shader.use();
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			shader.unUse();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}


		{
			numPoints = state.ColorCount;

			for (int i = 0; i < numPoints; i++) {
				auto& c = state.Colors[i].Color;
				controlPointsData[i] = ControlPoint(
					glm::vec4(state.Colors[i].Position),
					glm::vec4(c[0], c[1], c[2], 1)
				);
			}

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ControlPoint) * numPoints, controlPointsData.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_UNIFORM_BUFFER, ubo);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(int), &numPoints, GL_STATIC_DRAW);
		}

		// update cpu_color_ramp
		{
			int color_ramp_len = 256;
			std::vector<unsigned char> cpu_color_ramp(color_ramp_len * 4);
			for (int i = 0; i < color_ramp_len ; i++) {
				float p = 1.0f * i / (color_ramp_len - 1);
				std::array<float, 4> c = state.GetCombinedColor(p);
				int a = 1;
				for (int j = 0; j < 4; ++j) {
					int r = (int)std::round(c[j] * 255);
					r = std::clamp(r, 0, 255);
					char color = static_cast<unsigned char>((int)std::round(r));
					cpu_color_ramp[j + i * 4] = color;
				}
			}
			glBindTexture(GL_TEXTURE_2D, texture2);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, cpu_color_ramp.data());
		}

		//{
		//	const int color_ramp_len = 256;
		//	std::array<float, 4> textureData[color_ramp_len];
		//	for (int i = 0; i < color_ramp_len; i++) {
		//		float x = static_cast<float>(i) / static_cast<float>(color_ramp_len - 1);
		//		textureData[i] = state.GetCombinedColor(x);
		//	}

		//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());

		//}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		{
			DebugGroupOverrride d("imgui draw");

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			{
				ImGui::Begin("ImGradient");
				// 自定义标题栏
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // 背景颜色
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // 鼠标悬停时的背景颜色
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 文本颜色
				ImGui::PushID(0);

				bool isHeaderOpen = ImGui::CollapsingHeader("##MyHeader", ImGuiTreeNodeFlags_None);
				if (isHeaderOpen) {
					ImGui::Text("This is some text inside the custom header");
					//ImGui::Image((void*)yourTextureID, ImVec2(16, 16)); // 替换 yourTextureID 和图像大小
				}

				ImGui::PopID();
				ImGui::PopStyleColor(3); // 弹出之前推入的三个样式颜色

				static ImVec4 color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);


				if (ImGui::CollapsingHeader("Color")) {
					float availableWidth = ImGui::GetContentRegionAvail().x;
					float buttonWidth = availableWidth * 0.5f;

					float f;
					// color button
					{
						//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
						//ImGui::ColorEdit4("MyColor##3", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
						
						ImGui::PushID("color button");
						ImGui::Text("Color"); ImGui::SameLine(buttonWidth); bool open_popup = ImGui::ColorButton("button", color, 0, ImVec2(buttonWidth, 0));
						static ImVec4 backup_color;
						
						if (open_popup)
						{
							ImGui::OpenPopup("mypicker");
							backup_color = color;
						}

						if (ImGui::BeginPopup("mypicker"))
						{
							ImGui::ColorPicker4("##picker", (float*)&color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
							ImGui::SameLine();

							ImGui::BeginGroup(); // Lock X position
							ImGui::Text("Current");
							ImGui::ColorButton("##current", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
							ImGui::Text("Previous");
							if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
								color = backup_color;
							ImGui::EndGroup();

							ImGui::EndPopup();
						}
						ImGui::PopID();
					
						//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
						//ImGui::ColorEdit4("MyColor##3", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
					}
					
					// color ramp
					{

						ImGui::Text("Color ramp"); ImGui::SameLine(buttonWidth); 
						static bool show_color_ramp = false;

						if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(texture2), ImVec2(buttonWidth, ImGui::GetTextLineHeightWithSpacing()), { 0, 1 }, { 1, 0 })) {
							show_color_ramp = !show_color_ramp;
						}
						
						if (show_color_ramp) {
							ImGui::Indent();

							// gradient
							{

								bool isMarkerShown = true;
								ImGradient(stateID, state, tempState, isMarkerShown);

								if (ImGui::IsItemHovered())
								{
									ImGui::SetTooltip("Gradient");
								}

								if (tempState.selectedMarkerType == ImGradientMarkerType::Color)
								{
									auto selectedColorMarker = state.GetColorMarker(tempState.selectedIndex);
									if (selectedColorMarker != nullptr)
									{
										ImGui::ColorEdit3("Color", selectedColorMarker->Color.data(), ImGuiColorEditFlags_Float);
										ImGui::DragFloat("Intensity", &selectedColorMarker->Intensity, 0.1f, 0.0f, 100.0f, "%f", 1.0f);
									}
								}

								if (tempState.selectedMarkerType == ImGradientMarkerType::Alpha)
								{
									auto selectedAlphaMarker = state.GetAlphaMarker(tempState.selectedIndex);
									if (selectedAlphaMarker != nullptr)
									{
										ImGui::DragFloat("Alpha", &selectedAlphaMarker->Alpha, 0.1f, 0.0f, 1.0f, "%f", 1.0f);
									}
								}

								if (tempState.selectedMarkerType != ImGradientMarkerType::Unknown)
								{
									if (ImGui::Button("Delete"))
									{
										if (tempState.selectedMarkerType == ImGradientMarkerType::Color)
										{
											state.RemoveColorMarker(tempState.selectedIndex);
											tempState = ImGradientTemporaryState{};
										}
										else if (tempState.selectedMarkerType == ImGradientMarkerType::Alpha)
										{
											state.RemoveAlphaMarker(tempState.selectedIndex);
											tempState = ImGradientTemporaryState{};
										}
									}
								}
							}
							ImGui::Unindent();

						}

					}
				}


				ImGui::End();
			}

#if 0
			{
				ImGui::Begin("Image Dropdown Demo");

				// 一个用于存储下拉框中可选项的字符串数组
				const char* items[] = { "Option 1", "Option 2", "Option 3" };

				static int selectedItem = -1; // 当前选中的项（-1 表示未选中）

				if (ImGui::BeginCombo("##combo", (selectedItem == -1) ? "Select an option" : items[selectedItem])) {
					for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
						bool isSelected = (selectedItem == i);

						if (ImGui::Selectable(items[i], isSelected)) {
							selectedItem = i;
						}

						if (isSelected) {
							// 在选中的项后面绘制一个图像
							ImGui::SameLine();
							ImGui::Image(ImGui::GetIO().Fonts->TexID, ImVec2(16, 16)); // 你可以替换为自己的图像
						}
					}

					ImGui::EndCombo();
				}

				ImGui::End();
			}
#endif

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}