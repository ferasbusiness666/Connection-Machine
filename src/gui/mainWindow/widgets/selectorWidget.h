#ifndef selectorWidget_h
#define selectorWidget_h

#include "../widget.h"
#include "backend/container/block/blockDefs.h"
#include "backend/dataUpdateEventManager.h"

class SelectorWidget : public Widget {
	struct SelectorTreeNode {
		void updateWith(const std::string_view& path, std::variant<BlockType> data) {
			size_t slashPos = path.find_first_of("/");
			if (slashPos == std::string::npos) {
				auto pair = children.emplace(path, SelectorTreeNode());
				pair.first->second.data = data;
			} else {
				auto pair = children.emplace(path.substr(0, slashPos), SelectorTreeNode());
				pair.first->second.updateWith(path.substr(slashPos + 1), data);
			}
		}
		std::optional<std::variant<BlockType>> data;
		std::unordered_map<std::string, SelectorTreeNode> children;
	};
public:
	SelectorWidget(WidgetId widgetId, MainWindow& mainWindow);
	~SelectorWidget();
private:
	void createTree(const SelectorTreeNode& node);
	void render() override final;
	DataUpdateEventManager::DataUpdateEventReceiver dataUpdateEventReceiver;
	std::mutex pathsMux;
	std::map<std::variant<BlockType, std::string>, std::string> paths;
	SelectorTreeNode root;
};

#endif /* selectorWidget_h */