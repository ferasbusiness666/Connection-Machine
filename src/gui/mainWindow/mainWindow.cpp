#include "mainWindow.h"

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <SDL3/SDL_video.h>

#include "gpu/mainRenderer.h"

#include "gui/mainWindow/circuitView/circuitViewWidget.h"
#include "gui/mainWindow/menuBar/menuBar.h"
#include "gui/rml/rmlRenderInterface.h"
#include "gui/rml/rmlSystemInterface.h"
#include "settingsWindow/settingsWindow.h"

#include "app.h"

#include "backend/settings/settings.h"
#include "computerAPI/directoryManager.h"
#include "environment/environment.h"

MainWindow::MainWindow(Environment* environment) :
	sdlWindow(App::get().registerWindow("Connection Machine")), environment(environment), toolManagerManager(environment->getBackend().getDataUpdateEventManager()), popUpManager(this) {
	sdlWindow->setRecieveEventFunction(std::bind(&MainWindow::recieveEvent, this, std::placeholders::_1));
	sdlWindow->setRenderFunction(std::bind(&MainWindow::updateRml, this));

	windowId = MainRenderer::get().registerWindow(sdlWindow.get());

	// create rmlUI context
	rmlContext = Rml::CreateContext("mainWindow_" + std::to_string(windowId), Rml::Vector2i(sdlWindow->getSize().first, sdlWindow->getSize().second)); // ptr managed by rmlUi (I think)

	// create rmlUI document
	rmlDocument = rmlContext->LoadDocument(DirectoryManager::getResourceDirectory().generic_string() + "/gui/mainWindow/mainWindow.rml");

	// SdlWindow* sdlWindow2 = App::get().registerWindow("Debugger").get();
	// WindowId windowId2 = MainRenderer::get().registerWindow(sdlWindow2);
	// Rml::Context* rmlContext2 = Rml::CreateContext("Debugger", Rml::Vector2i(sdlWindow2->getSize().first, sdlWindow2->getSize().second));
	// if (rmlContext2) {
	// 	Rml::ElementDocument* rmlDocument2 = rmlContext2->CreateDocument();
	// 	rmlDocument2->AppendChild(rmlDocument2->CreateElement("div"));
	// 	Rml::Debugger::Initialise(rmlContext2);
	// 	Rml::Debugger::SetContext(rmlContext);
	// 	Rml::Debugger::SetVisible(true);
	// 	sdlWindow2->setRenderFunction([windowId2, rmlContext2](){
	// 		RmlRenderInterface* rmlRenderInterface = dynamic_cast<RmlRenderInterface*>(Rml::GetRenderInterface());
	// 		if (rmlRenderInterface) {
	// 			rmlContext2->Update();
	// 			rmlRenderInterface->setWindowToRenderOn(windowId2);
	// 			MainRenderer::get().prepareForRmlRender(windowId2);
	// 			rmlContext2->Render();
	// 			MainRenderer::get().endRmlRender(windowId2);
	// 		}
	// 	});
	// 	sdlWindow2->setRecieveEventFunction(
	// 		[windowId2, rmlContext2, sdlWindow2](SDL_Event& event){
	// 			if (sdlWindow2->isThisMyEvent(event)) {
	// 				if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
	// 					Rml::RemoveContext(rmlContext2->GetName());
	// 					MainRenderer::get().deregisterWindow(windowId2);
	// 					App::get().deregisterWindow(sdlWindow2);
	// 					return true;
	// 				}

	// 				RmlSDL::InputEventHandler(rmlContext2, sdlWindow2->getHandle(), event, sdlWindow2->getWindowScalingSize());

	// 				// let renderer know we if resized the window
	// 				if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
	// 					MainRenderer::get().resizeWindow(windowId2, { event.window.data1, event.window.data2 });
	// 					rmlContext2->Update();
	// 				}
	// 				return true;
	// 			}
	// 			return false;
	// 		}
	// 	);
	// }

	// get widget for circuit view
	Rml::Element* circuitViewWidgetElement = rmlDocument->GetElementById("circuit-view-rendering-area");
	createCircuitViewWidget(circuitViewWidgetElement);

	// eval menutree
	Rml::Element* evalTreeParent = rmlDocument->GetElementById("eval-tree");
	evalWindow.emplace(
		&(environment->getBackend().getEvaluatorManager()),
		&(environment->getBackend().getCircuitManager()),
		this,
		environment->getBackend().getDataUpdateEventManager(),
		rmlDocument,
		evalTreeParent
	);

	//  blocks/tools menutree
	selectorWindow.emplace(
		environment->getBackend().getBlockDataManager(),
		environment->getBackend().getDataUpdateEventManager(),
		environment->getBackend().getCircuitManager().getProceduralCircuitManager(),
		&toolManagerManager,
		rmlDocument
	);

	Rml::Element* blockCreationMenu = rmlDocument->GetElementById("block-creation-form");
	blockCreationWindow.emplace(&(environment->getBackend().getCircuitManager()), this, environment->getBackend().getDataUpdateEventManager(), &toolManagerManager, rmlDocument, blockCreationMenu);

	simControlsManager.emplace(rmlDocument, getCircuitViewWidget(0), environment->getBackend().getDataUpdateEventManager());

	SettingsWindow* settingsWindow = new SettingsWindow(rmlDocument);

	MenuBar* menuBar = new MenuBar(rmlDocument, settingsWindow, this);

	// keybind handling
	rmlDocument->AddEventListener(Rml::EventId::Keydown, &keybindHandler);
	keybindHandler.addListener("Keybinds/Editing/Paste", [this]() { toolManagerManager.setTool("paste tool"); });
	keybindHandler.addListener("Keybinds/Editing/Tools/State Changer", [this]() { toolManagerManager.setTool("state changer"); });
	keybindHandler.addListener("Keybinds/Editing/Tools/Connection", [this]() { toolManagerManager.setTool("connection"); });
	keybindHandler.addListener("Keybinds/Editing/Tools/Move", [this]() { toolManagerManager.setTool("move"); });
	keybindHandler.addListener("Keybinds/Editing/Tools/Mode Changer", [this]() { toolManagerManager.setTool("mode changer"); });
	keybindHandler.addListener("Keybinds/Editing/Tools/Placement", [this]() { toolManagerManager.setTool("placement"); });
	keybindHandler.addListener("Keybinds/Editing/Tools/Selection Maker", [this]() { toolManagerManager.setTool("selection maker"); });
	keybindHandler.addListener("Keybinds/Window/Toggle Fullscreen", [this]() { sdlWindow->toggleBorderlessFullscreen(); });
	keybindHandler.addListener("Keybinds/Window/Increase UI Scale", [this]() { offsetUiScale(kUiScaleStep); });
	keybindHandler.addListener("Keybinds/Window/Decrease UI Scale", [this]() { offsetUiScale(-kUiScaleStep); });
	keybindHandler.addListener("Keybinds/Window/Reset UI Scale", [this]() { applyUiScale(1.0f); });

	const double* initialUiScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
	applyUiScale(initialUiScale ? static_cast<float>(*initialUiScale) : 1.0f);
	Settings::registerListener<SettingType::DECIMAL>("Appearance/UI Scale", [this](const double& value) { applyUiScale(static_cast<float>(value)); });

	// show rmlUi document
	rmlDocument->Show();

	Settings::registerListener<SettingType::FILE_PATH>("Appearance/Font", [this](const std::string& fontFilePath) {
		// Rml::LoadFontFace(fontFilePath);
		logInfo("loaded, {}", "", fontFilePath);
	});

	popUpManager.addFeedbackPopup();
}

MainWindow::~MainWindow() {
	App::get().deregisterWindow(sdlWindow);
	Rml::RemoveContext(rmlContext->GetName());
	MainRenderer::get().deregisterWindow(windowId);
	rmlContext = nullptr;
}

bool MainWindow::recieveEvent(SDL_Event& event) {
	// check if we want this event
	if (!sdlWindow->isThisMyEvent(event)) return false;

	if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
		if (App::get().closeMainWindow(this)) {
			App::get().deregisterWindow(sdlWindow.get());
		}
		return true;
	}

	if (event.type == SDL_EVENT_DROP_FILE) {
		std::string file = event.drop.data;
		std::cout << file << "\n";
		std::vector<circuit_id_t> ids = getActiveCircuitViewWidget()->getFileManager()->loadFromFile(file);
		if (ids.empty()) {
			// logError("Error", "Failed to load circuit file."); // Not a error! It is valid to load 0 circuits.
		} else {
			circuit_id_t id = ids.back();
			if (id == 0) {
				logError("Error", "Failed to load circuit file.");
			} else {
				getActiveCircuitViewWidget()->getCircuitView()->setCircuit(&environment->getBackend(), id);
				// circuitViewWidget->getCircuitView()->getBackend()->linkCircuitViewWithCircuit(circuitViewWidget->getCircuitView(), id);
				for (auto& iter : environment->getBackend().getEvaluatorManager().getEvaluators()) {
					if (iter.second->getCircuitId(Address()) == id) {
						getActiveCircuitViewWidget()->getCircuitView()->setEvaluator(&environment->getBackend(), iter.first);
						// circuitViewWidget->getCircuitView()->getBackend()->linkCircuitViewWithEvaluator(circuitViewWidget->getCircuitView(), iter.first, Address());
					}
				}
			}
		}
	}

	// send event to RML
	RmlSDL::InputEventHandler(rmlContext, sdlWindow->getHandle(), event, getSdlWindowScalingSize());

	if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
		applyUiScale(uiScale);
	}

	// let renderer know we if resized the window
	if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
		MainRenderer::get().resizeWindow(windowId, { event.window.data1, event.window.data2 });
		rmlContext->Update();
		for (auto circuitViewWidget : circuitViewWidgets) {
			circuitViewWidget->handleResize();
		}
	}

	return true;
}

void MainWindow::updateRml() {
	RmlRenderInterface* rmlRenderInterface = dynamic_cast<RmlRenderInterface*>(Rml::GetRenderInterface());
	if (rmlRenderInterface) {
		rmlContext->Update();
		rmlRenderInterface->setWindowToRenderOn(windowId);
		MainRenderer::get().prepareForRmlRender(windowId);
		rmlContext->Render();
		MainRenderer::get().endRmlRender(windowId);
	}
	// update circuit view widget UI components like TPS display
	for (auto& circuitViewWidget : circuitViewWidgets) {
		circuitViewWidget->updateTps();
	}
}

void MainWindow::createCircuitViewWidget(Rml::Element* element) {
	circuitViewWidgets.push_back(std::make_shared<CircuitViewWidget>(environment, rmlDocument, this, windowId, element));
	circuitViewWidgets.back()->getCircuitView()->setBackend(&environment->getBackend());
	toolManagerManager.addCircuitView(circuitViewWidgets.back()->getCircuitView());
	activeCircuitViewWidget = circuitViewWidgets.back(); // if it is created, it should be used
}

void MainWindow::offsetUiScale(double delta) {
	const double* storedScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
	const double currentScale = storedScale ? *storedScale : static_cast<double>(uiScale);
	const double targetScale = std::clamp(currentScale + delta, kUiScaleMin, kUiScaleMax);
	if (std::abs(targetScale - currentScale) < 1e-6) {
		applyUiScale(static_cast<float>(targetScale));
		return;
	}
	if (!Settings::set<SettingType::DECIMAL>("Appearance/UI Scale", targetScale)) {
		applyUiScale(static_cast<float>(targetScale));
	}
}

void MainWindow::applyUiScale(float scale) {
	const float clamped = std::clamp(scale, static_cast<float>(kUiScaleMin), static_cast<float>(kUiScaleMax));
	if (std::abs(static_cast<double>(clamped) - static_cast<double>(scale)) > 1e-6 && !uiScaleSettingUpdateInProgress) {
		const double* storedScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
		if (!storedScale || std::abs(*storedScale - clamped) > 1e-6) {
			uiScaleSettingUpdateInProgress = true;
			Settings::set<SettingType::DECIMAL>("Appearance/UI Scale", clamped);
			uiScaleSettingUpdateInProgress = false;
		}
	}
	uiScale = clamped;
	if (!rmlContext) return;
	float displayScale = 1.0f;
	if (sdlWindow) {
		displayScale = SDL_GetWindowDisplayScale(sdlWindow->getHandle());
		if (!(displayScale > 0.0f)) {
			displayScale = 1.0f;
		}
	}
	rmlContext->SetDensityIndependentPixelRatio(displayScale * uiScale);

	rmlContext->Update();
	for (auto circuitViewWidget : circuitViewWidgets) {
		circuitViewWidget->handleResize();
	}
}

void setGlobalCssPropertyRec(Rml::Element* element, const std::string& property, const std::string& value) {
	if (!element) return;

	element->SetProperty(property, value);
	for (int i = 0; i < element->GetNumChildren(); i++) {
		setGlobalCssPropertyRec(element->GetChild(i), property, value);
	}
}

void MainWindow::setGlobalCssProperty(const std::string& property, const std::string& value) {
	logInfo("Setting {} to {}", "", property, value);
	setGlobalCssPropertyRec(rmlDocument, property, value);
}
