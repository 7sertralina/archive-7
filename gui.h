#pragma once
#include <d3d11.h>
#include <windows.h>
#include <dwmapi.h>
#include <string>
#include <D3DX11tex.h>
#pragma comment(lib, "D3DX11.lib")
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <imgui_freetype.h>
#include <map>
#include <algorithm>
#include <cmath>
#include <vector>
#include <sstream>
#include <fstream>
#include <tlhelp32.h>
#define CURL_STATICLIB
#include <curl/curl.h>
#pragma comment(lib, "freetype64.lib")  
#include "font_awesome.h"
#include "skStr.h"
#include "item.h"
#include "imspinner.h"
#include <Lmcons.h> 

inline ID3D11Device* g_pd3dDevice = nullptr;
inline ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
inline IDXGISwapChain* g_pSwapChain = nullptr;
inline UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
inline ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static char licensekey[256] = {};

#define size_of IM_ARRAYSIZE

inline HWND hwnd;
inline RECT rc;

namespace window {
	inline ImVec2 size_max = { 0, 0 };
	const float rounding = 10.f;
    inline ImVec2 previous_size = size_max;
    inline float speed = 8.f;
}

struct Fonts {
    ImFont* spacegrotesk_medium[3];
    ImFont* poppins_medium;
    ImFont* tab_icon;
    ImFont* button_icon;
    ImFont* notify_font;
    ImFont* font_awesome;
}; inline Fonts font;

namespace images
{
    inline ID3D11ShaderResourceView* logo_texture = nullptr;
    inline int logo_width = 0;
    inline int logo_height = 0;
};

namespace KeyAuthNS {
    // void initialize_keyauth();
}

namespace ui
{
    void move_window();
    void resize(ImVec2& size, const ImVec2& target);
    void initialize_fonts();
    void initialize_images();
    void add_notify(const std::string& icon, const std::string& msg, const ImVec4& icon_color);
    void render_notify();

    // render background
    void render_background(ImDrawList* drawlist);

    // render login
    void render_login(ImDrawList* drawlist);

    // render main
    void render_main(ImDrawList* drawlist);

    // render minimize & close
    void minimize_close(ImDrawList* drawlist);

    namespace items
    {
        bool input_text(const char* label, ImVec2 pos, ImVec2 Size, char buf[], size_t buf_size, ImGuiInputTextFlags flag);
        bool slider_to_confirm(const char* label, ImVec2 pos, ImVec2 size);
        bool icon_button(const char* label, ImVec2 pos, ImVec2 size, const char* icon);
    };

    namespace variables
    {
        inline bool logged_in = false;
        static float login_fade_timer = 0.f;
        inline bool login_screen_fully_hidden = false;
        inline float time_base = 0.f;
        inline bool resize_main = false;
        inline bool loading = false;
        inline float loading_anim = 0.0f;
        inline bool is_cleaning_traces = false;
        static float clean_timer_elapsed = 0.f;
        inline int login_tab = 0; // 0 = Login, 1 = Register
        inline bool remember_me = false;
    };

    // Mystic-style color scheme
    namespace colors
    {
        namespace loader
        {
        inline ImVec4 background = ImColor(13, 13, 13);       // #0D0D0D
        inline ImVec4 outline = ImColor(51, 51, 51);          // #333333
        inline ImVec4 main = ImColor(255, 0, 0);              // #FF0000 brand red
        inline ImVec4 child = ImColor(26, 26, 26);            // #1A1A1A card bg
        };

        namespace input_text
        {
            inline ImVec4 frame_bg = ImColor(26, 26, 26);              // #1A1A1A
            inline ImVec4 text_disabled = ImColor(102, 102, 102);      // #666666 placeholder
            inline ImVec4 text_selected_bg = ImColor(255, 0, 0, 80);   // red translucent

            inline ImVec4 background = ImColor(26, 26, 26);            // #1A1A1A
            inline ImVec4 background_hovered = ImColor(32, 32, 32);    // slightly lighter
            inline ImVec4 background_active = ImColor(38, 38, 38);     // active

            inline ImVec4 border = ImColor(51, 51, 51);                // #333333
            inline ImVec4 border_active = ImColor(255, 0, 0);          // red on focus

            inline ImVec4 render_selection = ImColor(255, 0, 0, 80);   // red translucent

            inline ImVec4 text = ImColor(229, 226, 225);               // #E5E2E1
            inline ImVec4 text_active = ImColor(255, 255, 255);        // white
            inline ImVec4 text_hovered = ImColor(235, 187, 180);       // warm light
        };

        namespace button
        {
            inline ImVec4 frame_bg = ImColor(255, 0, 0);               // red button bg
            inline ImVec4 text_disabled = ImColor(145, 145, 145);      // disabled
            inline ImVec4 text_selected_bg = ImColor(22, 22, 24, 150); // selected bg

            inline ImVec4 background_active = ImColor(180, 0, 0);      // darker red pressed
            inline ImVec4 background_hovered = ImColor(212, 0, 0);     // #D40000 hover
            inline ImVec4 background = ImColor(255, 0, 0);             // #FF0000

            inline ImVec4 border = ImColor(255, 0, 0);                 // red
            inline ImVec4 border_active = ImColor(255, 85, 64);        // lighter red

            inline ImVec4 text = ImColor(255, 255, 255);               // white text
            inline ImVec4 text_active = ImColor(255, 255, 255);        // white
            inline ImVec4 text_hovered = ImColor(255, 255, 255);       // white
        };

        namespace card
        {
            inline ImVec4 background = ImColor(26, 26, 26);            // #1A1A1A
            inline ImVec4 border = ImColor(51, 51, 51);                // #333333
            inline ImVec4 icon_bg = ImColor(37, 37, 37);               // #252525
        };

        namespace tab
        {
            inline ImVec4 background = ImColor(37, 37, 37);            // #252525
            inline ImVec4 active_bg = ImColor(26, 26, 26);             // #1A1A1A
            inline ImVec4 text_inactive = ImColor(136, 136, 136);      // #888888
            inline ImVec4 text_active = ImColor(255, 255, 255);        // white
        };
    };

    namespace alpha
    {
        inline float background = 0.f;
        inline float logged_in = 0.f;
        inline float spinner = 0.f;
    };
}
