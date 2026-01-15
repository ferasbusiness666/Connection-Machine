#include "simControlsManager.h"

#include <RmlUi/Core/Input.h>

#include "gui/helper/eventPasser.h"
#include "gui/helper/tooltip.h"
#include "gui/mainWindow/circuitView/circuitViewWidget.h"

#include "gui/helper/uiControllers/numberControllers.h"
#include "gui/mainWindow/mainWindow.h"


SimControlsManager::SimControlsManager(
	Rml::ElementDocument* document,
	std::shared_ptr<CircuitViewWidget> circuitViewWidget,
	DataUpdateEventManager& dataUpdateEventManager
) : circuitViewWidget(circuitViewWidget), dataUpdateEventReceiver(dataUpdateEventManager) {
	toggleSimElement = document->GetElementById("toggle-simulation");
	realisticElement = document->GetElementById("realistic-button");
	limitSpeedElement = document->GetElementById("limit-speed-checkbox");
	tpsInputElement = document->GetElementById("tps-input");

	/* trust me bro */ new Tooltip(circuitViewWidget->getMainWindow().getSdlWindoHandle(), realisticElement, "Toggle Realistic Simulation");
	/* trust me bro */ new Tooltip(circuitViewWidget->getMainWindow().getSdlWindoHandle(), limitSpeedElement, "Toggle Limit Speed");
	toggleSimElement->AddEventListener("click", new EventPasser(std::bind(&SimControlsManager::toggleSimulation, this)));
	realisticElement->AddEventListener("click", new EventPasser(std::bind(&SimControlsManager::setRealistic, this)));
	limitSpeedElement->AddEventListener("click", new EventPasser([this](Rml::Event& event) {
		limitSpeed();
		event.StopPropagation();
	}));

	EvalLogicSimulator* simulator = this->circuitViewWidget->getCircuitView()->getSimulator();
	new UiDataController<float>(tpsInputElement, simulator ? simulator->getTickrate() : 0, [this](float value) {
		EvalLogicSimulator* simulator = this->circuitViewWidget->getCircuitView()->getSimulator();
		if (simulator) simulator->setTickrate(value);
	}, [dataUpdateEventManager = &dataUpdateEventManager, circuitViewWidget](std::function<void(double)> func) {
		std::shared_ptr<DataUpdateEventManager::DataUpdateEventReceiver> DUER =
			std::make_shared<DataUpdateEventManager::DataUpdateEventReceiver>(*dataUpdateEventManager);
		DUER->linkFunction("simulatorTargetTickrateSet", [circuitViewWidget, func](const DataUpdateEventManager::EventData& event) {
			auto data = event.cast<std::pair<simulator_id_t, double>>();
			EvalLogicSimulator* simulator = circuitViewWidget->getCircuitView()->getSimulator();
			if (!simulator) return;
			if (data->get().first == simulator->getSimulatorId()) func(data->get().second);
		});
		return DUER;
	}, [](float value, const std::string* string) -> std::string {
		if (string) {
			if (string->empty()) return "tps";
			std::string tpsStr = fmt::format("{:.1f}", value);
			if (tpsStr.back() == '0') tpsStr.pop_back();
			if (string->back() != '.' && tpsStr.back() == '.') tpsStr.pop_back();
			return tpsStr + "tps";
		}
		std::string tpsStr = fmt::format("{:.1f}", value);
		if (tpsStr.back() == '0') tpsStr.pop_back();
		if (tpsStr.back() == '.') tpsStr.pop_back();
		return tpsStr + "tps";
	});

	// std::stringstream ss(correctValue);
	// if (!approx_equals(simulator->getTickrate(), tps)) {
	// 	std::string tpsStr = fmt::format("{:.1f}", simulator->getTickrate());
	// 	if (tpsStr.back() == '0') tpsStr.pop_back();
	// 	if (tpsStr.back() == '.') tpsStr.pop_back();
	// 	tpsInputElement->SetInnerRML(std::string(tpsStr.size(), ' ') + "tps");
	// 	tpsInputElement->SetAttribute<Rml::String>("value", tpsStr);
	// }

	// tpsInputElement->AddEventListener("change", new EventPasser(std::bind(&SimControlsManager::setTPS, this)));

	dataUpdateEventReceiver.linkFunction("circuitViewChangeSimulator", std::bind(&SimControlsManager::update, this));
	update();
}

void SimControlsManager::update() {
	EvalLogicSimulator* simulator = circuitViewWidget->getCircuitView()->getSimulator();
	if (simulator) {
		if (simulator->isPause()) {
			toggleSimElement->SetClass("checked", false);
		} else {
			toggleSimElement->SetClass("checked", true);
		}
		if (simulator->isRealistic()) {
			realisticElement->SetClass("checked", true);
			realisticElement->SetInnerRML("R");
		} else {
			realisticElement->SetClass("checked", false);
			realisticElement->SetInnerRML("S");
		}
		if (simulator->getUseTickrate()) {
			limitSpeedElement->SetAttribute("checked", true);
		} else {
			limitSpeedElement->RemoveAttribute("checked");
		}
		Rml::String value = tpsInputElement->GetAttribute<Rml::String>("value", "");
		std::string correctValue;
		bool foundDecimal = false;
		for (char c : value) {
			if ('0' <= c && c <= '9') {
				correctValue += c;
			} else if (c == '.' && !foundDecimal) {
				foundDecimal = true;
				correctValue += c;
			}
		}
		std::stringstream ss(correctValue);
		double tps = 0;
		ss >> tps;
		if (!approx_equals(simulator->getTickrate(), tps)) {
			std::string tpsStr = fmt::format("{:.1f}", simulator->getTickrate());
			if (tpsStr.back() == '0') tpsStr.pop_back();
			if (tpsStr.back() == '.') tpsStr.pop_back();
			tpsInputElement->SetInnerRML(std::string(tpsStr.size(), ' ') + "tps");
			tpsInputElement->SetAttribute<Rml::String>("value", tpsStr);
		}
	} else {
		toggleSimElement->SetClass("checked", false);
		realisticElement->SetClass("checked", false);
		realisticElement->SetInnerRML("S");
		limitSpeedElement->RemoveAttribute("checked");
		tpsInputElement->SetInnerRML("tps");
		tpsInputElement->SetAttribute<Rml::String>("value", "");
	}
}

void SimControlsManager::toggleSimulation() {
	EvalLogicSimulator* simulator = circuitViewWidget->getCircuitView()->getSimulator();
	if (simulator) {
		if (simulator->isPause()) {
			simulator->setPause(false);
		} else {
			simulator->setPause(true);
		}
	}
	update();
}

void SimControlsManager::setRealistic() {
	EvalLogicSimulator* simulator = circuitViewWidget->getCircuitView()->getSimulator();
	if (simulator) {
		simulator->setRealistic(!(simulator->isRealistic()));
	}
	update();
}

void SimControlsManager::limitSpeed() {
	EvalLogicSimulator* simulator = circuitViewWidget->getCircuitView()->getSimulator();
	if (simulator) {
		simulator->setUseTickrate(!(simulator->getUseTickrate()));
	}
	update();
}

void SimControlsManager::setTPS() {
	EvalLogicSimulator* simulator = circuitViewWidget->getCircuitView()->getSimulator();
	if (simulator) {
		Rml::String value = tpsInputElement->GetAttribute<Rml::String>("value", "");
		std::string correctValue;
		bool foundDecimal = false;
		for (char c : value) {
			if ('0' <= c && c <= '9') {
				correctValue += c;
			} else if (c == '.' && !foundDecimal) {
				foundDecimal = true;
				correctValue += c;
			}
		}
		std::stringstream ss(correctValue);
		double tps = 0;
		ss >> tps;
		tpsInputElement->SetInnerRML(std::string(correctValue.size(), ' ') + "tps");
		tpsInputElement->SetAttribute<Rml::String>("value", correctValue);
		simulator->setTickrate(tps);
	} else {
		tpsInputElement->SetInnerRML("tps");
	}
	update();
}
