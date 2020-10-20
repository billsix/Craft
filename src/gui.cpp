#include "gl3w.h"

#include "tinycthread.h"

#include <GLFW/glfw3.h>

#include "imgui.h"

#include "gui.h"

#include "config.h"

#include "sign.h"

#include "map.h"

#include "main.h"

extern Model *g;

// Our state
bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


void gui_show_demo_window() {
  ImGui::NewFrame();

  {

    ImGui::Begin("Craft");
    if (ImGui::Button("Resume")) {
      escape_pressed = false;
      glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Options", tab_bar_flags))
      {
        if (ImGui::BeginTabItem("Rendering Suppression"))
          {
            ImGui::Checkbox(
                            "Render Chunks",
                            &do_render_chunks);
            ImGui::Checkbox(
                            "Render Signs",
                            &do_render_signs);

            ImGui::Checkbox(
                            "Render Sky",
                            &do_render_sky);
            ImGui::Checkbox(
                            "Render Wireframe",
                            &do_render_wireframe);
            ImGui::Checkbox(
                            "Render Text",
                            &do_render_text);
            if (ImGui::TreeNode("Items in bottom left"))
              {
                ImGui::Checkbox(
                                "Render Item",
                                &do_render_item);
                ImGui::Checkbox(
                                "Render Plant Item",
                                &do_render_plant);
                ImGui::Checkbox(
                                "Render Cube Item",
                                &do_render_cube);

                ImGui::TreePop();
              }

            if(!do_render_item)
              {
                do_render_plant = false;
                do_render_cube = false;
              }
            ImGui::Checkbox(
                            "Render Crosshairs",
                            &do_render_crosshairs);

            ImGui::EndTabItem();
          }
        ImGui::EndTabBar();
      }


    ImGui::End();
  }

  // 1. Show the big demo window (Most of the sample code is in
  // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
  // ImGui!).
  if (show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair
  // to created a named window.
  {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and
                                   // append into it.

    ImGui::Text("This is some useful text."); // Display some text (you can use
                                              // a format strings too)
    ImGui::Checkbox(
        "Demo Window",
        &show_demo_window); // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f,
                       1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3(
        "clear color",
        (float *)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button")) // Buttons return true when clicked (most
                                 // widgets return true when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }

  // 3. Show another simple window.
  if (show_another_window) {
    ImGui::Begin(
        "Another Window",
        &show_another_window); // Pass a pointer to our bool variable (the
                               // window will have a closing button that will
                               // clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me"))
      show_another_window = false;
    ImGui::End();
  }
}
