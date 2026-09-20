#include "gui.h"
#include "Scope.h"
#include "blur.h"
#include "checker.h"
#include "fonts.h"
#include "images.h"
#include "info.h"
#include "src/Headers/Auth.h"
#include "temp.h"
#include <atomic>
#include <exception>
#include <thread>


static char licenseKey[256] = {};

const char *kProductHash =
    "b0377912f5d549859546816a47064863ca75873a20c95ff673d4f7518ffc654c";

float lerp(float a, float b, float t) { return a + t * (b - a); }

std::string to_lower(const std::string &str) {
  std::string lower_str = str;
  std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return lower_str;
}

void ui::move_window() {
  static int last_width = static_cast<int>(window::size_max.x);
  static int last_height = static_cast<int>(window::size_max.y);

  RECT rc;
  GetWindowRect(hwnd, &rc);

  int current_width = rc.right - rc.left;
  int current_height = rc.bottom - rc.top;

  int center_x = rc.left + (current_width / 2);
  int center_y = rc.top + (current_height / 2);

  int new_width = static_cast<int>(std::round(window::size_max.x));
  int new_height = static_cast<int>(std::round(window::size_max.y));

  int new_left = center_x - (new_width / 2);
  int new_top = center_y - (new_height / 2);

  ImVec2 imguiPos = ImGui::GetWindowPos();
  new_left += static_cast<int>(std::round(imguiPos.x));
  new_top += static_cast<int>(std::round(imguiPos.y));

  MoveWindow(hwnd, new_left, new_top, new_width, new_height, TRUE);

  last_width = new_width;
  last_height = new_height;

  ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
}

void ui::resize(ImVec2 &size, const ImVec2 &target) {
  size.x = ImLerp(size.x, target.x, window::speed * ImGui::GetIO().DeltaTime);
  size.y = ImLerp(size.y, target.y, window::speed * ImGui::GetIO().DeltaTime);
}

void ui::initialize_fonts() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.IniFilename = NULL;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImFontConfig spacegrotesk_cfg;
  spacegrotesk_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint |
                                      ImGuiFreeTypeBuilderFlags_LightHinting |
                                      ImGuiFreeTypeBuilderFlags_LoadColor;
  spacegrotesk_cfg.GlyphExtraSpacing.x = -1.0f;
  // cfg.GlyphOffset.y = 1.0f;
  ImFontConfig cfg;

  static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.OversampleH = 1;
  icons_config.OversampleV = 1;
  font.spacegrotesk_medium[0] = io.Fonts->AddFontFromMemoryTTF(
      spacegrotesk_medium, sizeof(spacegrotesk_medium), 24.0f,
      &spacegrotesk_cfg, io.Fonts->GetGlyphRangesCyrillic());
  font.spacegrotesk_medium[1] = io.Fonts->AddFontFromMemoryTTF(
      spacegrotesk_medium, sizeof(spacegrotesk_medium), 16.0f,
      &spacegrotesk_cfg, io.Fonts->GetGlyphRangesCyrillic());
  font.spacegrotesk_medium[2] = io.Fonts->AddFontFromMemoryTTF(
      spacegrotesk_medium, sizeof(spacegrotesk_medium), 20.0f,
      &spacegrotesk_cfg, io.Fonts->GetGlyphRangesCyrillic());

  font.poppins_medium = io.Fonts->AddFontFromMemoryTTF(
      poppins_medium, sizeof(poppins_medium), 14.f, &cfg,
      io.Fonts->GetGlyphRangesCyrillic());
  font.notify_font =
      io.Fonts->AddFontFromMemoryTTF(notify_font, sizeof(notify_font), 14, &cfg,
                                     io.Fonts->GetGlyphRangesCyrillic());
  font.tab_icon = io.Fonts->AddFontFromMemoryTTF(
      tab_icon, sizeof(tab_icon), 14, &cfg, io.Fonts->GetGlyphRangesCyrillic());
  font.button_icon =
      io.Fonts->AddFontFromMemoryTTF(button_icon, sizeof(button_icon), 14, &cfg,
                                     io.Fonts->GetGlyphRangesCyrillic());
  font.font_awesome = io.Fonts->AddFontFromMemoryCompressedTTF(
      fa6_solid_compressed_data, fa6_solid_compressed_size, 18.0f,
      &icons_config, icons_ranges);
}

void ui::initialize_images() {
  D3DX11_IMAGE_LOAD_INFO info;
  ID3DX11ThreadPump *pump{nullptr};

  // Load Vanity logo from embedded data
  #include "logo_data.h"

  HRESULT hr = D3DX11CreateShaderResourceViewFromMemory(
      g_pd3dDevice, logo_png_data, logo_png_size,
      nullptr, nullptr, &images::logo_texture, nullptr);

  if (SUCCEEDED(hr) && images::logo_texture) {
    ID3D11Resource *resource = nullptr;
    images::logo_texture->GetResource(&resource);
    if (resource) {
      ID3D11Texture2D *tex = nullptr;
      resource->QueryInterface(&tex);
      if (tex) {
        D3D11_TEXTURE2D_DESC desc;
        tex->GetDesc(&desc);
        images::logo_width = desc.Width;
        images::logo_height = desc.Height;
        tex->Release();
      }
      resource->Release();
    }
  }
}

struct Notification {
  std::string message;
  std::string icon;
  float alpha = 0.0f;
  float timer = 0.0f;
  bool fading_out = false;
  float duration = 3.0f;
  float target_y = 0.0f;
  float current_y = 0.0f;
  ImVec4 bg_color = ImColor(13, 14, 15);
  ImVec4 icon_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
};

std::vector<Notification> notifications;

void ui::add_notify(const std::string &icon, const std::string &msg,
                    const ImVec4 &icon_color) {
  notifications.push_back({msg, icon, 0.0f, 0.0f, false, 3.0f, 0.0f, 0.0f,
                           ImVec4(0.051f, 0.055f, 0.059f, 1.0f), icon_color});
}

void ui::render_notify() {
  const float padding = 14.0f;
  const ImVec2 viewport_pos = ImGui::GetMainViewport()->WorkPos;
  const ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
  float total_offset = 0.0f;

  for (int i = 0; i < notifications.size(); ++i) {
    Notification &note = notifications[i];
    float delta = ImGui::GetIO().DeltaTime;
    note.timer += delta;

    if (!note.fading_out) {
      note.alpha = ImClamp(note.alpha + delta * 3.0f, 0.0f, 1.0f);
      if (note.timer >= note.duration)
        note.fading_out = true;
    } else {
      note.alpha = ImClamp(note.alpha - delta * 2.0f, 0.0f, 1.0f);
    }

    if (note.fading_out && note.alpha <= 0.0f) {
      notifications.erase(notifications.begin() + i);
      --i;
      continue;
    }

    ImGui::PushFont(font.notify_font);
    ImVec2 icon_size = ImGui::CalcTextSize(note.icon.c_str());
    ImGui::PopFont();

    ImGui::PushFont(font.poppins_medium);
    ImVec2 text_size = ImGui::CalcTextSize(note.message.c_str());
    ImGui::PopFont();

    ImVec2 box_size(icon_size.x + text_size.x + 30.0f,
                    max(icon_size.y, text_size.y) + 10.0f);

    float target_y = viewport_pos.y + padding + total_offset;
    if (note.target_y == 0.0f)
      note.target_y = target_y;

    note.target_y = target_y;
    note.current_y += (note.target_y - note.current_y) * delta * 10.0f;

    ImVec2 pos(viewport_pos.x + padding, note.current_y);

    ImDrawList *draw = ImGui::GetForegroundDrawList();
    draw->AddRectFilled(
        ImVec2(pos.x, pos.y), ImVec2(pos.x + box_size.x, pos.y + box_size.y),
        ImColor(note.bg_color.x, note.bg_color.y, note.bg_color.z, note.alpha),
        6.0f);

    ImGui::PushFont(font.notify_font);
    draw->AddText(ImVec2(pos.x + 9, pos.y + 6),
                  ImColor(note.icon_color.x, note.icon_color.y,
                          note.icon_color.z, note.alpha),
                  note.icon.c_str());
    ImGui::PopFont();

    ImGui::PushFont(font.poppins_medium);
    draw->AddText(ImVec2(pos.x + 15 + icon_size.x, pos.y + 6),
                  ImColor(1.0f, 1.0f, 1.0f, note.alpha), note.message.c_str());
    ImGui::PopFont();

    total_offset += box_size.y + 5.0f;
  }
}

void ui::render_background(ImDrawList *drawlist) {
  ImVec2 pos(0, 0);
  ImVec2 size(pos.x + window::size_max.x, pos.y + window::size_max.y);

  drawlist->AddRectFilled(pos, size,
                          IM_COL32(ui::colors::loader::background.x * 255,
                                   ui::colors::loader::background.y * 255,
                                   ui::colors::loader::background.z * 255,
                                   ui::alpha::background * 255),
                          window::rounding, ImDrawFlags_RoundCornersAll);

  const float spacing = 20.0f;
  const float outer_radius = 3.0f;
  const float inner_radius = 1.5f;

  ImVec4 base_color = ui::colors::loader::main;
  ImVec4 background_color = ui::colors::loader::background;

  ImVec2 center = ImVec2(pos.x + size.x / 2.f, pos.y + size.y / 2.f);
  float max_dist =
      sqrtf((size.x / 2.f) * (size.x / 2.f) + (size.y / 2.f) * (size.y / 2.f));

  float time = ImGui::GetTime();

  for (float y = pos.y; y < pos.y + size.y; y += spacing) {
    for (float x = pos.x; x < pos.x + size.x; x += spacing) {
      float dist = sqrtf((x - center.x) * (x - center.x) +
                         (y - center.y) * (y - center.y));

      float base_alpha = 0.1f * (1.0f - dist / max_dist);
      if (base_alpha < 0.0f)
        base_alpha = 0.0f;

      float alpha =
          base_alpha * (0.5f + 0.5f * sinf(time * 2.0f + (x + y) * 0.05f));

      if (alpha > 0.005f) {
        ImU32 col_outer = ImGui::GetColorU32(
            ImVec4(base_color.x, base_color.y, base_color.z, alpha));
        ImU32 col_inner = ImGui::GetColorU32(ImVec4(
            background_color.x, background_color.y, background_color.z, alpha));

        drawlist->AddCircleFilled(ImVec2(x, y), outer_radius, col_outer);

        drawlist->AddCircleFilled(ImVec2(x, y), inner_radius, col_inner);
      }
    }
  }

  ImVec2 obj_center =
      ImVec2(window::size_max.x / 2 - 3, window::size_max.y + 5);
  ImU32 shadow_col = IM_COL32(
      ui::colors::loader::main.x * 255, ui::colors::loader::main.y * 255,
      ui::colors::loader::main.z * 255, ui::alpha::background * 150);
  ImVec2 shadow_offset = ImVec2(10.0f, 10.0f);
  ImDrawFlags flags = ImDrawFlags_RoundCornersAll;
  int num_segments = 32;

  drawlist->AddShadowCircle(obj_center, 16, shadow_col, 681, shadow_offset,
                            flags, num_segments);

  drawlist->AddRect(pos, size,
                    IM_COL32(ui::colors::loader::outline.x * 255,
                             ui::colors::loader::outline.y * 255,
                             ui::colors::loader::outline.z * 255,
                             ui::alpha::background * 255),
                    window::rounding - 1, ImDrawFlags_RoundCornersAll);
}

void ui::render_login(ImDrawList *drawlist) {
  ImVec2 pos(0, 0);
  ImVec2 size(pos.x + window::size_max.x, pos.y + window::size_max.y);
  float fade_speed = 1.0f;

  static std::atomic<bool> isLoggingIn{false};
  static std::atomic<bool> loginCompleted{false};
  static std::atomic<bool> loginSuccess{false};
  static std::string loginErrorMessage;

  if (!ui::variables::logged_in) {
    ui::variables::login_fade_timer += ImGui::GetIO().DeltaTime * fade_speed;
    if (ui::variables::login_fade_timer > 1.0f)
      ui::variables::login_fade_timer = 1.0f;
  } else {
    ui::variables::login_fade_timer -= ImGui::GetIO().DeltaTime * fade_speed;
    if (ui::variables::login_fade_timer < 0.0f)
      ui::variables::login_fade_timer = 0.0f;
  }

  ui::variables::login_screen_fully_hidden =
      (ui::variables::login_fade_timer <= 0.0f);
  ui::variables::time_base = ui::variables::login_fade_timer;

  if (ui::variables::login_fade_timer <= 0.0f)
    return;

  float alpha = ui::variables::login_fade_timer;
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

  // Layout constants
  float left_width = size.x * 0.55f;
  float padding = 40.0f;
  float start_x = padding;
  float start_y = 15.0f;

  // ============================================================
  // Brand Title: "Vanity" (red) + "Spoofer" (white)
  // ============================================================
  ImGui::PushFont(font.spacegrotesk_medium[0]);
  const char *brand_red = "Vanity";
  const char *brand_white = "Spoofer";
  ImVec2 brand_red_size = ImGui::CalcTextSize(brand_red);
  ImVec2 brand_white_size = ImGui::CalcTextSize(brand_white);

  float brand_y = start_y + 10.0f;
  drawlist->AddText(ImVec2(start_x, brand_y),
                    IM_COL32(255, 0, 0, (int)(alpha * 255)), brand_red);
  drawlist->AddText(ImVec2(start_x + brand_red_size.x + 2, brand_y),
                    IM_COL32(255, 255, 255, (int)(alpha * 255)), brand_white);
  ImGui::PopFont();

  // ============================================================
  // Tabs: Login / Register
  // ============================================================
  float tab_y = brand_y + brand_red_size.y + 20.0f;
  float tab_width = 100.0f;
  float tab_height = 32.0f;
  float tab_spacing = 4.0f;
  float tab_container_width = tab_width * 2 + tab_spacing;

  // Tab container background
  ImVec2 tab_bg_min(start_x, tab_y);
  ImVec2 tab_bg_max(start_x + tab_container_width, tab_y + tab_height);
  drawlist->AddRectFilled(tab_bg_min, tab_bg_max,
                          IM_COL32(37, 37, 37, (int)(alpha * 255)), 6.0f);

  ImGui::PushFont(font.spacegrotesk_medium[1]);

  // Login tab
  {
    ImVec2 tab_min(start_x + 2, tab_y + 2);
    ImVec2 tab_max(start_x + tab_width, tab_y + tab_height - 2);
    bool is_active = (ui::variables::login_tab == 0);

    if (is_active) {
      drawlist->AddRectFilled(tab_min, tab_max,
                              IM_COL32(26, 26, 26, (int)(alpha * 255)), 5.0f);
      // Red underline
      drawlist->AddRectFilled(
          ImVec2(tab_min.x + (tab_width - 40) * 0.5f, tab_max.y - 2),
          ImVec2(tab_min.x + (tab_width + 40) * 0.5f, tab_max.y),
          IM_COL32(255, 0, 0, (int)(alpha * 255)));
    }

    const char *login_text = "Login";
    ImVec2 text_size = ImGui::CalcTextSize(login_text);
    ImVec2 text_pos(tab_min.x + (tab_width - text_size.x) * 0.5f,
                    tab_min.y + (tab_height - 4 - text_size.y) * 0.5f);
    ImU32 text_col = is_active ? IM_COL32(255, 255, 255, (int)(alpha * 255))
                               : IM_COL32(136, 136, 136, (int)(alpha * 255));
    drawlist->AddText(text_pos, text_col, login_text);

    // Click detection
    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (mouse.x >= tab_min.x && mouse.x <= tab_max.x &&
        mouse.y >= tab_min.y && mouse.y <= tab_max.y) {
      if (ImGui::IsMouseClicked(0))
        ui::variables::login_tab = 0;
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
  }

  // Register tab
  {
    ImVec2 tab_min(start_x + tab_width + tab_spacing, tab_y + 2);
    ImVec2 tab_max(start_x + tab_container_width - 2, tab_y + tab_height - 2);
    bool is_active = (ui::variables::login_tab == 1);

    if (is_active) {
      drawlist->AddRectFilled(tab_min, tab_max,
                              IM_COL32(26, 26, 26, (int)(alpha * 255)), 5.0f);
      drawlist->AddRectFilled(
          ImVec2(tab_min.x + (tab_width - 40) * 0.5f, tab_max.y - 2),
          ImVec2(tab_min.x + (tab_width + 40) * 0.5f, tab_max.y),
          IM_COL32(255, 0, 0, (int)(alpha * 255)));
    }

    const char *reg_text = "Register";
    ImVec2 text_size = ImGui::CalcTextSize(reg_text);
    ImVec2 text_pos(tab_min.x + (tab_width - tab_spacing - text_size.x) * 0.5f,
                    tab_min.y + (tab_height - 4 - text_size.y) * 0.5f);
    ImU32 text_col = is_active ? IM_COL32(255, 255, 255, (int)(alpha * 255))
                               : IM_COL32(136, 136, 136, (int)(alpha * 255));
    drawlist->AddText(text_pos, text_col, reg_text);

    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (mouse.x >= tab_min.x && mouse.x <= tab_max.x &&
        mouse.y >= tab_min.y && mouse.y <= tab_max.y) {
      if (ImGui::IsMouseClicked(0))
        ui::variables::login_tab = 1;
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
  }
  ImGui::PopFont();

  // ============================================================
  // Input Fields
  // ============================================================
  float input_y = tab_y + tab_height + 20.0f;
  float input_width = left_width - padding * 2;
  float input_height_val = 42.0f;
  float input_gap = 12.0f;
  ImVec2 input_size(input_width, input_height_val);

  if (ui::variables::logged_in)
    ImGui::BeginDisabled();

  // Field 1: Username
  ImVec2 username_pos(start_x, input_y);
  ui::items::input_text("Username", username_pos, input_size,
                        ::Auth.DiscordID, sizeof(::Auth.DiscordID),
                        ImGuiInputTextFlags_None);

  // Field 2: Password
  ImVec2 password_pos(start_x, input_y + input_height_val + input_gap);
  ui::items::input_text("Password", password_pos, input_size,
                        ::Auth.Password, sizeof(::Auth.Password),
                        ImGuiInputTextFlags_Password);

  // Field 3: License Key (Register tab only)
  ImVec2 license_pos(start_x, input_y + (input_height_val + input_gap) * 2);
  if (ui::variables::login_tab == 1) {
    ui::items::input_text("License Key", license_pos, input_size,
                          ::Auth.Senha, sizeof(::Auth.Senha),
                          ImGuiInputTextFlags_None);
  }

  // ============================================================
  // Remember Me Checkbox (only on Login tab)
  // ============================================================
  float last_field_bottom = (ui::variables::login_tab == 0)
                                ? password_pos.y + input_height_val
                                : license_pos.y + input_height_val;

  if (ui::variables::login_tab == 0) {
    float checkbox_y = last_field_bottom + 16.0f;
    float checkbox_size = 18.0f;
    ImVec2 cb_min(start_x, checkbox_y);
    ImVec2 cb_max(start_x + checkbox_size, checkbox_y + checkbox_size);

    // Checkbox background
    ImU32 cb_bg = ui::variables::remember_me
                      ? IM_COL32(255, 0, 0, (int)(alpha * 255))
                      : IM_COL32(26, 26, 26, (int)(alpha * 255));
    ImU32 cb_border = ui::variables::remember_me
                          ? IM_COL32(255, 0, 0, (int)(alpha * 255))
                          : IM_COL32(51, 51, 51, (int)(alpha * 255));
    drawlist->AddRectFilled(cb_min, cb_max, cb_bg, 4.0f);
    drawlist->AddRect(cb_min, cb_max, cb_border, 4.0f);

    // Checkmark
    if (ui::variables::remember_me) {
      float cx = cb_min.x + checkbox_size * 0.5f;
      float cy = cb_min.y + checkbox_size * 0.5f;
      drawlist->AddLine(ImVec2(cx - 4, cy), ImVec2(cx - 1, cy + 3),
                        IM_COL32(255, 255, 255, (int)(alpha * 255)), 2.0f);
      drawlist->AddLine(ImVec2(cx - 1, cy + 3), ImVec2(cx + 5, cy - 3),
                        IM_COL32(255, 255, 255, (int)(alpha * 255)), 2.0f);
    }

    // Click detection
    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (mouse.x >= cb_min.x && mouse.x <= cb_max.x + 100 &&
        mouse.y >= cb_min.y && mouse.y <= cb_max.y) {
      if (ImGui::IsMouseClicked(0)) {
        ui::variables::remember_me = !ui::variables::remember_me;
        SaveConfig();
      }
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    // Label
    ImGui::PushFont(font.spacegrotesk_medium[1]);
    drawlist->AddText(ImVec2(cb_max.x + 10, checkbox_y + 1),
                      IM_COL32(255, 255, 255, (int)(alpha * 255)),
                      "Remember Me");
    ImGui::PopFont();
  }

  // ============================================================
  // Submit Button
  // ============================================================
  float button_y = (ui::variables::login_tab == 0)
                       ? last_field_bottom + 60.0f
                       : last_field_bottom + 20.0f;
  float button_height_val = 44.0f;
  ImVec2 btn_min(start_x, button_y);
  ImVec2 btn_max(start_x + input_width, button_y + button_height_val);

  // Button hover/press state
  ImVec2 mouse = ImGui::GetIO().MousePos;
  bool btn_hovered = (mouse.x >= btn_min.x && mouse.x <= btn_max.x &&
                      mouse.y >= btn_min.y && mouse.y <= btn_max.y);
  bool btn_clicked = btn_hovered && ImGui::IsMouseClicked(0);

  ImU32 btn_color = btn_hovered ? IM_COL32(212, 0, 0, (int)(alpha * 255))
                                : IM_COL32(255, 0, 0, (int)(alpha * 255));
  drawlist->AddRectFilled(btn_min, btn_max, btn_color, 8.0f);

  // Button glow shadow
  drawlist->AddShadowRect(btn_min, btn_max,
                          IM_COL32(255, 0, 0, (int)(alpha * 60)), 15.0f,
                          ImVec2(0, 0), ImDrawFlags_RoundCornersAll, 8.0f);

  // Button text
  const char *btn_text =
      (ui::variables::login_tab == 0) ? "Login" : "Register";
  ImGui::PushFont(font.spacegrotesk_medium[1]);
  ImVec2 btn_text_size = ImGui::CalcTextSize(btn_text);
  ImVec2 btn_text_pos(btn_min.x + (input_width - btn_text_size.x) * 0.5f,
                      btn_min.y + (button_height_val - btn_text_size.y) * 0.5f);
  drawlist->AddText(btn_text_pos,
                    IM_COL32(255, 255, 255, (int)(alpha * 255)), btn_text);
  ImGui::PopFont();

  if (btn_hovered)
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

  // Button action
  if (btn_clicked && !isLoggingIn.load()) {
    if (ui::variables::login_tab == 0) {
      // LOGIN: username + password
      if (strlen(::Auth.DiscordID) > 0 && strlen(::Auth.Password) > 0) {
        isLoggingIn = true;
        loginCompleted = false;
        std::thread([hwid = GenerateHWID()]() {
          std::string error_message;
          bool success_result =
              PerformLogin(::Auth.DiscordID, ::Auth.Password, hwid, error_message,
                           "");
          loginSuccess = success_result;
          loginErrorMessage = error_message;
          loginCompleted = true;
        }).detach();
      } else {
        ui::add_notify("D", "Please enter username and password",
                       ImVec4(0.99f, 0.60f, 0.60f, 1.0f));
      }
    } else {
      // REGISTER: username + password + license key
      if (strlen(::Auth.DiscordID) > 0 && strlen(::Auth.Password) > 0 &&
          strlen(::Auth.Senha) > 0) {
        isLoggingIn = true;
        loginCompleted = false;
        std::thread([hwid = GenerateHWID()]() {
          std::string error_message;
          bool success_result =
              PerformRegister(::Auth.DiscordID, ::Auth.Password, ::Auth.Senha,
                              hwid, error_message);
          loginSuccess = success_result;
          loginErrorMessage = error_message;
          loginCompleted = true;
        }).detach();
      } else {
        ui::add_notify("D", "Please fill all fields",
                       ImVec4(0.99f, 0.60f, 0.60f, 1.0f));
      }
    }
  }

  if (ui::variables::logged_in)
    ImGui::EndDisabled();

  // ============================================================
  // Logo on the right side
  // ============================================================
  if (images::logo_texture) {
    float logo_max_size = 180.0f;
    float aspect = (images::logo_height > 0)
                       ? (float)images::logo_width / (float)images::logo_height
                       : 1.0f;
    float logo_w, logo_h;
    if (aspect >= 1.0f) {
      logo_w = logo_max_size;
      logo_h = logo_max_size / aspect;
    } else {
      logo_h = logo_max_size;
      logo_w = logo_max_size * aspect;
    }

    float logo_x = size.x * 0.55f + (size.x * 0.45f - logo_w) * 0.5f;
    float logo_y = (size.y - logo_h) * 0.5f;

    drawlist->AddImage(
        (ImTextureID)images::logo_texture,
        ImVec2(logo_x, logo_y),
        ImVec2(logo_x + logo_w, logo_y + logo_h),
        ImVec2(0, 0), ImVec2(1, 1),
        IM_COL32(255, 255, 255, (int)(alpha * 255)));
  }

  ImGui::PopStyleVar(); // Alpha

  // Handle login completion
  if (loginCompleted.load()) {
    isLoggingIn = false;
    loginCompleted = false;
    if (loginSuccess.load()) {
      ui::add_notify("C", "Welcome to Vanity Spoofer!",
                     ImVec4(0.6f, 0.9f, 0.6f, 1.0f));
      ui::variables::logged_in = true;
      ui::variables::loading = false;
      ui::variables::resize_main = true;
      SaveConfig();
    } else {
      ui::add_notify("D", "Auth Failed: " + loginErrorMessage,
                     ImVec4(0.99f, 0.60f, 0.60f, 1.0f));
    }
  }
}

struct Tab {
  const char *name;
  const char *icon;
};

struct tab_state {
  float anim_t = 0.0f;
};

void ui::render_main(ImDrawList *drawlist) {
  if (ui::variables::loading) {
    ui::variables::loading_anim += 0.05f;
    if (ui::variables::loading_anim > 1.0f)
      ui::variables::loading_anim = 1.0f;
  } else {
    ui::variables::loading_anim -= 0.05f;
    if (ui::variables::loading_anim < 0.0f)
      ui::variables::loading_anim = 0.0f;
  }

  float content_alpha = 1.0f - ui::variables::loading_anim;
  if (content_alpha <= 0.0f)
    return;

  ImVec2 size(window::size_max.x, window::size_max.y);
  ImGuiIO &io = ImGui::GetIO();

  // ============================================================
  // Header: "Vanity" (red) > Products
  // ============================================================
  float padding_x = 30.0f;
  float header_y = 25.0f;

  ImGui::PushFont(font.spacegrotesk_medium[0]);
  const char *h_brand = "Vanity";
  ImVec2 h_brand_size = ImGui::CalcTextSize(h_brand);
  drawlist->AddText(ImVec2(padding_x, header_y),
                    IM_COL32(255, 0, 0, (int)(content_alpha * 255)), h_brand);

  const char *h_sep = " > ";
  ImVec2 h_sep_size = ImGui::CalcTextSize(h_sep);
  float sep_x = padding_x + h_brand_size.x;
  drawlist->AddText(ImVec2(sep_x, header_y),
                    IM_COL32(136, 136, 136, (int)(content_alpha * 255)), h_sep);

  const char *h_page = "Products";
  float page_x = sep_x + h_sep_size.x;
  drawlist->AddText(ImVec2(page_x, header_y),
                    IM_COL32(255, 255, 255, (int)(content_alpha * 255)), h_page);
  ImGui::PopFont();

  // ============================================================
  // Product Cards
  // ============================================================
  float cards_start_y = header_y + h_brand_size.y + 30.0f;
  float card_width = size.x - padding_x * 2;
  float card_height = 90.0f;
  float card_gap = 14.0f;
  float card_rounding = 8.0f;

  // Icon box dimensions
  float icon_box_size = 64.0f;
  float icon_box_margin = 16.0f;

  // Download button dimensions
  float btn_size = 50.0f;
  float btn_margin = 16.0f;

  // --- Card 1: Vanity Spoofer ---
  {
    float card_y = cards_start_y;
    ImVec2 card_min(padding_x, card_y);
    ImVec2 card_max(padding_x + card_width, card_y + card_height);

    // Card background
    drawlist->AddRectFilled(card_min, card_max,
                            IM_COL32(26, 26, 26, (int)(content_alpha * 255)),
                            card_rounding);
    drawlist->AddRect(card_min, card_max,
                      IM_COL32(51, 51, 51, (int)(content_alpha * 255)),
                      card_rounding);

    // Icon box
    float icon_x = card_min.x + icon_box_margin;
    float icon_y = card_min.y + (card_height - icon_box_size) * 0.5f;
    ImVec2 icon_min(icon_x, icon_y);
    ImVec2 icon_max(icon_x + icon_box_size, icon_y + icon_box_size);
    drawlist->AddRectFilled(icon_min, icon_max,
                            IM_COL32(37, 37, 37, (int)(content_alpha * 255)),
                            6.0f);

    // Vanity logo icon
    if (images::logo_texture) {
      float pad = 6.0f;
      drawlist->AddImage(
          (ImTextureID)images::logo_texture,
          ImVec2(icon_min.x + pad, icon_min.y + pad),
          ImVec2(icon_max.x - pad, icon_max.y - pad),
          ImVec2(0, 0), ImVec2(1, 1),
          IM_COL32(255, 255, 255, (int)(content_alpha * 255)));
    } else {
      // Fallback: draw "V" letter
      ImGui::PushFont(font.spacegrotesk_medium[0]);
      const char *fl = "V";
      ImVec2 fs = ImGui::CalcTextSize(fl);
      drawlist->AddText(
          ImVec2(icon_min.x + (icon_box_size - fs.x) * 0.5f,
                 icon_min.y + (icon_box_size - fs.y) * 0.5f),
          IM_COL32(255, 255, 255, (int)(content_alpha * 255)), fl);
      ImGui::PopFont();
    }

    // Text details
    float text_x = icon_max.x + 20.0f;
    float text_y_start = card_min.y + 18.0f;

    ImGui::PushFont(font.spacegrotesk_medium[2]);
    drawlist->AddText(ImVec2(text_x, text_y_start),
                      IM_COL32(229, 226, 225, (int)(content_alpha * 255)),
                      "Vanity Spoofer");
    ImGui::PopFont();

    ImGui::PushFont(font.spacegrotesk_medium[1]);
    float detail_y = text_y_start + 26.0f;
    drawlist->AddText(ImVec2(text_x, detail_y),
                      IM_COL32(180, 180, 180, (int)(content_alpha * 255)),
                      "Status:");
    float status_x = text_x + ImGui::CalcTextSize("Status: ").x;
    drawlist->AddText(ImVec2(status_x, detail_y),
                      IM_COL32(255, 0, 0, (int)(content_alpha * 255)),
                      "Working");
    ImGui::PopFont();

    // Action button (red square with arrow icon)
    float btn_x = card_max.x - btn_margin - btn_size;
    float btn_y_pos = card_min.y + (card_height - btn_size) * 0.5f;
    ImVec2 btn_min(btn_x, btn_y_pos);
    ImVec2 btn_max(btn_x + btn_size, btn_y_pos + btn_size);

    ImVec2 mouse_pos = io.MousePos;
    bool btn_hovered = (mouse_pos.x >= btn_min.x && mouse_pos.x <= btn_max.x &&
                        mouse_pos.y >= btn_min.y && mouse_pos.y <= btn_max.y);

    ImU32 btn_col = btn_hovered
                        ? IM_COL32(212, 0, 0, (int)(content_alpha * 255))
                        : IM_COL32(255, 0, 0, (int)(content_alpha * 255));
    drawlist->AddRectFilled(btn_min, btn_max, btn_col, 8.0f);

    // Glow
    drawlist->AddShadowRect(btn_min, btn_max,
                            IM_COL32(255, 0, 0, (int)(content_alpha * 50)),
                            15.0f, ImVec2(0, 0), ImDrawFlags_RoundCornersAll,
                            8.0f);

    // Play/arrow icon (simple triangle)
    float tri_cx = btn_min.x + btn_size * 0.5f;
    float tri_cy = btn_min.y + btn_size * 0.5f;
    float tri_r = 10.0f;
    ImVec2 p1(tri_cx - tri_r * 0.5f, tri_cy - tri_r);
    ImVec2 p2(tri_cx - tri_r * 0.5f, tri_cy + tri_r);
    ImVec2 p3(tri_cx + tri_r, tri_cy);
    drawlist->AddTriangleFilled(
        p1, p2, p3, IM_COL32(255, 255, 255, (int)(content_alpha * 255)));

    if (btn_hovered)
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    // Click action: Spoof System
    if (btn_hovered && ImGui::IsMouseClicked(0) &&
        !ui::variables::loading) {
      try {
        ui::variables::loading = true;
        ui::variables::resize_main = false;
        ui::add_notify("C", "Initializing spoofer",
                       ImVec4(0.6f, 0.9f, 0.6f, 1.0f));
        std::thread([]() {
          try {
            StartVirturlizor();
            ui::add_notify("C", "System spoofed successfully!",
                           ImVec4(0.6f, 0.9f, 0.6f, 1.0f));
          } catch (const std::exception &e) {
            ui::add_notify("D", "Spoofer failed to execute",
                           ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
          } catch (...) {
            ui::add_notify("D", "Unknown spoofer error",
                           ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
          }
          ui::variables::loading = false;
          ui::variables::resize_main = true;
        }).detach();
      } catch (...) {
        ui::add_notify("D", "Failed to initialize spoofer",
                       ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
        ui::variables::loading = false;
        ui::variables::resize_main = true;
      }
    }
  }

  // --- Card 2: Vanity Cleaner ---
  {
    float card_y = cards_start_y + card_height + card_gap;
    ImVec2 card_min(padding_x, card_y);
    ImVec2 card_max(padding_x + card_width, card_y + card_height);

    // Card background
    drawlist->AddRectFilled(card_min, card_max,
                            IM_COL32(26, 26, 26, (int)(content_alpha * 255)),
                            card_rounding);
    drawlist->AddRect(card_min, card_max,
                      IM_COL32(51, 51, 51, (int)(content_alpha * 255)),
                      card_rounding);

    // Icon box
    float icon_x = card_min.x + icon_box_margin;
    float icon_y = card_min.y + (card_height - icon_box_size) * 0.5f;
    ImVec2 icon_min(icon_x, icon_y);
    ImVec2 icon_max(icon_x + icon_box_size, icon_y + icon_box_size);
    drawlist->AddRectFilled(icon_min, icon_max,
                            IM_COL32(37, 37, 37, (int)(content_alpha * 255)),
                            6.0f);

    // Vanity logo icon
    if (images::logo_texture) {
      float pad = 6.0f;
      drawlist->AddImage(
          (ImTextureID)images::logo_texture,
          ImVec2(icon_min.x + pad, icon_min.y + pad),
          ImVec2(icon_max.x - pad, icon_max.y - pad),
          ImVec2(0, 0), ImVec2(1, 1),
          IM_COL32(255, 255, 255, (int)(content_alpha * 255)));
    } else {
      ImGui::PushFont(font.spacegrotesk_medium[0]);
      const char *fl = "V";
      ImVec2 fs = ImGui::CalcTextSize(fl);
      drawlist->AddText(
          ImVec2(icon_min.x + (icon_box_size - fs.x) * 0.5f,
                 icon_min.y + (icon_box_size - fs.y) * 0.5f),
          IM_COL32(255, 255, 255, (int)(content_alpha * 255)), fl);
      ImGui::PopFont();
    }

    // Text details
    float text_x = icon_max.x + 20.0f;
    float text_y_start = card_min.y + 18.0f;

    ImGui::PushFont(font.spacegrotesk_medium[2]);
    drawlist->AddText(ImVec2(text_x, text_y_start),
                      IM_COL32(229, 226, 225, (int)(content_alpha * 255)),
                      "Vanity Cleaner");
    ImGui::PopFont();

    ImGui::PushFont(font.spacegrotesk_medium[1]);
    float detail_y = text_y_start + 26.0f;
    drawlist->AddText(ImVec2(text_x, detail_y),
                      IM_COL32(180, 180, 180, (int)(content_alpha * 255)),
                      "Status:");
    float status_x = text_x + ImGui::CalcTextSize("Status: ").x;
    drawlist->AddText(ImVec2(status_x, detail_y),
                      IM_COL32(255, 0, 0, (int)(content_alpha * 255)),
                      "Working");
    ImGui::PopFont();

    // Action button
    float btn_x = card_max.x - btn_margin - btn_size;
    float btn_y_pos = card_min.y + (card_height - btn_size) * 0.5f;
    ImVec2 btn_min(btn_x, btn_y_pos);
    ImVec2 btn_max(btn_x + btn_size, btn_y_pos + btn_size);

    ImVec2 mouse_pos = io.MousePos;
    bool btn_hovered = (mouse_pos.x >= btn_min.x && mouse_pos.x <= btn_max.x &&
                        mouse_pos.y >= btn_min.y && mouse_pos.y <= btn_max.y);

    ImU32 btn_col = btn_hovered
                        ? IM_COL32(212, 0, 0, (int)(content_alpha * 255))
                        : IM_COL32(255, 0, 0, (int)(content_alpha * 255));
    drawlist->AddRectFilled(btn_min, btn_max, btn_col, 8.0f);

    drawlist->AddShadowRect(btn_min, btn_max,
                            IM_COL32(255, 0, 0, (int)(content_alpha * 50)),
                            15.0f, ImVec2(0, 0), ImDrawFlags_RoundCornersAll,
                            8.0f);

    // Play/arrow icon
    float tri_cx = btn_min.x + btn_size * 0.5f;
    float tri_cy = btn_min.y + btn_size * 0.5f;
    float tri_r = 10.0f;
    ImVec2 p1(tri_cx - tri_r * 0.5f, tri_cy - tri_r);
    ImVec2 p2(tri_cx - tri_r * 0.5f, tri_cy + tri_r);
    ImVec2 p3(tri_cx + tri_r, tri_cy);
    drawlist->AddTriangleFilled(
        p1, p2, p3, IM_COL32(255, 255, 255, (int)(content_alpha * 255)));

    if (btn_hovered)
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    // Click action: Clean Traces
    if (btn_hovered && ImGui::IsMouseClicked(0) &&
        !ui::variables::loading) {
      try {
        ui::variables::loading = true;
        ui::variables::resize_main = false;
        ui::variables::is_cleaning_traces = true;
        ui::add_notify("C", "Initializing cleaning traces",
                       ImVec4(0.6f, 0.9f, 0.6f, 1.0f));
        std::thread([]() {
          try {
            CleanProcess();
          } catch (const std::exception &e) {
            ui::add_notify("D", "Cleaning failed to execute",
                           ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
          } catch (...) {
            ui::add_notify("D", "Unknown cleaning error",
                           ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
          }
        }).detach();
      } catch (...) {
        ui::variables::is_cleaning_traces = false;
        ui::add_notify("D", "Failed to initialize cleaner",
                       ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
        ui::variables::loading = false;
        ui::variables::resize_main = true;
      }
    }
  }
}

bool ui::items::input_text(const char *label, ImVec2 pos, ImVec2 Size,
                           char buf[], size_t buf_size,
                           ImGuiInputTextFlags flag) {
  const float Speed = 5.f;
  const float Rounding = 8.f;

  auto *window = ImGui::GetCurrentWindow();
  auto &style = ImGui::GetStyle();
  auto &io = ImGui::GetIO();

  float time = io.DeltaTime * Speed;

  ImVec4 prevFrameBg = style.Colors[ImGuiCol_FrameBg];
  ImVec4 prevTextDisabled = style.Colors[ImGuiCol_TextDisabled];
  ImVec4 prevTextSelectedBg = style.Colors[ImGuiCol_TextSelectedBg];
  ImVec2 prevFramePadding = style.FramePadding;

  style.Colors[ImGuiCol_FrameBg] = colors::input_text::frame_bg;
  style.Colors[ImGuiCol_TextDisabled] = colors::input_text::text_disabled;
  style.Colors[ImGuiCol_TextSelectedBg] = colors::input_text::text_selected_bg;
  style.FramePadding = ImVec2(8.f, 4.f);

  std::string lbl = "###";
  lbl += label;

  ImGui::PushID(label);
  ImGui::SetCursorPos(pos);
  ImGui::SetNextItemWidth(Size.x);

  ImGuiID id = window->GetID(label);
  static std::map<ImGuiID, input_state> anim;
  auto &i1 = anim[id];

  ImVec2 MIN = ImGui::GetCursorScreenPos();
  ImVec2 MAX = ImVec2(MIN.x + Size.x, MIN.y + Size.y);

  window->DrawList->AddRectFilled(ImVec2(MIN.x + 1 - 5, MIN.y + 1),
                                  ImVec2(MIN.x + 2, MAX.y - 1),
                                  ImGui::GetColorU32(i1.background_color),
                                  Rounding, ImDrawFlags_RoundCornersLeft);

  window->DrawList->AddRectFilled(ImVec2(MIN.x + 1, MIN.y + 1),
                                  ImVec2(MAX.x - 1, MAX.y - 1),
                                  ImGui::GetColorU32(i1.background_color),
                                  Rounding, ImDrawFlags_RoundCornersRight);

  window->DrawList->AddRect(
      ImVec2(MIN.x + 1 - 5, MIN.y + 1), ImVec2(MAX.x - 1, MAX.y - 1),
      ImGui::GetColorU32(i1.border_color), Rounding,
      ImDrawFlags_RoundCornersLeft | ImDrawFlags_RoundCornersRight, 1.0f);

  ImGui::PushStyleColor(ImGuiCol_Text, i1.text_color);
  ImGui::PushFont(font.spacegrotesk_medium[1]);

  float desiredHeight = Size.y;
  float fontSize = ImGui::GetFontSize();
  float framePaddingY = (desiredHeight - fontSize) * 0.5f;
  framePaddingY = framePaddingY < 0 ? 0 : framePaddingY;

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(style.FramePadding.x, framePaddingY));
  bool result =
      ImGui::InputTextWithHint(lbl.c_str(), "", buf, (int)buf_size, flag);
  ImGui::PopStyleVar();

  ImVec4 Background_Target =
      ImGui::IsItemActive()    ? colors::input_text::background_active
      : ImGui::IsItemHovered() ? colors::input_text::background_hovered
                               : colors::input_text::background;
  ImVec4 Border_Target =
      ImGui::IsItemActive() ? colors::input_text::border_active
      : ImGui::IsItemHovered() || strlen(buf) != 0 ? colors::input_text::border
                                                   : colors::input_text::border;
  ImVec4 Text_Color_Target = ImGui::IsItemActive()
                                 ? colors::input_text::text_active
                             : ImGui::IsItemHovered() || strlen(buf) != 0
                                 ? colors::input_text::text_hovered
                                 : colors::input_text::text;
  bool showLabel = strlen(buf) == 0;

  ImVec2 Text_Pos_Target = ImVec2(
      style.FramePadding.x, (Size.y - ImGui::CalcTextSize(label).y) / 2 + 1);

  if (showLabel) {
    window->DrawList->AddText(
        ImVec2(MIN.x + Text_Pos_Target.x, MIN.y + Text_Pos_Target.y - 2),
        ImGui::GetColorU32(i1.text_color), label);
  }
  ImGui::PopFont();

  i1.background_color =
      ImVec4(ImLerp(i1.background_color, Background_Target, time));
  i1.border_color = ImVec4(ImLerp(i1.border_color, Border_Target, time));
  i1.text_color = ImVec4(ImLerp(i1.text_color, Text_Color_Target, time));
  i1.text_pos = ImVec2(ImLerp(i1.text_pos, Text_Pos_Target, 0.1f));

  ImGui::PopStyleColor();
  ImGui::PopID();

  style.Colors[ImGuiCol_FrameBg] = prevFrameBg;
  style.Colors[ImGuiCol_TextDisabled] = prevTextDisabled;
  style.Colors[ImGuiCol_TextSelectedBg] = prevTextSelectedBg;
  style.FramePadding = prevFramePadding;

  return result;
}

bool ui::items::slider_to_confirm(const char *label, ImVec2 pos, ImVec2 size) {
  const float Speed = 5.f;
  const float Rounding = 8.f;
  const float ResetSpeed = 2.f;

  auto *window = ImGui::GetCurrentWindow();
  auto &io = ImGui::GetIO();
  float delta_time = io.DeltaTime;

  ImGui::PushID(label);
  ImGui::SetCursorPos(pos);

  ImVec2 MIN = ImGui::GetCursorScreenPos();
  ImVec2 MAX = ImVec2(MIN.x + size.x, MIN.y + size.y);

  ImGuiID id = window->GetID(label);
  static std::map<ImGuiID, float> drag_progress;
  static std::map<ImGuiID, bool> confirmed;
  static std::map<ImGuiID, input_state> anim;

  auto &progress = drag_progress[id];
  auto &is_confirmed = confirmed[id];
  auto &i1 = anim[id];

  ImRect total_rect(MIN, MAX);
  ImGui::InvisibleButton(label, size);
  bool is_hovered = ImGui::IsItemHovered();
  bool is_active = ImGui::IsItemActive();

  if (is_hovered)
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

  ImVec4 Background_Target = is_hovered
                                 ? ui::colors::input_text::background_hovered
                                 : ui::colors::input_text::background;
  ImVec4 Border_Target = is_active ? ui::colors::input_text::border_active
                                   : ui::colors::input_text::border;
  ImVec4 Text_Color_Target = is_hovered ? ui::colors::input_text::text_hovered
                                        : ui::colors::input_text::text;

  if (is_active && !is_confirmed) {
    float mouse_x = ImClamp(io.MousePos.x, MIN.x, MAX.x);
    progress = (mouse_x - MIN.x) / size.x;
  } else if (!is_confirmed && progress > 0.0f) {
    progress = ImMax(0.0f, progress - delta_time * ResetSpeed);
  }

  if (progress >= 0.98f && !is_confirmed && is_active) {
    is_confirmed = true;
    ImGui::PopID();
    return true;
  }

  window->DrawList->AddRectFilled(
      ImVec2(MIN.x + 1, MIN.y + 1), ImVec2(MAX.x - 1, MAX.y - 1),
      ImGui::GetColorU32(i1.background_color), Rounding);

  const float slider_width = 40.f;
  const float initial_offset = 2.f;

  float slider_x =
      ImLerp(MIN.x + initial_offset, MAX.x - slider_width, progress);

  ImVec2 slider_min = ImVec2(slider_x, MIN.y + 2);
  ImVec2 slider_max = ImVec2(slider_x + slider_width, MAX.y - 2);

  float global_alpha = ImGui::GetStyle().Alpha;
  ImU32 slider_color = ImGui::GetColorU32(
      ImVec4(35 / 255.f, 38 / 255.f, 49 / 255.f, global_alpha));
  window->DrawList->AddRectFilled(slider_min, slider_max, slider_color,
                                  Rounding * 0.5f);

  ImVec2 arrow_size = ImGui::CalcTextSize(">");
  ImVec2 arrow_pos = ImVec2(slider_min.x + (slider_width - arrow_size.x) * 0.5f,
                            slider_min.y + (size.y - arrow_size.y) * 0.5f - 2);
  window->DrawList->AddText(arrow_pos, ImGui::GetColorU32(i1.text_color), ">");

  const char *slider_label = "Go inside!";
  int len = (int)strlen(slider_label);
  ImVec2 base_text_pos =
      ImVec2(MIN.x + (size.x - ImGui::CalcTextSize(slider_label).x) * 0.5f,
             MIN.y + (size.y - ImGui::GetFontSize()) * 0.5f);

  float slider_right = slider_min.x + slider_width;

  float x_cursor = base_text_pos.x;

  for (int i = 0; i < len; ++i) {
    char buf[2] = {slider_label[i], '\0'};
    ImVec2 char_size = ImGui::CalcTextSize(buf);

    float letter_start = x_cursor;
    float letter_end = x_cursor + char_size.x;

    float covered =
        ImClamp((slider_right - letter_start) / char_size.x, 0.0f, 1.0f);

    float alpha = 1.0f - covered;

    ImVec4 letter_color = i1.text_color;
    letter_color.w *= alpha;

    window->DrawList->AddText(ImVec2(x_cursor, base_text_pos.y),
                              ImGui::GetColorU32(letter_color), buf);

    x_cursor += char_size.x;
  }

  window->DrawList->AddRect(
      ImVec2(MIN.x + 1, MIN.y + 1), ImVec2(MAX.x - 1, MAX.y - 1),
      ImGui::GetColorU32(i1.border_color), Rounding - 2, 0, 1.0f);

  float lerp_time = delta_time * Speed;
  i1.background_color =
      ImVec4(ImLerp(i1.background_color, Background_Target, lerp_time));
  i1.border_color = ImVec4(ImLerp(i1.border_color, Border_Target, lerp_time));
  i1.text_color = ImVec4(ImLerp(i1.text_color, Text_Color_Target, lerp_time));

  ImGui::PopID();
  return false;
}

bool ui::items::icon_button(const char *label, ImVec2 pos, ImVec2 size,
                            const char *icon = nullptr) {
  if (ui::variables::loading)
    ImGui::BeginDisabled();

  auto *window = ImGui::GetCurrentWindow();
  auto &io = ImGui::GetIO();
  auto &g = *ImGui::GetCurrentContext();

  const float Speed = 5.f;
  const float Rounding = 8.f;

  float time = io.DeltaTime * Speed;
  if (!window)
    return false;

  ImGui::SetCursorPos(pos);
  bool result = ImGui::InvisibleButton(label, size);
  ImRect rect = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};

  auto id = window->GetID(label);
  static std::map<ImGuiID, c> anim;
  auto &c = anim[id];

  ImVec4 base_color = ui::colors::loader::main;

  if (icon && strcmp(icon, "B") == 0)
    base_color = ImVec4(1.0f, 0.41f, 0.41f, 1.0f);

  ImVec4 Text_Color_Target = ui::colors::button::text_active;
  ImVec4 Border_Target =
      ImGui::IsItemActive()    ? ui::colors::button::border_active
      : ImGui::IsItemHovered() ? ui::colors::button::border_active
                               : ui::colors::button::border;

  if (ImGui::IsItemHovered()) {
    c.background_target =
        ImVec4(base_color.x + 0.2f > 1.0f ? 1.0f : base_color.x + 0.2f,
               base_color.y + 0.2f > 1.0f ? 1.0f : base_color.y + 0.2f,
               base_color.z + 0.2f > 1.0f ? 1.0f : base_color.z + 0.2f, 1.0f);
  } else {
    c.background_target = base_color;
  }

  if (c.background_current.w == 0.0f)
    c.background_current = c.background_target;

  c.background_current =
      ImLerp(c.background_current, c.background_target, time);
  c.text_target = ImLerp(c.text_target, Text_Color_Target, time);
  c.border_target = ImLerp(c.border_target, Border_Target, time);

  if (ImGui::IsItemHovered())
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

  ImGui::PushClipRect(ImVec2(rect.Min.x + 1, rect.Min.y + 1),
                      ImVec2(rect.Max.x - 1, rect.Max.y - 1), false);

  ImVec4 bg_color = c.background_current;

  ImDrawList *draw = ImGui::GetBackgroundDrawList();
  draw->AddRectFilled(rect.Min, rect.Max, ImGui::GetColorU32(bg_color),
                      Rounding);

  ImVec4 outline_color = ImVec4(1.0f, 1.0f, 1.0f, 0.15f * g.Style.Alpha);
  draw->AddRect(rect.Min, rect.Max, ImGui::GetColorU32(outline_color), Rounding,
                0, 1.0f);

  ImGui::PopClipRect();

  ImVec2 icon_size = ImVec2(0, 0);
  if (icon) {
    ImGui::PushFont(font.button_icon);
    icon_size = ImGui::CalcTextSize(icon);
    ImGui::PopFont();
  }

  ImGui::PushFont(font.spacegrotesk_medium[1]);
  ImVec2 text_size = ImGui::CalcTextSize(label);
  ImGui::PopFont();

  const float spacing = (icon ? 6.0f : 0.0f);
  const float total_width = icon_size.x + spacing + text_size.x;
  const float start_x = rect.Min.x + (rect.GetWidth() - total_width) * 0.5f - 4;
  const float center_y =
      rect.Min.y + (rect.GetHeight() - ImMax(icon_size.y, text_size.y)) * 0.5f;

  float current_x = start_x;
  if (icon) {
    ImGui::PushFont(font.button_icon);
    draw->AddText(ImVec2(current_x, center_y),
                  ImGui::GetColorU32(c.text_target), icon);
    ImGui::PopFont();
    current_x += icon_size.x + spacing;
  }

  ImGui::PushFont(font.spacegrotesk_medium[1]);
  draw->AddText(ImVec2(current_x, center_y - 1),
                ImGui::GetColorU32(c.text_target), label);
  ImGui::PopFont();

  if (ui::variables::loading)
    ImGui::EndDisabled();

  return result;
}

void ui::minimize_close(ImDrawList *drawlist) {
  ImVec2 pos(0, 10);
  ImVec2 size(pos.x + window::size_max.x, pos.y + window::size_max.y);

  static float alpha_min = 0.5f, alpha_max = 1.0f;
  static float close_alpha = alpha_min, min_alpha = alpha_min;

  ImGui::PushFont(font.font_awesome);

  float icon_size = 18.0f;
  ImVec2 pad(8, 4);
  ImVec2 pos_close(pos.x + size.x - icon_size - pad.x, pos.y + pad.y);
  ImVec2 pos_min(pos.x + size.x - icon_size * 2 - pad.x * 1.5f - 8,
                 pos.y + pad.y);
  ImVec2 mouse = ImGui::GetIO().MousePos;

  ImVec2 close_size =
      ImGui::CalcTextSize(ICON_FA_TIMES, nullptr, false, icon_size);
  ImVec2 min_size =
      ImGui::CalcTextSize(ICON_FA_MINUS, nullptr, false, icon_size);

  bool hovered_close =
      mouse.x >= pos_close.x && mouse.x <= pos_close.x + close_size.x &&
      mouse.y >= pos_close.y && mouse.y <= pos_close.y + close_size.y;
  bool hovered_min = mouse.x >= pos_min.x &&
                     mouse.x <= pos_min.x + min_size.x &&
                     mouse.y >= pos_min.y && mouse.y <= pos_min.y + min_size.y;

  close_alpha =
      ImLerp(close_alpha, hovered_close ? alpha_max : alpha_min, 0.15f);
  min_alpha = ImLerp(min_alpha, hovered_min ? alpha_max : alpha_min, 0.15f);

  drawlist->AddText(
      pos_min, IM_COL32(219, 221, 231, min_alpha * ui::alpha::background * 255),
      ICON_FA_MINUS);
  drawlist->AddText(
      pos_close,
      IM_COL32(219, 221, 231, close_alpha * ui::alpha::background * 255),
      ICON_FA_TIMES);

  if (hovered_close || hovered_min)
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  if (hovered_close && ImGui::IsMouseClicked(0))
    exit(1);
  if (hovered_min && ImGui::IsMouseClicked(0))
    ShowWindow(GetActiveWindow(), SW_MINIMIZE);

  ImGui::PopFont();
}
