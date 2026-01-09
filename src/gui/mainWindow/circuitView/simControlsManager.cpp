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

	Evaluator* evaluator = this->circuitViewWidget->getCircuitView()->getEvaluator();
	new UiDataController<float>(tpsInputElement, evaluator ? evaluator->getEvalLogicSimulator().getTickrate() : 0, [this](float value) {
		Evaluator* evaluator = this->circuitViewWidget->getCircuitView()->getEvaluator();
		if (evaluator) evaluator->getEvalLogicSimulator().setTickrate(value);
	}, [dataUpdateEventManager = &dataUpdateEventManager, circuitViewWidget](std::function<void(double)> func) {
		std::shared_ptr<DataUpdateEventManager::DataUpdateEventReceiver> DUER =
			std::make_shared<DataUpdateEventManager::DataUpdateEventReceiver>(*dataUpdateEventManager);
		DUER->linkFunction("evaluatorTargetTickrateSet", [circuitViewWidget, func](const DataUpdateEventManager::EventData* dataEvent) {
			auto data = dataEvent->cast<std::pair<simulator_id_t, double>>();
			Evaluator* evaluator = circuitViewWidget->getCircuitView()->getEvaluator();
			if (!evaluator) return;
			if (data->get().first == evaluator->getEvalLogicSimulator().getSimulatorId()) func(data->get().second);
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
	// if (!approx_equals(evaluator->getEvalLogicSimulator().getTickrate(), tps)) {
	// 	std::string tpsStr = fmt::format("{:.1f}", evaluator->getEvalLogicSimulator().getTickrate());
	// 	if (tpsStr.back() == '0') tpsStr.pop_back();
	// 	if (tpsStr.back() == '.') tpsStr.pop_back();
	// 	tpsInputElement->SetInnerRML(std::string(tpsStr.size(), ' ') + "tps");
	// 	tpsInputElement->SetAttribute<Rml::String>("value", tpsStr);
	// }

	// tpsInputElement->AddEventListener("change", new EventPasser(std::bind(&SimControlsManager::setTPS, this)));

	dataUpdateEventReceiver.linkFunction("circuitViewChangeEvaluator", std::bind(&SimControlsManager::update, this));
	update();
}

void SimControlsManager::update() {
	Evaluator* evaluator = circuitViewWidget->getCircuitView()->getEvaluator();
	if (evaluator) {
		if (evaluator->getEvalLogicSimulator().isPause()) {
			toggleSimElement->SetClass("checked", false);
		} else {
			toggleSimElement->SetClass("checked", true);
		}
		if (evaluator->getEvalLogicSimulator().isRealistic()) {
			realisticElement->SetClass("checked", true);
			realisticElement->SetInnerRML("R");
		} else {
			realisticElement->SetClass("checked", false);
			realisticElement->SetInnerRML("S");
		}
		if (evaluator->getEvalLogicSimulator().getUseTickrate()) {
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
		if (!approx_equals(evaluator->getEvalLogicSimulator().getTickrate(), tps)) {
			std::string tpsStr = fmt::format("{:.1f}", evaluator->getEvalLogicSimulator().getTickrate());
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
	Evaluator* evaluator = circuitViewWidget->getCircuitView()->getEvaluator();
	if (evaluator) {
		if (evaluator->getEvalLogicSimulator().isPause()) {
			evaluator->getEvalLogicSimulator().setPause(false);
		} else {
			evaluator->getEvalLogicSimulator().setPause(true);
		}
	}
	update();
}

void SimControlsManager::setRealistic() {
	Evaluator* evaluator = circuitViewWidget->getCircuitView()->getEvaluator();
	if (evaluator) {
		evaluator->getEvalLogicSimulator().setRealistic(!(evaluator->getEvalLogicSimulator().isRealistic()));
	}
	update();
}

void SimControlsManager::limitSpeed() {
	Evaluator* evaluator = circuitViewWidget->getCircuitView()->getEvaluator();
	if (evaluator) {
		evaluator->getEvalLogicSimulator().setUseTickrate(!(evaluator->getEvalLogicSimulator().getUseTickrate()));
	}
	update();
}

void SimControlsManager::setTPS() {
	Evaluator* evaluator = circuitViewWidget->getCircuitView()->getEvaluator();
	if (evaluator) {
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
		evaluator->getEvalLogicSimulator().setTickrate(tps);
	} else {
		tpsInputElement->SetInnerRML("tps");
	}
	update();
}
