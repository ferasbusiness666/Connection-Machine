#ifndef circuitView_h
#define circuitView_h

#include "events/eventRegister.h"
#include "renderer/circuitRenderManager.h"
#include "tools/toolManager.h"
#include "tutorial.h"
#include "tutorialDataManager.h"
#include "computerAPI/tutorialLoader.h"
#include "viewManager/viewManager.h"

#include "gpu/mainRendererDefs.h"

class Evaluator;
class Circuit;
class Backend;

class CircuitView {
	friend class Backend;
public:
	CircuitView(Environment& environment, ViewportId viewportId);

	// --------------- Gettters ---------------

	Circuit* getCircuit();
	const Circuit* getCircuit() const;

	Evaluator* getEvaluator();
	const Evaluator* getEvaluator() const;

	EventRegister& getEventRegister() { return eventRegister; }
	const EventRegister& getEventRegister() const { return eventRegister; }

	ToolManager& getToolManager() { return toolManager; }
	const ToolManager& getToolManager() const { return toolManager; }

	Tutorial& getTutorialManager() { return tutorialManager; }
	const Tutorial& getTutorialManager() const { return tutorialManager; }

	// void loadTutorial(const std::vector<TutorialStep>& steps) { tutorialManager.setTutorial(std::move(steps)); };

	void initializeTutorial(std::string filename) { tutorialManager.setTutorial(std::move(loadTutorialFromFile(filename))); }

	ViewManager& getViewManager() { return viewManager; }
	const ViewManager& getViewManager() const { return viewManager; }

	ViewportId getViewportId() const { return viewportId; }

	Backend& getBackend() { return backend; }
	const Backend& getBackend() const { return backend; }

	const Address& getAddress() const { return address; }

	// void setBackend(Backend* backend);
	void setEvaluator(evaluator_id_t evaluatorId, const Address& address = Address());
	void setEvaluator(std::shared_ptr<Evaluator> evaluator, const Address& address = Address());
	void setCircuit(std::shared_ptr<Circuit> circuit);
	void setCircuit(circuit_id_t circuitId);

	void viewChanged();

private:
	Backend& backend;

	ViewportId viewportId;
	circuit_id_t circuitId;
	evaluator_id_t evaluatorId;
	Address address;

	DataUpdateEventManager& dataUpdateEventManager;

	std::optional<CircuitRenderManager> circuitRenderManager;
	EventRegister eventRegister;
	ViewManager viewManager;
	ToolManager toolManager;
	Tutorial tutorialManager;
};

#endif /* circuitView_h */
