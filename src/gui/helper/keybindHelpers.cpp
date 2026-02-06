#include "keybindHelpers.h"

#include "backend/settings/keybind.h"
#include "gpu/renderer/imgui/imGuiRenderer.h"

std::set<ImGuiKey> getPressedKeys(SDL_Window& sdlWindow) {
	ImGuiRenderer::getImGuiRenderer(sdlWindow);
	std::set<ImGuiKey> pressedKeys;
	for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
		if (ImGui::IsKeyPressed((ImGuiKey)key)) pressedKeys.insert((ImGuiKey)key);
	}
	if (ImGui::IsKeyDown(ImGuiKey::ImGuiMod_Ctrl)) pressedKeys.insert(ImGuiKey::ImGuiMod_Ctrl);
	if (ImGui::IsKeyDown(ImGuiKey::ImGuiMod_Shift)) pressedKeys.insert(ImGuiKey::ImGuiMod_Shift);
	if (ImGui::IsKeyDown(ImGuiKey::ImGuiMod_Alt)) pressedKeys.insert(ImGuiKey::ImGuiMod_Alt);
	if (ImGui::IsKeyDown(ImGuiKey::ImGuiMod_Super)) pressedKeys.insert(ImGuiKey::ImGuiMod_Super);
	return pressedKeys;
}

bool isPressingKeybind(const Keybind& keybinds, const std::set<ImGuiKey>& pressedKeys) {
	if (
		((ImGuiKey)(keybinds.getKeybind() & ~ImGuiKey::ImGuiMod_Mask_) != ImGuiKey::ImGuiKey_None) &&
		(!pressedKeys.contains((ImGuiKey)(keybinds.getKeybind() & ~ImGuiKey::ImGuiMod_Mask_)))
	) return false;
	if (((keybinds.getKeybind() & ImGuiKey::ImGuiMod_Ctrl) != 0) != (
			pressedKeys.contains(ImGuiKey::ImGuiMod_Ctrl)// ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_RightCtrl) ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_LeftCtrl)
	)) {
		return false;
	}
	if (((keybinds.getKeybind() & ImGuiKey::ImGuiMod_Alt) != 0) != (
			pressedKeys.contains(ImGuiKey::ImGuiMod_Alt)// ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_RightAlt) ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_LeftAlt)
	)) {
		return false;
	}
	if (((keybinds.getKeybind() & ImGuiKey::ImGuiMod_Shift) != 0) != (
			pressedKeys.contains(ImGuiKey::ImGuiMod_Shift)// ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_RightShift) ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_LeftShift)
	)) {
		return false;
	}
	if (((keybinds.getKeybind() & ImGuiKey::ImGuiMod_Super) != 0) != (
			pressedKeys.contains(ImGuiKey::ImGuiMod_Super)// ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_RightSuper) ||
			// pressedKeys.contains(ImGuiKey::ImGuiKey_LeftSuper)
	)) {
		return false;
	}
	return true;
}
