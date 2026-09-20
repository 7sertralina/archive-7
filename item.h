#pragma once
#include <imgui.h>

struct input_state {
	ImVec4 background_color;
	ImVec4 border_color;
	ImVec4 text_color;
	ImVec4 icon_color;
	ImVec2 text_pos;
};

struct c {
	ImVec4 background_current = ImVec4(0, 0, 0, 0);  // NEW: current animated bg color
	ImVec4 background_target, border_target, text_target, circle_target;
	ImVec2 p;
	float background_p, d;
};
