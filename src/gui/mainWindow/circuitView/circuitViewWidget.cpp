#include "circuitViewWidget.h"

#include <SDL3/SDL.h>

#include "backend/backend.h"
#include "backend/settings/settings.h"

#include "gui/helper/eventPasser.h"
#include "gui/helper/keybindHelpers.h"
#include "gui/mainWindow/mainWindow.h"
#include "gui/viewportManager/circuitView/circuitView.h"
#include "gui/viewportManager/circuitView/events/customEvents.h"

// #include "backend/circuitTestCase/circuitTestCase.h"

#include "environment/environment.h"

void LoadCallback(void* userData, const char* const* filePaths, int filter) {
	CircuitViewWidget* circuitViewWidget = (CircuitViewWidget*)userData;
	if (filePaths && filePaths[0]) {
		std::string filePath = filePaths[0];
		std::vector<circuit_id_t> ids = circuitViewWidget->getFileManager()->loadFromFile(filePath);
		logInfo("Requested load from '{}' produced {} circuit(s)", "CircuitViewWidget:LoadCallback", filePath, ids.size());
		if (ids.empty()) {
			logInfo("No circuits found in '{}'", "CircuitViewWidget:LoadCallback", filePath);
			// logError("Failed to load circuit file."); // Not a error! It is valid to load 0 circuits.
			return;
		}
		circuit_id_t id = ids.back();
		bool doSetCir = true;
		// circuitViewWidget->getCircuitView()->getBackend()->linkCircuitViewWithCircuit(circuitViewWidget->getCircuitView(), id);
		for (auto& iter : circuitViewWidget->getCircuitView()->getBackend().getSimulatorManager().getSimulators()) {
			if (iter.second->getCircuitId() == id) {
				doSetCir = false;
				circuitViewWidget->getCircuitView()->setSimulatoruator(iter.first);
				// circuitViewWidget->getCircuitView()->getBackend()->linkCircuitViewWithSimulator(circuitViewWidget->getCircuitView(), iter.first, Address());
				return;
			}
		}
		if (doSetCir) circuitViewWidget->getCircuitView()->setCircuit(id);
		logInfo("Circuit view switched to loaded circuit {}", "CircuitViewWidget:LoadCallback", id);
	} else {
		logInfo("File dialog canceled.");
	}
}

CircuitViewWidget::CircuitViewWidget(Environment& environment, Rml::ElementDocument* document, MainWindow& mainWindow, WindowId windowId, Rml::Element* element) :
	fileManager(&environment.getCircuitFileManager()), document(document), mainWindow(mainWindow), windowId(windowId), element(element) {
	// create circuitView
	int w = this->element->GetClientWidth();
	int h = this->element->GetClientHeight();
	int x = this->element->GetAbsoluteLeft() + this->element->GetClientLeft();
	int y = this->element->GetAbsoluteTop() + this->element->GetClientTop();
	viewportId = MainRenderer::get().registerViewport(windowId, { x, y }, { w, h });
	circuitView = std::make_unique<CircuitView>(environment, viewportId);

	circuitView->getEventRegister().registerFunction("status bar changed", [this](const Event* event) -> bool {
		auto eventData = event->cast2<std::string>();
		if (eventData) setStatusBar(eventData->get());
		return false;
	});

	Settings::registerListener<SettingType::BOOL>("Keybinds/Camera/Scroll Panning", [this](const bool& enabled) { mouseControls = !enabled; });
	mouseControls = !Settings::get<SettingType::BOOL>("Keybinds/Camera/Scroll Panning");

	// set initial view
	element->AddEventListener(Rml::EventId::Resize, new EventPasser([this](Rml::Event&) { handleResize(); }));
	handleResize();

	// create keybind shortcuts and connect them
	document->AddEventListener(Rml::EventId::Keydown, &keybindHandler);
	keybindHandler.addListener("Keybinds/Camera/Home", [this]() { circuitView->getViewManager().focus(); });
	keybindHandler.addListener("Keybinds/Editing/Undo", [this]() {
		if (circuitView->getCircuit()) circuitView->getCircuit()->undo();
	});
	keybindHandler.addListener("Keybinds/Editing/Redo", [this]() {
		if (circuitView->getCircuit()) circuitView->getCircuit()->redo();
	});
	keybindHandler.addListener("Keybinds/File/Save", [this]() { save(); });
	keybindHandler.addListener("Keybinds/File/Save As", [this]() { asSave(); });
	keybindHandler.addListener("Keybinds/File/Open", [this]() { load(); });
	keybindHandler.addListener("Keybinds/Simulation/Start Stop", [this]() {
		if (!circuitView->getSimulator()) {
			this->mainWindow.logError("Cant start simulation when there is none");
			return;
		}
		circuitView->getSimulator()->setPause(!circuitView->getSimulator()->isPause());
		this->mainWindow.getSimControlsManager()->update();
	});
	keybindHandler.addListener("Keybinds/Simulation/Step Forward", [this]() {
		if (!circuitView->getSimulator()) {
			this->mainWindow.logError("Cant step simulation when there is none");
			return;
		}
		circuitView->getSimulator()->stepForward();
		this->mainWindow.getSimControlsManager()->update();
	});
	keybindHandler.addListener("Keybinds/Simulation/Step Back", [this]() {
		if (!circuitView->getSimulator()) {
			this->mainWindow.logError("Cant back step simulation when there is none");
			return;
		}
		if (circuitView->getSimulator()->stepBack()) {
			this->mainWindow.getSimControlsManager()->update();
		} else {
			this->mainWindow.logError("Cant back step simulation with no simulation data");
		}
	});
	keybindHandler.addListener("Keybinds/Simulation/Increase Speed", [this]() {
		if (!circuitView->getSimulator()) {
			this->mainWindow.logError("Cant change simulation speed when there is none");
			return;
		}
		circuitView->getSimulator()->increaseTickrateSeq();
		this->mainWindow.getSimControlsManager()->update();
	});
	keybindHandler.addListener("Keybinds/Simulation/Decrease Speed", [this]() {
		if (!circuitView->getSimulator()) {
			this->mainWindow.logError("Cant change simulation speed when there is none");
			return;
		}
		circuitView->getSimulator()->decreaseTickrateSeq();
		this->mainWindow.getSimControlsManager()->update();
	});
	keybindHandler.addListener("Keybinds/Simulation/Skip Forward", [this]() {
		if (circuitView->getSimulator()) {
			if (circuitView->getSimulator()->skipForward()) {
				this->mainWindow.getSimControlsManager()->update();
			} else {
				this->mainWindow.logError("Cant skip forward simulation with no simulation data");
			}
			this->mainWindow.getSimControlsManager()->update();
		} else {
			this->mainWindow.logError("Cant skip forward simulation when there is none");
		}
	});
	keybindHandler.addListener("Keybinds/Simulation/Skip Back", [this]() {
		if (circuitView->getSimulator()) {
			if (circuitView->getSimulator()->skipBack()) {
				this->mainWindow.getSimControlsManager()->update();
			} else {
				this->mainWindow.logError("Cant skip back simulation with no simulation data");
			}
		} else {
			this->mainWindow.logError("Cant skip back simulation when there is none");
		}
	});
	keybindHandler.addListener("Keybinds/Simulation/Reset Simulation", [this]() {
		if (!circuitView->getSimulator()) {
			this->mainWindow.logError("Cant reset simulation when there is none");
			return;
		}
		bool showConfirm = *Settings::get<SettingType::BOOL>("Preferences/Simulation/Show Confirmation for Reset Simulation");
		if (showConfirm) {
			simulator_id_t simulatorId = circuitView->getSimulator()->getSimulatorId();
			EvalLogicSimulator* simulator = circuitView->getBackend().getSimulator(simulatorId);

			this->mainWindow.getPopUpManager().addOptionsPopUp(
				"Reset Simulation States?",
				{
					{
						"Reset",
						[this, simulator]() {
							simulator->resetStates();
						}
					},
					{
						"Cancel",
						[]() {}
					}
				}
			);
		} else {
			circuitView->getSimulator()->resetStates();
		}
	});
	keybindHandler.addListener("Keybinds/Editing/Copy", [this]() { circuitView->getEventRegister().doEvent(Event("Copy")); });
	keybindHandler.addListener("Keybinds/Editing/Rotate CCW", [this]() { circuitView->getEventRegister().doEvent(Event("Tool Rotate Block CCW")); });
	keybindHandler.addListener("Keybinds/Editing/Rotate CW", [this]() { circuitView->getEventRegister().doEvent(Event("Tool Rotate Block CW")); });
	keybindHandler.addListener("Keybinds/Editing/Flip", [this]() { circuitView->getEventRegister().doEvent(Event("Tool Flip Block")); });
	keybindHandler.addListener("Keybinds/Editing/Confirm", [this]() { circuitView->getEventRegister().doEvent(Event("Confirm")); });
	keybindHandler.addListener("Keybinds/Editing/Tool Invert Mode", [this]() { circuitView->getEventRegister().doEvent(Event("Tool Invert Mode")); });
	keybindHandler.addListener("Keybinds/Editing/Tools/Cycle Mode", [this]() { this->mainWindow.getToolManagerManager().cycleActiveToolMode(); });
	keybindHandler.addListener("Keybinds/Editing/Tools/Cycle Mode Back", [this]() { this->mainWindow.getToolManagerManager().cycleActiveToolMode(-1); });
	keybindHandler.addListener("Keybinds/File/New", [this]() { newCircuit(); });
	keybindHandler.addListener(Rml::Input::KeyIdentifier::KI_P, 0, [this]() { this->mainWindow.log(circuitView->getViewManager().getPointerPosition().snap().toString()); });
	// keybindHandler.addListener("Keybinds/Tutorial/Start", [this]() { circuitView->getTutorialManager().StartTutorial(); });
	// keybindHandler.addListener("Keybinds/Tutorial/Stop", [this]() { circuitView->getTutorialManager().Stop(); });

	Rml::Element* root = document->GetElementById("main-container");
	root->AddEventListener(Rml::EventId::Mouseup, new EventPasser([this](Rml::Event& event) {
		int button = event.GetParameter<int>("button", 0);
		if (button == 0) { // left
			circuitView->getEventRegister().doEvent(PositionEvent("View Dettach Anchor", circuitView->getViewManager().getPointerPosition()));
			circuitView->getEventRegister().doEvent(PositionEvent("tool primary deactivate", circuitView->getViewManager().getPointerPosition()));
		} else if (button == 1) { // right
			circuitView->getEventRegister().doEvent(PositionEvent("tool secondary deactivate", circuitView->getViewManager().getPointerPosition()));
		}
	}));

	element->AddEventListener(Rml::EventId::Mousemove, new EventPasser([this](Rml::Event& event) {
		SDL_FPoint point(event.GetParameter<int>("mouse_x", 0), event.GetParameter<int>("mouse_y", 0));
		if (insideWindow(point)) { // inside the widget
			Vec2 viewPos = pixelsToView(point);
			circuitView->getEventRegister().doEvent(ViewPositionEvent("Pointer Move On View", viewPos));
		}
	}));

	element->AddEventListener(Rml::EventId::Mousedown, new EventPasser([this](Rml::Event& event) {
		int button = event.GetParameter<int>("button", 0);
		if (button == 0) { // left
			if (makeKeybind(event) == *Settings::get<SettingType::KEYBIND>("Keybinds/Editing/Pick Block")) {
				CircuitView* circuitView = this->getCircuitView();
				if (circuitView) {
					Circuit* circuit = circuitView->getCircuit();
					if (circuit) {
						const Block* block = circuit->getBlockContainer().getBlock(circuitView->getViewManager().getPointerPosition().snap());
						if (block) {
							BlockType type = block->type();
							Orientation orientation = block->getOrientation();
							ToolManagerManager& toolManagerManager = this->mainWindow.getToolManagerManager();
							toolManagerManager.setBlock(type);
							if (*Settings::get<SettingType::BOOL>("Preferences/Editing/Pick Block Copy Orientation")) {
								toolManagerManager.setOrientation(orientation);
							} else {
								toolManagerManager.setOrientation(Orientation());
							}
							event.StopPropagation();
							return;
						}
					}
				}
			}
			if (makeKeybind(event) == *Settings::get<SettingType::KEYBIND>("Keybinds/Camera/Pan")) {
				if (circuitView->getEventRegister().doEvent(PositionEvent("View Attach Anchor", circuitView->getViewManager().getPointerPosition()))) {
					event.StopPropagation();
					return;
				}
			}
			if (circuitView->getEventRegister().doEvent(PositionEvent("Tool Primary Activate", circuitView->getViewManager().getPointerPosition())))
				event.StopPropagation();
		} else if (button == 1) { // right
			if (circuitView->getEventRegister().doEvent(PositionEvent("Tool Secondary Activate", circuitView->getViewManager().getPointerPosition())))
				event.StopPropagation();
		}
	}));

	element->AddEventListener(Rml::EventId::Mouseover, new EventPasser([this](Rml::Event& event) {
		// // grab focus so key inputs work without clicking
		// setFocus(Qt::MouseFocusReason);
		this->document->Focus();
		SDL_FPoint point(event.GetParameter<int>("mouse_x", 0), event.GetParameter<int>("mouse_y", 0));
		Vec2 viewPos = pixelsToView(point);
		if (viewPos.x < 0 || viewPos.y < 0 || viewPos.x > 1 || viewPos.y > 1) return;
		circuitView->getEventRegister().doEvent(PositionEvent("pointer enter view", circuitView->getViewManager().viewToGrid(viewPos)));
	}));

	element->AddEventListener(Rml::EventId::Mouseout, new EventPasser([this](Rml::Event& event) {
		SDL_FPoint point(event.GetParameter<int>("mouse_x", 0), event.GetParameter<int>("mouse_y", 0));
		Vec2 viewPos = pixelsToView(point);
		if (viewPos.x >= 0 && viewPos.y >= 0 && viewPos.x <= 1 && viewPos.y <= 1) return;
		circuitView->getEventRegister().doEvent(PositionEvent("pointer exit view", circuitView->getViewManager().viewToGrid(viewPos)));
	}));

	element->AddEventListener(Rml::EventId::Mousescroll, new EventPasser([this](Rml::Event& event) {
		SDL_FPoint delta(event.GetParameter<float>("wheel_delta_x", 0) * 12, event.GetParameter<float>("wheel_delta_y", 0) * -12);
		// logInfo("{}, {}", "", delta.x, delta.y);
		if (mouseControls) {
			if (circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(delta.y) / 150.f))) event.StopPropagation();
		} else {
			const Keybind* keybind = Settings::get<SettingType::KEYBIND>("Keybinds/Camera/Zoom");
			if (keybind && makeKeybind(event) == *keybind) {
				// do zoom
				if (circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(delta.y) / 150.f))) event.StopPropagation();
			} else {
				if (circuitView->getEventRegister().doEvent(DeltaXYEvent(
						"view pan",
						delta.x / getPixelsWidth() * circuitView->getViewManager().getViewWidth(),
						delta.y / getPixelsHeight() * circuitView->getViewManager().getViewHeight()
					)))
					event.StopPropagation();
			}
		}
	}));

	element->AddEventListener("pinch", new EventPasser([this](Rml::Event& event) {
		float delta = event.GetParameter<float>("delta", 0);
		logInfo(delta);
		if (circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(delta) * 50))) event.StopPropagation();
		// if (mouseControls) {
		// } else {
		// 	if (event.GetParameter<int>("shift_key", 0)) {
		// 		// do zoom
		// 		if (circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(delta.y) / 100.f))) event.StopPropagation();
		// 	} else {
		// 		if (circuitView->getEventRegister().doEvent(DeltaXYEvent(
		// 			"view pan",
		// 			delta.x / getPixelsWidth() * circuitView->getViewManager().getViewWidth(),
		// 			delta.y / getPixelsHeight() * circuitView->getViewManager().getViewHeight()
		// 		))) event.StopPropagation();
		// 	}
		// }
	}));

	element->AddEventListener("dropFile", new EventPasser([this](Rml::Event& event) {
		std::string filePath = event.GetParameter<Rml::String>("file_path", "");
		std::cout << filePath << "\n";
		if (filePath.empty()) return;
		std::vector<circuit_id_t> ids = getFileManager()->loadFromFile(filePath);
		if (ids.empty()) {
			// logError("Error", "Failed to load circuit file."); // Not a error! It is valid to load 0 circuits.
		} else {
			circuit_id_t id = ids.back();
			if (id == 0) {
				logError("Error", "Failed to load circuit file.");
				this->mainWindow.logError("Failed to load circuit.");
			} else {
				circuitView->setCircuit(id);
				for (auto& iter : circuitView->getBackend().getSimulatorManager().getSimulators()) {
					if (iter.second->getCircuitId() == id) {
						circuitView->setSimulatoruator(iter.first);
					}
				}
			}
		}
	}));
}

void CircuitViewWidget::updateTps() {
	EvalLogicSimulator* simulator = circuitView->getSimulator();
	std::string tpsText = "Real tps: N/A";
	if (simulator) {
		double tps = simulator->getRealTickrate();
		if (tps < 5.235) tpsText = "Real tps: " + fmt::format("{:.2f}", tps);
		else if (tps < 100) tpsText = "Real tps: " + fmt::format("{:.1f}", tps);
		else tpsText = "Real tps: " + fmt::format("{:.0f}", tps);
	}
	Rml::Element* realTpsDisplay = document->GetElementById("real-tps-display");
	if (realTpsDisplay) {
		Rml::Element* realTpsDisplayElement = realTpsDisplay->GetChild(0);
		if (!realTpsDisplayElement) realTpsDisplayElement = realTpsDisplay->AppendChild(std::move(realTpsDisplay->GetOwnerDocument()->CreateTextNode("")));
		Rml::ElementText* realTpsDisplayText = rmlui_dynamic_cast<Rml::ElementText*>(realTpsDisplayElement);
		if (realTpsDisplayText && realTpsDisplayText->GetText() != tpsText) {
			realTpsDisplayText->SetText(tpsText);
		}
	}
}

void CircuitViewWidget::setSimState(bool state) {
	if (circuitView->getSimulator()) circuitView->getSimulator()->setPause(!state);
}

void CircuitViewWidget::simUseSpeed(bool state) {
	if (circuitView->getSimulator()) circuitView->getSimulator()->setUseTickrate(state);
}

void CircuitViewWidget::setSimSpeed(double speed) {
	if (circuitView->getSimulator()) circuitView->getSimulator()->setTickrate(speed);
}

void CircuitViewWidget::newCircuit() {
	circuit_id_t id = circuitView->getBackend().createCircuit();
	if (id == 0) return; // other logs shoud happen before this
	logInfo("Created new circuit {}", "CircuitViewWidget::newCircuit", id);
	circuitView->setCircuit(id);
	// tmp get eval with this circuit id because circuit manager makes a eval for loaded circuits
	for (auto& iter : circuitView->getBackend().getSimulatorManager().getSimulators()) {
		if (iter.second->getCircuitId() == id) {
			circuitView->setSimulatoruator(iter.second.get());
			// circuitView->getBackend()->linkCircuitViewWithSimulator(circuitView.get(), iter.first, Address());
			return;
		}
	}
}

void CircuitViewWidget::setStatusBar(const std::string& text) {
	Rml::Element* statusBar = element->GetElementById("status-bar");
	statusBar->SetInnerRML(text);
}

// save current circuit view widget we are viewing. Right now only works if it is the only widget in application.
// Called via Ctrl-S keybind
void CircuitViewWidget::save() {
	if (circuitView->getCircuit()) mainWindow.getPopUpManager().savePopUp(circuitView->getCircuit()->getUUID());
	else {
		logWarning("Could not save because non circuit was selected.", "CircuitViewWidget");
		mainWindow.log("Could not save because non circuit was selected.");
	}
}

void CircuitViewWidget::asSave() {
	if (circuitView->getCircuit()) mainWindow.getPopUpManager().saveAsPopUp(circuitView->getCircuit()->getUUID());
	else {
		logWarning("Could not save because non circuit was selected.", "CircuitViewWidget");
		mainWindow.log("Could not save because non circuit was selected.");
	}
}

// for drag and drop load directly onto this circuit view widget
void CircuitViewWidget::load() {
	if (!fileManager) return;
	static const SDL_DialogFileFilter filters[] = {
		{ "Circuit Files", "cir" },
		{ "BLIF Files", "blif" },
		{ "WASM Files", "wasm" },
	};

	logInfo("Opening load dialog for circuit import", "CircuitViewWidget::load");
	SDL_ShowOpenFileDialog(LoadCallback, this, nullptr, filters, 3, nullptr, true);
}

void CircuitViewWidget::handleResize() {
	int w = this->element->GetClientWidth();
	int h = this->element->GetClientHeight();
	int x = this->element->GetAbsoluteLeft() + this->element->GetClientLeft();
	int y = this->element->GetAbsoluteTop() + this->element->GetClientTop();

	circuitView->getViewManager().setAspectRatio((float)w / (float)h);
	MainRenderer::get().moveViewport(viewportId, windowId, { x, y }, { w, h });
}

inline Vec2 CircuitViewWidget::pixelsToView(const SDL_FPoint& point) const {
	return Vec2((point.x - getPixelsXPos()) / getPixelsWidth(), (point.y - getPixelsYPos()) / getPixelsHeight());
}

inline bool CircuitViewWidget::insideWindow(const SDL_FPoint& point) const {
	int x = point.x - getPixelsXPos();
	int y = point.y - getPixelsYPos();
	return x >= 0 && y >= 0 && x < getPixelsWidth() && y < getPixelsHeight();
}

inline float CircuitViewWidget::getPixelsWidth() const { return element->GetClientWidth(); }

inline float CircuitViewWidget::getPixelsHeight() const { return element->GetClientHeight(); }

inline float CircuitViewWidget::getPixelsXPos() const { return element->GetAbsoluteLeft() + element->GetClientLeft(); }

inline float CircuitViewWidget::getPixelsYPos() const { return element->GetAbsoluteTop() + element->GetClientTop(); }
