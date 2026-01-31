#include "circuitViewWidget.h"

#include "../mainWindow.h"
#include "gpu/mainRenderer.h"
#include "gui/viewportManager/circuitView/circuitView.h"
#include "imgui/imgui.h"

CircuitViewWidget::CircuitViewWidget(WidgetId widgetId, MainWindow& mainWindow) : Widget(widgetId, mainWindow) {
	ViewportId viewportId = MainRenderer::get().registerViewport(mainWindow.getWindowId(), { 0, 0 }, { 100, 100 });
	circuitView = std::make_unique<CircuitView>(mainWindow.getEnvironment(), viewportId);
}

CircuitViewWidget::~CircuitViewWidget() { MainRenderer::get().deregisterViewport(circuitView->getViewportId()); }

void CircuitViewWidget::render() {
	ImGui::Begin(getWidgetIdStr().c_str());
	{
		ImGui::BeginChild("CircuitView");
		circuitView->getViewManager().setAspectRatio(ImGui::GetWindowSize().x / ImGui::GetWindowSize().y);
		MainRenderer::get().moveViewport(
			circuitView->getViewportId(),
			getMainWindow().getWindowId(),
			{ ImGui::GetWindowPos().x * getMainWindow().getWindowScalingSize(), ImGui::GetWindowPos().y * getMainWindow().getWindowScalingSize() },
			{ ImGui::GetWindowSize().x * getMainWindow().getWindowScalingSize(), ImGui::GetWindowSize().y * getMainWindow().getWindowScalingSize() }
		);
		ImGui::EndChild();
	}
	ImGui::End();
}
