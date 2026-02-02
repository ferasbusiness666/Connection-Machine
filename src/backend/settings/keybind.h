#ifndef keybind_h
#define keybind_h

#include "imgui/imgui.h"

class Keybind {
public:
	Keybind() = default;
	Keybind(ImGuiKey key) : key(key) { }
	Keybind(unsigned int key) : Keybind((ImGuiKey)key) { }
	Keybind(std::string keyString) {
		if (Keybind::stringToKeyId.empty()) {
			for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
				ImGuiKey imguiKey = (ImGuiKey)key;
				if (key == ImGuiKey_ReservedForModCtrl) imguiKey = ImGuiKey::ImGuiMod_Ctrl;
				else if (key == ImGuiKey_ReservedForModShift) imguiKey = ImGuiKey::ImGuiMod_Shift;
				else if (key == ImGuiKey_ReservedForModAlt) imguiKey = ImGuiKey::ImGuiMod_Alt;
				else if (key == ImGuiKey_ReservedForModSuper) imguiKey = ImGuiKey::ImGuiMod_Super;
				const char* name = ImGui::GetKeyName(imguiKey);
				if (name && name[0]) Keybind::stringToKeyId[name] = imguiKey;
			}
		}

    	std::istringstream iss(keyString);
    	std::string token;
    	while (std::getline(iss, token, '+')) {
			// trim spaces
        	token.erase(0, token.find_first_not_of(" \t"));
        	token.erase(token.find_last_not_of(" \t") + 1);
			key = (ImGuiKey)(key | Keybind::stringToKeyId[token]);
		}
	}
	bool operator==(Keybind keybind) const { return keybind.getKeybind() == key; }
	bool operator!=(Keybind keybind) const { return keybind.getKeybind() != key; }

	ImGuiKey getKeybind() const { return key; }
	std::string toString() const { return toString(false); }
	std::string toString(bool convertForLayoutForceDisable) const;

private:
	ImGuiKey key = (ImGuiKey)(ImGuiKey::ImGuiKey_None | ImGuiKey::ImGuiMod_None);
	static std::map<std::string, ImGuiKey> stringToKeyId;

	ImGuiKey transformKeyIdForLayout(ImGuiKey keyid) const;

};

template <> struct std::hash<Keybind> {
	inline std::size_t operator()(Keybind keybind) const noexcept { return std::hash<unsigned int>{}(keybind.getKeybind()); }
};

#endif /* keybind_h */
