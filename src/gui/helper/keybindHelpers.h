#ifndef keybindHelpers_h
#define keybindHelpers_h

#include "imgui/imgui.h"
class Keybind;

std::set<ImGuiKey> getPressedKeys();

bool isPressingKeys(const Keybind& keybinds, const std::set<ImGuiKey>& pressedKeys);

#endif /* keybindHelpers_h */
