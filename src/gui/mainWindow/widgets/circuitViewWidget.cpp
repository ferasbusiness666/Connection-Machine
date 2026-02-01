#include "circuitViewWidget.h"

#include "../mainWindow.h"
#include "gpu/mainRenderer.h"
#include "gui/viewportManager/circuitView/circuitView.h"
#include "imgui/imgui.h"

CircuitViewWidget::CircuitViewWidget(WidgetId widgetId, MainWindow& mainWindow) : Widget(widgetId, mainWindow) {
	ViewportId viewportId = MainRenderer::get().registerViewport({ 100, 100 });
	circuitView = std::make_unique<CircuitView>(mainWindow.getEnvironment(), viewportId);
}

CircuitViewWidget::~CircuitViewWidget() { MainRenderer::get().deregisterViewport(circuitView->getViewportId()); }

void CircuitViewWidget::render() {
	ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	ImGui::Begin(getWidgetIdStr().c_str());
	{
		ImGui::BeginChild("CircuitView");
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		circuitView->getViewManager().setAspectRatio(viewportPanelSize.x / viewportPanelSize.y); // prob needs to be does in a thread safe way.
		MainRenderer::get().resizeViewport(circuitView->getViewportId(), { viewportPanelSize.x, viewportPanelSize.y });
		VkDescriptorSet descriptor = MainRenderer::get().getViewportLatestImage(circuitView->getViewportId());
		if (descriptor != VK_NULL_HANDLE) ImGui::Image(descriptor, ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
		// MainRenderer::get().moveViewport(
		// 	circuitView->getViewportId(),
		// 	getMainWindow().getWindowId(),
		// 	{ ImGui::GetWindowPos().x * getMainWindow().getWindowScalingSize(), ImGui::GetWindowPos().y * getMainWindow().getWindowScalingSize() },
		// 	{ ImGui::GetWindowSize().x * getMainWindow().getWindowScalingSize(), ImGui::GetWindowSize().y * getMainWindow().getWindowScalingSize() }
		// );
		ImGui::EndChild();
	}
	ImGui::End();
}
