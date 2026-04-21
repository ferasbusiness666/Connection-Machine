#include "logicSimulator.h"
#include "util/fastMath.h"

LogicSimulator::LogicSimulator(simulator_id_t simulatorId, std::vector<simulator_gate_id_t>& dirtySimulatorIds, DataUpdateEventManager &dataUpdateEventManager) :
	simulatorConfig(simulatorId, dataUpdateEventManager), dirtySimulatorIds(dirtySimulatorIds), simulatorIdProvider(4) {
	simulatorConfig.subscribe([this]() {
		{
			SimPauseGuard pauseGuard(*this);
			this->regenerateJobs();
		}
		bool shouldSprint = (this->simulatorConfig.isRunning() && !this->simulatorConfig.isTickrateLimiterEnabled());
		this->threadPool.setSprinting(shouldSprint);

		std::lock_guard<std::mutex> lk(cvMutex);
		cv.notify_all();
	});

	extendDataVectors(simulator_gate_id_t(4));

	resetStates();
	simulationThread = std::thread(&LogicSimulator::simulationLoop, this);
}

LogicSimulator::~LogicSimulator() {
	{
		std::lock_guard<std::mutex> lk(cvMutex);
		running = false;
		cv.notify_all();
	}
	if (simulationThread.joinable()) {
		simulationThread.join();
	}
}

double LogicSimulator::getAverageTickrate() const {
	if (!simulatorConfig.isRunning()) {
		return 0.0;
	}
	double tickspeed = averageTickrate.load(std::memory_order_acquire);
	// if tickspeed close enough to target tickspeed, return target tickspeed
	double targetTickrate = simulatorConfig.getTargetTickrate();
	double percentageError = (tickspeed - targetTickrate) / targetTickrate;
	if (std::abs(percentageError) < 0.01) {
		return targetTickrate;
	}
	return tickspeed;
}

void LogicSimulator::simulationLoop() {
	using clock = std::chrono::steady_clock;
	nextTick = clock::now();
	lastTickTime = clock::now();
	bool isFirstTick = true;

	while (running) {
		processPendingStateChanges();

		bool didSprint = false;
		while (running && !pauseRequest.load(std::memory_order_acquire) && simulatorConfig.canConsumeSprintTick()) {
			didSprint = true;
			auto currentTime = clock::now();
			tickOnce();
			simulatorConfig.consumeSprintTick();
			updateEmaTickrate(currentTime, lastTickTime, isFirstTick);
			if (pauseRequest.load(std::memory_order_acquire)) break;
		}

		if (!didSprint) {
			if (simulatorConfig.isRunning()) {
				auto currentTime = clock::now();

				tickOnce();

				updateEmaTickrate(currentTime, lastTickTime, isFirstTick);

				if (simulatorConfig.isTickrateLimiterEnabled()) {
					double targetTickrate = simulatorConfig.getTargetTickrate();
					if (targetTickrate > 0) {
						auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / targetTickrate));
						nextTick += period;
						std::unique_lock lk(cvMutex);
						cv.wait_until(lk, nextTick, [&] { return pauseRequest || !running || !simulatorConfig.isRunning(); });
					}
				}
			} else {
				averageTickrate.store(0.0, std::memory_order_release);
				std::unique_lock lk(cvMutex);
				cv.wait(lk, [&] {
					std::lock_guard<std::mutex> stateLock(stateChangeQueueMutex);
					return pauseRequest || !running || simulatorConfig.isRunning() || simulatorConfig.getSprintCount() > 0 || !pendingStateChanges.empty();
				});
				nextTick = clock::now();
				lastTickTime = clock::now();
				isFirstTick = true;
			}
		}
	}
}

inline void LogicSimulator::updateEmaTickrate(
	const std::chrono::steady_clock::time_point& currentTime,
	std::chrono::steady_clock::time_point& lastTickTime,
	bool& isFirstTick) {
	if (!isFirstTick) {
		auto deltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - lastTickTime);
		if (deltaTime.count() > 0) {
			double currentTickrate = 1.0e9 / static_cast<double>(deltaTime.count());
			double dtSeconds = std::chrono::duration<double>(deltaTime).count();
			double alpha = 1.0 - std::exp(-dtSeconds * std::log(2.0) / tickrateHalflife);

			double currentEMA = averageTickrate.load(std::memory_order_acquire);

			// if (currentTickrate/currentEMA > 3 || 0.33 > currentTickrate/currentEMA) {
			// 	averageTickrate.store(currentTickrate, std::memory_order_release);
			// } else {
			double newEMA = alpha * currentTickrate + (1.0 - alpha) * currentEMA;
			averageTickrate.store(newEMA, std::memory_order_release);
			// }
		}
	} else {
		isFirstTick = false;
	}
	lastTickTime = currentTime;
}

inline void LogicSimulator::tickOnce() {
	std::unique_lock lkNext(mainDataMutex);

	threadPool.resetAndLoad(jobs);
	threadPool.waitForCompletion(true);
	advanceReplayHead();

	for (auto& gate : junctions) gate.tick(statesB);
	std::unique_lock lkCurEx(statesAMutex);
	std::swap(statesA, statesB);
}

void LogicSimulator::processPendingStateChanges() {
	std::queue<StateChange> localQueue;
	{
		std::lock_guard<std::mutex> lock(stateChangeQueueMutex);
		std::swap(localQueue, pendingStateChanges);
	}

	if (!localQueue.empty()) {
		std::scoped_lock lk(mainDataMutex, statesAMutex);
		while (!localQueue.empty()) {
			const StateChange& change = localQueue.front();
			localQueue.pop();

			extendDataVectors(change.id);
			if (!gateLocations.contains(change.id)) {
				continue;
			}
			GateLocation& gateLocation = gateLocations.at(change.id);
			if (gateLocation.gateType == SimGateType::CONSTANT || gateLocation.gateType == SimGateType::JUNCTION) {
				continue;
			}

			statesA[change.id] = change.state;
			statesB[change.id] = change.state;
			setStateUsed[replayHead] = true;
		}
		for (auto& gate : junctions) gate.doubleTick(statesA, statesB);
	}
}

void LogicSimulator::setState(simulator_gate_id_t id, logic_state_t st) {
	if (viewingReplay) {
		return;
	}
	if (!simulatorIdProvider.isIdUsed(id)) {
		return;
	}
	// we don't want to freeze up if the mutexes are locked, so we'll only set the state if we can successfully lock. otherwise, we'll wait until the next tick to set the states.

	std::unique_lock lkMain(mainDataMutex, std::try_to_lock);
	std::unique_lock lkA(statesAMutex, std::try_to_lock);

	if (lkMain.owns_lock() && lkA.owns_lock()) {
		extendDataVectors(id); // why
		if (!gateLocations.contains(id)) {
			return;
		}
		GateLocation& gateLocation = gateLocations.at(id);
		if (gateLocation.gateType == SimGateType::CONSTANT || gateLocation.gateType == SimGateType::JUNCTION) {
			return;
		}
		statesA[id] = st;
		statesB[id] = st;
		setStateUsed[replayHead] = true;
		for (auto& gate : junctions) gate.doubleTick(statesA, statesB); // disabling because this greatly breaks the simulation // added back because tests fail
	} else {
		std::lock_guard<std::mutex> lock(stateChangeQueueMutex);
		pendingStateChanges.push({ id, st });
		cv.notify_one();
	}
}

void LogicSimulator::resetStates() {
	std::unique_lock lkMain(mainDataMutex);
	std::unique_lock lkA(statesAMutex);
	for (simulator_gate_id_t id : statesA.ids()) {
		statesA[id] = logic_state_t::UNDEFINED;
		statesB[id] = logic_state_t::UNDEFINED;
	}
	statesA[0] = logic_state_t::LOW;
	statesB[0] = logic_state_t::LOW;
	statesA[1] = logic_state_t::HIGH;
	statesB[1] = logic_state_t::HIGH;
	statesA[2] = logic_state_t::FLOATING;
	statesB[2] = logic_state_t::FLOATING;
	statesA[3] = logic_state_t::UNDEFINED;
	statesB[3] = logic_state_t::UNDEFINED;
	auto resetGateStates = [this](auto& gates) {
		for (auto& gate : gates) {
			gate.resetState(simulatorConfig.isRealistic(), statesA);
			gate.resetState(simulatorConfig.isRealistic(), statesB);
		}
		};
	resetGateStates(andGates);
	resetGateStates(xorGates);
	resetGateStates(junctions);
	resetGateStates(buffers);
	resetGateStates(singleBuffers);
	resetGateStates(tristateBuffers);
	resetGateStates(constantGates);
	resetGateStates(constantResetGates);
	resetGateStates(copySelfOutputGates);
	resetGateStates(portsToIntGates);
	for (auto& junction : junctions) junction.doubleTick(statesA, statesB);
}

logic_state_t LogicSimulator::getState(simulator_gate_id_t id) const {
	std::shared_lock lk(statesAMutex);
	return getStateUnlocked(id);
}

std::vector<logic_state_t> LogicSimulator::getStates(const std::vector<simulator_gate_id_t>& ids) const {
	std::vector<logic_state_t> result(ids.size());
	std::shared_lock lk(statesAMutex);
	for (size_t i = 0; i < ids.size(); ++i) {
		result[i] = getStateUnlocked(ids[i]);
	}
	return result;
}

logic_state_t LogicSimulator::getStateUnlocked(simulator_gate_id_t id) const {
	if (id >= statesA.size()) {
		return logic_state_t::UNDEFINED;
	}
	if (viewingReplay) {
		return statesReplay[stateView][id];
	}
	return statesA[id];
}

simulator_gate_id_t LogicSimulator::addGate(const BlockType blockType) {
	invalidateReplay();
	simulator_gate_id_t simulatorId;

	switch (blockType) {
	case BlockType::AND:
		simulatorId = andGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(andGates.back().getId());
		extendDataVectors(simulatorId);
		andGates.push_back({ simulatorId, false, false });
		updateGateLocation(simulatorId, SimGateType::AND, andGates.size() - 1);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::OR:
		simulatorId = andGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(andGates.back().getId());
		extendDataVectors(simulatorId);
		andGates.push_back({ simulatorId, true, true });
		updateGateLocation(simulatorId, SimGateType::AND, andGates.size() - 1);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::NAND:
		simulatorId = andGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(andGates.back().getId());
		extendDataVectors(simulatorId);
		andGates.push_back({ simulatorId, false, true });
		updateGateLocation(simulatorId, SimGateType::AND, andGates.size() - 1);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::NOR:
		simulatorId = andGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(andGates.back().getId());
		extendDataVectors(simulatorId);
		andGates.push_back({ simulatorId, true, false });
		updateGateLocation(simulatorId, SimGateType::AND, andGates.size() - 1);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		andGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::XOR:
		simulatorId = xorGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(xorGates.back().getId());
		extendDataVectors(simulatorId);
		xorGates.push_back({ simulatorId, false });
		updateGateLocation(simulatorId, SimGateType::XOR, xorGates.size() - 1);
		xorGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		xorGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::XNOR:
		simulatorId = xorGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(xorGates.back().getId());
		extendDataVectors(simulatorId);
		xorGates.push_back({ simulatorId, true });
		updateGateLocation(simulatorId, SimGateType::XOR, xorGates.size() - 1);
		xorGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		xorGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::BUFFER:
		simulatorId = singleBuffers.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(singleBuffers.back().getId());
		extendDataVectors(simulatorId);
		singleBuffers.push_back({ simulatorId, false });
		updateGateLocation(simulatorId, SimGateType::SINGLE_BUFFER, singleBuffers.size() - 1);
		singleBuffers.back().resetState(simulatorConfig.isRealistic(), statesA);
		singleBuffers.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::NOT:
		simulatorId = singleBuffers.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(singleBuffers.back().getId());
		extendDataVectors(simulatorId);
		singleBuffers.push_back({ simulatorId, true });
		updateGateLocation(simulatorId, SimGateType::SINGLE_BUFFER, singleBuffers.size() - 1);
		singleBuffers.back().resetState(simulatorConfig.isRealistic(), statesA);
		singleBuffers.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::JUNCTION:
		simulatorId = junctions.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(junctions.back().getId());
		extendDataVectors(simulatorId);
		junctions.push_back({ simulatorId, logic_state_t::FLOATING });
		updateGateLocation(simulatorId, SimGateType::JUNCTION, junctions.size() - 1);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesA);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::JUNCTION_L:
		simulatorId = junctions.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(junctions.back().getId());
		extendDataVectors(simulatorId);
		junctions.push_back({ simulatorId, logic_state_t::LOW });
		updateGateLocation(simulatorId, SimGateType::JUNCTION, junctions.size() - 1);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesA);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::JUNCTION_H:
		simulatorId = junctions.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(junctions.back().getId());
		extendDataVectors(simulatorId);
		junctions.push_back({ simulatorId, logic_state_t::HIGH });
		updateGateLocation(simulatorId, SimGateType::JUNCTION, junctions.size() - 1);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesA);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::JUNCTION_X:
		simulatorId = junctions.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(junctions.back().getId());
		extendDataVectors(simulatorId);
		junctions.push_back({ simulatorId, logic_state_t::UNDEFINED });
		updateGateLocation(simulatorId, SimGateType::JUNCTION, junctions.size() - 1);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesA);
		junctions.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::TRISTATE_BUFFER:
		simulatorId = tristateBuffers.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(tristateBuffers.back().getId());
		extendDataVectors(simulatorId);
		tristateBuffers.push_back({ simulatorId, false });
		updateGateLocation(simulatorId, SimGateType::TRISTATE_BUFFER, tristateBuffers.size() - 1);
		tristateBuffers.back().resetState(simulatorConfig.isRealistic(), statesA);
		tristateBuffers.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	// case GateType::TRISTATE_BUFFER_INVERTED:
	// 	simulatorId = tristateBuffers.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(tristateBuffers.back().getId());
	// 	extendDataVectors(simulatorId);
	// 	tristateBuffers.push_back({ simulatorId, true });
	// 	updateGateLocation(simulatorId, SimGateType::TRISTATE_BUFFER, tristateBuffers.size() - 1);
	// 	tristateBuffers.back().resetState(simulatorConfig.isRealistic(), statesA);
	// 	tristateBuffers.back().resetState(simulatorConfig.isRealistic(), statesB);
	// 	break;
	// case GateType::CONSTANT_OFF:
	// 	simulatorId = constantGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(constantGates.back().getId());
	// 	extendDataVectors(simulatorId);
	// 	constantGates.push_back({ simulatorId, logic_state_t::LOW });
	// 	updateGateLocation(simulatorId, SimGateType::CONSTANT, constantGates.size() - 1);
	// 	constantGates.back().resetState(simulatorConfig.isRealistic(), statesA);
	// 	constantGates.back().resetState(simulatorConfig.isRealistic(), statesB);
	// 	break;
	// case GateType::CONSTANT_ON:
	// 	simulatorId = constantGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(constantGates.back().getId());
	// 	extendDataVectors(simulatorId);
	// 	constantGates.push_back({ simulatorId, logic_state_t::HIGH });
	// 	updateGateLocation(simulatorId, SimGateType::CONSTANT, constantGates.size() - 1);
	// 	constantGates.back().resetState(simulatorConfig.isRealistic(), statesA);
	// 	constantGates.back().resetState(simulatorConfig.isRealistic(), statesB);
	// 	break;
	case BlockType::CONSTANT_OFF:
		simulatorId = constantGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(constantGates.back().getId());
		extendDataVectors(simulatorId);
		constantGates.push_back({ simulatorId, logic_state_t::LOW });
		updateGateLocation(simulatorId, SimGateType::CONSTANT, constantGates.size() - 1);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::CONSTANT_ON:
		simulatorId = constantGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(constantGates.back().getId());
		extendDataVectors(simulatorId);
		constantGates.push_back({ simulatorId, logic_state_t::HIGH });
		updateGateLocation(simulatorId, SimGateType::CONSTANT, constantGates.size() - 1);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::CONSTANT_Z:
		simulatorId = constantGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(constantGates.back().getId());
		extendDataVectors(simulatorId);
		constantGates.push_back({ simulatorId, logic_state_t::FLOATING });
		updateGateLocation(simulatorId, SimGateType::CONSTANT, constantGates.size() - 1);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::CONSTANT_X:
		simulatorId = constantGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(constantGates.back().getId());
		extendDataVectors(simulatorId);
		constantGates.push_back({ simulatorId, logic_state_t::UNDEFINED });
		updateGateLocation(simulatorId, SimGateType::CONSTANT, constantGates.size() - 1);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		constantGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::SWITCH:
		simulatorId = copySelfOutputGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(copySelfOutputGates.back().getId());
		extendDataVectors(simulatorId);
		copySelfOutputGates.push_back({ simulatorId });
		updateGateLocation(simulatorId, SimGateType::COPY_SELF_OUTPUT, copySelfOutputGates.size() - 1);
		copySelfOutputGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		copySelfOutputGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	case BlockType::BUTTON:
		simulatorId = copySelfOutputGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(copySelfOutputGates.back().getId());
		extendDataVectors(simulatorId);
		copySelfOutputGates.push_back({ simulatorId });
		updateGateLocation(simulatorId, SimGateType::COPY_SELF_OUTPUT, copySelfOutputGates.size() - 1);
		copySelfOutputGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		copySelfOutputGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;
	// case GateType::THROUGH:
	// 	simulatorId = singleBuffers.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(singleBuffers.back().getId());
	// 	extendDataVectors(simulatorId);
	// 	singleBuffers.push_back({ simulatorId, false });
	// 	updateGateLocation(simulatorId, SimGateType::SINGLE_BUFFER, singleBuffers.size() - 1);
	// 	singleBuffers.back().resetState(simulatorConfig.isRealistic(), statesA);
	// 	singleBuffers.back().resetState(simulatorConfig.isRealistic(), statesB);
	// 	break;
	case BlockType::TICK_BUTTON:
		simulatorId = constantResetGates.size() == 0 ? simulatorIdProvider.getNewId() : simulatorIdProvider.getNewId(constantResetGates.back().getId());
		extendDataVectors(simulatorId);
		constantResetGates.push_back({ simulatorId, logic_state_t::LOW });
		updateGateLocation(simulatorId, SimGateType::CONSTANT_RESET, constantResetGates.size() - 1);
		constantResetGates.back().resetState(simulatorConfig.isRealistic(), statesA);
		constantResetGates.back().resetState(simulatorConfig.isRealistic(), statesB);
		break;

	// case BlockType::NONE:
	// 	logError("Cannot add gate of type NONE", "LogicSimulator::addGate");
	// 	return 0;
	default:
		logError("Cannot add gate of type {}", "LogicSimulator::addGate", (unsigned int)blockType);
		return simulator_gate_id_t(3);
	}

	return simulatorId;
}

void LogicSimulator::removeGate(simulator_gate_id_t simulatorId) {
	invalidateReplay();
	auto locationIt = gateLocations.find(simulatorId);
	if (locationIt == gateLocations.end()) {
		logError("Cannot remove gate: not found " + std::to_string(simulatorId), "LogicSimulator::removeGate");
		return;
	}

	std::optional<std::vector<simulator_gate_id_t>> outputIdsOpt = getOutputSimulatorIdsFromGate(simulatorId);
	if (!outputIdsOpt.has_value()) {
		logError("Cannot remove gate: no output IDs found for simulator_gate_id_t " + std::to_string(simulatorId), "LogicSimulator::removeGate");
		return;
	}
	const auto& outputIds = outputIdsOpt.value();

	for (const auto& outId : outputIds) {
		auto depIt = outputDependencies.find(outId);
		if (depIt != outputDependencies.end()) {
			assert(false);
			for (const auto& dependency : depIt->second) {
				auto depLocIt = gateLocations.find(dependency.gateId);
				if (depLocIt == gateLocations.end()) continue;

				const auto depType = depLocIt->second.gateType;
				const auto depIdx = depLocIt->second.gateIndex;
				switch (depType) {
				case SimGateType::AND:             if (depIdx < andGates.size())             andGates[depIdx].removeIdRefs(outId); break;
				case SimGateType::XOR:             if (depIdx < xorGates.size())             xorGates[depIdx].removeIdRefs(outId); break;
				case SimGateType::JUNCTION:        if (depIdx < junctions.size())            junctions[depIdx].removeIdRefs(outId); break;
				case SimGateType::BUFFER:          if (depIdx < buffers.size())              buffers[depIdx].removeIdRefs(outId); break;
				case SimGateType::SINGLE_BUFFER:   if (depIdx < singleBuffers.size())        singleBuffers[depIdx].removeIdRefs(outId); break;
				case SimGateType::TRISTATE_BUFFER: if (depIdx < tristateBuffers.size())      tristateBuffers[depIdx].removeIdRefs(outId); break;
				case SimGateType::CONSTANT:        if (depIdx < constantGates.size())        constantGates[depIdx].removeIdRefs(outId); break;
				case SimGateType::CONSTANT_RESET:  if (depIdx < constantResetGates.size())   constantResetGates[depIdx].removeIdRefs(outId); break;
				case SimGateType::COPY_SELF_OUTPUT:if (depIdx < copySelfOutputGates.size())  copySelfOutputGates[depIdx].removeIdRefs(outId); break;
				case SimGateType::PORTS_TO_INT:    if (depIdx < portsToIntGates.size())      portsToIntGates[depIdx].removeIdRefs(outId); break;
				}
			}
			outputDependencies.erase(depIt);
		}
		simulatorIdProvider.releaseId(outId);
		dirtySimulatorIds.push_back(outId);
	}

	SimGateType gateType = locationIt->second.gateType;
	size_t gateIndex = locationIt->second.gateIndex;

	auto fixMovedIndex = [&](auto& vec) {
		const size_t last = vec.size() - 1;
		if (gateIndex != last) {
			std::swap(vec[gateIndex], vec[last]);
			simulator_gate_id_t movedId = vec[gateIndex].getId();
			gateLocations[movedId].gateIndex = gateIndex;
		}
		vec.pop_back();
	};

	switch (gateType) {
	case SimGateType::AND:             if (!andGates.empty())             fixMovedIndex(andGates); break;
	case SimGateType::XOR:             if (!xorGates.empty())             fixMovedIndex(xorGates); break;
	case SimGateType::JUNCTION:        if (!junctions.empty())            fixMovedIndex(junctions); break;
	case SimGateType::BUFFER:          if (!buffers.empty())              fixMovedIndex(buffers); break;
	case SimGateType::SINGLE_BUFFER:   if (!singleBuffers.empty())        fixMovedIndex(singleBuffers); break;
	case SimGateType::TRISTATE_BUFFER: if (!tristateBuffers.empty())      fixMovedIndex(tristateBuffers); break;
	case SimGateType::CONSTANT:        if (!constantGates.empty())        fixMovedIndex(constantGates); break;
	case SimGateType::CONSTANT_RESET:  if (!constantResetGates.empty())   fixMovedIndex(constantResetGates); break;
	case SimGateType::COPY_SELF_OUTPUT:if (!copySelfOutputGates.empty())  fixMovedIndex(copySelfOutputGates); break;
	case SimGateType::PORTS_TO_INT:    if (!portsToIntGates.empty())      fixMovedIndex(portsToIntGates); break;
	}

	removeGateLocation(simulatorId);
}

void LogicSimulator::makeConnection(simulator_gate_id_t sourceId, connection_end_id_t sourcePort, simulator_gate_id_t destinationId, connection_end_id_t destinationPort) {
	if (getPortType(sourceId, sourcePort) == BlockData::ConnectionData::PortType::INPUT) {
		std::swap(sourceId, destinationId);
		std::swap(sourcePort, destinationPort);
	} else if (getPortType(destinationId, destinationPort) == BlockData::ConnectionData::PortType::OUTPUT) {
		std::swap(sourceId, destinationId);
		std::swap(sourcePort, destinationPort);
	}
	invalidateReplay();
	std::optional<simulator_gate_id_t> actualSourceId = getOutputPortId(sourceId, sourcePort);

	if (!actualSourceId.has_value()) {
		logError("Cannot resolve actual source ID for connection", "LogicSimulator::makeConnection");
		return;
	}

	addInputToGate(destinationId, actualSourceId.value(), destinationPort);
}

void LogicSimulator::removeConnection(simulator_gate_id_t sourceId, connection_end_id_t sourcePort, simulator_gate_id_t destinationId, connection_end_id_t destinationPort) {
	if (getPortType(sourceId, sourcePort) == BlockData::ConnectionData::PortType::INPUT) {
		std::swap(sourceId, destinationId);
		std::swap(sourcePort, destinationPort);
	} else if (getPortType(destinationId, destinationPort) == BlockData::ConnectionData::PortType::OUTPUT) {
		std::swap(sourceId, destinationId);
		std::swap(sourcePort, destinationPort);
	}
	invalidateReplay();
	std::optional<simulator_gate_id_t> actualSourceId = getOutputPortId(sourceId, sourcePort);
	if (!actualSourceId.has_value()) {
		logError("Cannot resolve actual source ID for disconnection", "LogicSimulator::removeConnection");
		return;
	}
	removeInputFromGate(destinationId, actualSourceId.value(), destinationPort);
}

void LogicSimulator::endEdit() {
	for (auto& gate : junctions) gate.doubleTick(statesA, statesB);
	regenerateJobs();
}

BlockData::ConnectionData::PortType LogicSimulator::getPortType(simulator_gate_id_t simulatorId, connection_end_id_t portId) const {
	auto locationIt = gateLocations.find(simulatorId);
	if (locationIt != gateLocations.end()) {
		SimGateType gateType = locationIt->second.gateType;
		size_t gateIndex = locationIt->second.gateIndex;
		switch (gateType) {
		case SimGateType::AND:
			if (gateIndex < andGates.size()) {
				return andGates[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::XOR:
			if (gateIndex < xorGates.size()) {
				return xorGates[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::JUNCTION:
			if (gateIndex < junctions.size()) {
				return junctions[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::BUFFER:
			if (gateIndex < buffers.size()) {
				return buffers[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::SINGLE_BUFFER:
			if (gateIndex < singleBuffers.size()) {
				return singleBuffers[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::TRISTATE_BUFFER:
			if (gateIndex < tristateBuffers.size()) {
				return tristateBuffers[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::CONSTANT:
			if (gateIndex < constantGates.size()) {
				return constantGates[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::CONSTANT_RESET:
			if (gateIndex < constantResetGates.size()) {
				return constantResetGates[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::COPY_SELF_OUTPUT:
			if (gateIndex < copySelfOutputGates.size()) {
				return copySelfOutputGates[gateIndex].getPortType(portId);
			}
			break;
		case SimGateType::PORTS_TO_INT:
			if (gateIndex < portsToIntGates.size()) {
				assert(false); // im going to say this gate needs to be killed
				// return portsToIntGates[gateIndex].getPortType(portId);
			}
			break;
		}
	}
	return BlockData::ConnectionData::PortType::NONE;
}

std::optional<simulator_gate_id_t> LogicSimulator::getOutputPortId(simulator_gate_id_t simulatorId, connection_end_id_t portId) const {
	auto locationIt = gateLocations.find(simulatorId);
	if (locationIt != gateLocations.end()) {
		SimGateType gateType = locationIt->second.gateType;
		size_t gateIndex = locationIt->second.gateIndex;

		switch (gateType) {
		case SimGateType::AND:
			if (gateIndex < andGates.size()) {
				return andGates[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::XOR:
			if (gateIndex < xorGates.size()) {
				return xorGates[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::JUNCTION:
			if (gateIndex < junctions.size()) {
				return junctions[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::BUFFER:
			if (gateIndex < buffers.size()) {
				return buffers[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::SINGLE_BUFFER:
			if (gateIndex < singleBuffers.size()) {
				return singleBuffers[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::TRISTATE_BUFFER:
			if (gateIndex < tristateBuffers.size()) {
				return tristateBuffers[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::CONSTANT:
			if (gateIndex < constantGates.size()) {
				return constantGates[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::CONSTANT_RESET:
			if (gateIndex < constantResetGates.size()) {
				return constantResetGates[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::COPY_SELF_OUTPUT:
			if (gateIndex < copySelfOutputGates.size()) {
				return copySelfOutputGates[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		case SimGateType::PORTS_TO_INT:
			if (gateIndex < portsToIntGates.size()) {
				return portsToIntGates[gateIndex].getIdOfOutputPort(portId);
			}
			break;
		}
	}

	return std::nullopt;
}

void LogicSimulator::addInputToGate(simulator_gate_id_t simulatorId, simulator_gate_id_t inputId, connection_end_id_t portId) {
	dirtySimulatorIds.push_back(inputId);
	auto locationIt = gateLocations.find(simulatorId);
	if (locationIt != gateLocations.end()) {
		SimGateType gateType = locationIt->second.gateType;
		size_t gateIndex = locationIt->second.gateIndex;

		switch (gateType) {
		case SimGateType::AND:
			if (gateIndex < andGates.size()) {
				andGates[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::XOR:
			if (gateIndex < xorGates.size()) {
				xorGates[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::JUNCTION:
			if (gateIndex < junctions.size()) {
				junctions[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::BUFFER:
			if (gateIndex < buffers.size()) {
				buffers[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::SINGLE_BUFFER:
			if (gateIndex < singleBuffers.size()) {
				singleBuffers[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::TRISTATE_BUFFER:
			if (gateIndex < tristateBuffers.size()) {
				tristateBuffers[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::CONSTANT:
			if (gateIndex < constantGates.size()) {
				constantGates[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::CONSTANT_RESET:
			if (gateIndex < constantResetGates.size()) {
				constantResetGates[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::COPY_SELF_OUTPUT:
			if (gateIndex < copySelfOutputGates.size()) {
				copySelfOutputGates[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::PORTS_TO_INT:
			if (gateIndex < portsToIntGates.size()) {
				portsToIntGates[gateIndex].addInput(inputId, portId);
				addOutputDependency(inputId, simulatorId);
			}
			break;
		}
		return;
	}

	logError("Gate not found for addInputToGate", "LogicSimulator::addInputToGate");
}

void LogicSimulator::removeInputFromGate(simulator_gate_id_t simulatorId, simulator_gate_id_t inputId, connection_end_id_t portId) {
	dirtySimulatorIds.push_back(inputId);
	auto locationIt = gateLocations.find(simulatorId);
	if (locationIt != gateLocations.end()) {
		SimGateType gateType = locationIt->second.gateType;
		size_t gateIndex = locationIt->second.gateIndex;

		switch (gateType) {
		case SimGateType::AND:
			if (gateIndex < andGates.size()) {
				andGates[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::XOR:
			if (gateIndex < xorGates.size()) {
				xorGates[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::JUNCTION:
			if (gateIndex < junctions.size()) {
				junctions[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::BUFFER:
			if (gateIndex < buffers.size()) {
				buffers[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::SINGLE_BUFFER:
			if (gateIndex < singleBuffers.size()) {
				singleBuffers[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::TRISTATE_BUFFER:
			if (gateIndex < tristateBuffers.size()) {
				tristateBuffers[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::CONSTANT:
			if (gateIndex < constantGates.size()) {
				constantGates[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::CONSTANT_RESET:
			if (gateIndex < constantResetGates.size()) {
				constantResetGates[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::COPY_SELF_OUTPUT:
			if (gateIndex < copySelfOutputGates.size()) {
				copySelfOutputGates[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		case SimGateType::PORTS_TO_INT:
			if (gateIndex < portsToIntGates.size()) {
				portsToIntGates[gateIndex].removeInput(inputId, portId);
				removeOutputDependency(inputId, simulatorId);
			}
			break;
		}
		return;
	}

	logError("Gate not found for removeInputFromGate", "LogicSimulator::removeInputFromGate");
}

std::optional<std::vector<simulator_gate_id_t>> LogicSimulator::getOutputSimulatorIdsFromGate(simulator_gate_id_t simulatorId) const {
	auto locationIt = gateLocations.find(simulatorId);
	if (locationIt == gateLocations.end()) return std::nullopt;

	SimGateType gateType = locationIt->second.gateType;
	size_t gateIndex = locationIt->second.gateIndex;

	switch (gateType) {
	case SimGateType::AND:
		if (gateIndex < andGates.size()) return andGates[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::XOR:
		if (gateIndex < xorGates.size()) return xorGates[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::JUNCTION:
		if (gateIndex < junctions.size()) return junctions[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::BUFFER:
		if (gateIndex < buffers.size()) return buffers[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::SINGLE_BUFFER:
		if (gateIndex < singleBuffers.size()) return singleBuffers[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::TRISTATE_BUFFER:
		if (gateIndex < tristateBuffers.size()) return tristateBuffers[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::CONSTANT:
		if (gateIndex < constantGates.size()) return constantGates[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::CONSTANT_RESET:
		if (gateIndex < constantResetGates.size()) return constantResetGates[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::COPY_SELF_OUTPUT:
		if (gateIndex < copySelfOutputGates.size()) return copySelfOutputGates[gateIndex].getOutputSimulatorIds();
		break;
	case SimGateType::PORTS_TO_INT:
		if (gateIndex < portsToIntGates.size()) return portsToIntGates[gateIndex].getOutputSimulatorIds();
		break;
	}
	return std::nullopt;
}

void LogicSimulator::updateGateLocation(simulator_gate_id_t gateId, SimGateType gateType, size_t gateIndex) {
	gateLocations[gateId] = GateLocation(gateType, gateIndex);
}

void LogicSimulator::removeGateLocation(simulator_gate_id_t gateId) {
	gateLocations.erase(gateId);
}

void LogicSimulator::addOutputDependency(simulator_gate_id_t outputId, simulator_gate_id_t dependentGateId) {
	outputDependencies[outputId].emplace_back(dependentGateId);
}

void LogicSimulator::removeOutputDependency(simulator_gate_id_t outputId, simulator_gate_id_t dependentGateId) {
	auto it = outputDependencies.find(outputId);
	if (it != outputDependencies.end()) {
		auto& deps = it->second;
		auto it2 = std::find(deps.begin(), deps.end(), GateDependency(dependentGateId));
		if (it2 != deps.end()) deps.erase(it2);
		if (deps.empty()) {
			outputDependencies.erase(it);
		}
	}
}

void LogicSimulator::regenerateJobs() {
	threadPool.waitForCompletion();
	jobInstructionStorage.clear();
	bool isRealistic = simulatorConfig.isRealistic();

	auto makeJI = [&](size_t start, size_t end) -> JobInstruction* {
		jobInstructionStorage.emplace_back(std::make_unique<JobInstruction>(JobInstruction{ this, start, end }));
		return jobInstructionStorage.back().get();
	};
	std::vector<ThreadPool::Job> allJobs;

	constexpr size_t batch = 256;

	for (size_t i = 0; i < andGates.size(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, andGates.size()));
		allJobs.push_back(ThreadPool::Job{ isRealistic ? &LogicSimulator::execANDRealistic : &LogicSimulator::execAND, ji });
	}
	for (size_t i = 0; i < xorGates.size(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, xorGates.size()));
		allJobs.push_back(ThreadPool::Job{ isRealistic ? &LogicSimulator::execXORRealistic : &LogicSimulator::execXOR, ji });
	}
	for (size_t i = 0; i < tristateBuffers.size(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, tristateBuffers.size()));
		allJobs.push_back(ThreadPool::Job{ isRealistic ? &LogicSimulator::execTristateRealistic : &LogicSimulator::execTristate, ji });
	}
	for (size_t i = 0; i < singleBuffers.size(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, singleBuffers.size()));
		allJobs.push_back(ThreadPool::Job{ isRealistic ? &LogicSimulator::execSingleBufferRealistic : &LogicSimulator::execSingleBuffer, ji });
	}
	for (size_t i = 0; i < constantResetGates.size(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, constantResetGates.size()));
		allJobs.push_back(ThreadPool::Job{ &LogicSimulator::execConstantReset, ji });
	}
	for (size_t i = 0; i < copySelfOutputGates.size(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, copySelfOutputGates.size()));
		allJobs.push_back(ThreadPool::Job{ &LogicSimulator::execCopySelfOutput, ji });
	}
	for (size_t i = 0; i < portsToIntGates.size(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, portsToIntGates.size()));
		allJobs.push_back(ThreadPool::Job{ &LogicSimulator::execPortsToInt, ji });
	}
	for (size_t i = 0; i < statesA.size().get(); i += batch) {
		JobInstruction* ji = makeJI(i, std::min(i + batch, static_cast<size_t>(statesA.size().get())));
		allJobs.push_back(ThreadPool::Job{ &LogicSimulator::saveReplayStates, ji });
	}
	unsigned int threadCount = min(allJobs.size(), simulatorConfig.getMaxThreadCount());
	if (threadCount == 0 && allJobs.size() != 0) { threadCount = 1; }
	jobs.clear();
	jobs.resize(threadCount);
	unsigned int threadIndex = 0;
	while (!allJobs.empty()) {
		jobs[threadIndex].emplace_back(allJobs.back());
		allJobs.pop_back();
		if (++threadIndex >= threadCount) threadIndex = 0;
	}
	updateThreadCount(threadCount);
	// logInfo("{} jobs created for the current round", "LogicSimulator::regenerateJobs", jobs.size());
}

void LogicSimulator::execAND(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->andGates[i].tick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execANDRealistic(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->andGates[i].realisticTick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execXOR(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->xorGates[i].tick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execXORRealistic(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->xorGates[i].realisticTick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execTristate(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->tristateBuffers[i].tick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execTristateRealistic(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->tristateBuffers[i].realisticTick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execSingleBuffer(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->singleBuffers[i].tick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execSingleBufferRealistic(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->singleBuffers[i].realisticTick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execConstantReset(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->constantResetGates[i].tick(ji->self->statesB);
}
void LogicSimulator::execCopySelfOutput(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->copySelfOutputGates[i].tick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::execPortsToInt(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	for (size_t i = ji->start; i < ji->end; ++i) ji->self->portsToIntGates[i].tick(ji->self->statesA, ji->self->statesB);
}
void LogicSimulator::saveReplayStates(void* jobInstruction) {
	auto* ji = static_cast<JobInstruction*>(jobInstruction);
	IdVector<simulator_gate_id_t, logic_state_t>& copyDestination = ji->self->statesReplay[ji->self->replayHead];
	for (size_t i = ji->start; i < ji->end; ++i) {
		copyDestination[simulator_gate_id_t(i)] = ji->self->statesA[simulator_gate_id_t(i)];
	}
}

bool LogicSimulator::stepBack() {
	if (!viewingReplay) {
		if (replayHead == replayTail) {
			return false;
		}
		viewingReplay = true;
		stateView = (replayHead - 1) % statesReplay.size();
		return true;
	}
	if (stateView == replayTail) {
		return false;
	}
	stateView = (stateView - 1) % statesReplay.size();
	return true;
}

bool LogicSimulator::stepForward() {
	if (!viewingReplay) {
		return false;
	}
	stateView = (stateView + 1) % statesReplay.size();
	if (stateView == replayHead) {
		viewingReplay = false;
	}
	return true;
}

bool LogicSimulator::skipBack() {
	if (!viewingReplay) {
		if (replayHead == replayTail) {
			return false;
		}
		viewingReplay = true;
		stateView = (replayHead - 1) % statesReplay.size();
	}
	if (stateView == replayTail) {
		return false;
	}
	while (stateView != replayTail) {
		stateView = (stateView - 1) % statesReplay.size();
		if (setStateUsed[(stateView + 1) % statesReplay.size()]) {
			return true;
		}
	}
	return true;
}

bool LogicSimulator::skipForward() {
	if (!viewingReplay) {
		return false;
	}
	while (stateView != replayHead) {
		stateView = (stateView + 1) % statesReplay.size();
		if (stateView == replayHead) {
			viewingReplay = false;
			return true;
		}
		if (setStateUsed[stateView]) {
			return true;
		}
	}
	return true;
}

nlohmann::json LogicSimulator::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	std::unique_lock<std::mutex> lock(mainDataMutex);
	nlohmann::json stateJson;
	stateJson["running"] = running.load();
	stateJson["pauseRequest"] = pauseRequest.load();
	stateJson["statesA"] = nlohmann::json::array();
	for (const simulator_gate_id_t id : statesA.ids()) {
		stateJson["statesA"].push_back(logicstate_to_string(statesA[id]));
	}
	stateJson["statesB"] = nlohmann::json::array();
	for (const simulator_gate_id_t id : statesB.ids()) {
		stateJson["statesB"].push_back(logicstate_to_string(statesB[id]));
	}
	stateJson["setStateUsed"] = setStateUsed;
	stateJson["replayHead"] = replayHead;
	stateJson["replayTail"] = replayTail;
	stateJson["viewingReplay"] = viewingReplay;
	stateJson["stateView"] = stateView;
	stateJson["pendingStateChanges"] = nlohmann::json::array();
	std::queue<StateChange> stateChangesCopy = pendingStateChanges;
	while (!stateChangesCopy.empty()) {
		const StateChange& sc = stateChangesCopy.front();
		stateJson["pendingStateChanges"].push_back(sc.dumpState());
		stateChangesCopy.pop();
	}
	stateJson["gates"] = nlohmann::json::object();
	nlohmann::json& gatesJson = stateJson["gates"];
	gatesJson["andGates"] = nlohmann::json::array();
	for (const auto& gate : andGates) gatesJson["andGates"].push_back(gate.dumpState());
	gatesJson["xorGates"] = nlohmann::json::array();
	for (const auto& gate : xorGates) gatesJson["xorGates"].push_back(gate.dumpState());
	gatesJson["junctions"] = nlohmann::json::array();
	for (const auto& gate : junctions) gatesJson["junctions"].push_back(gate.dumpState());
	gatesJson["buffers"] = nlohmann::json::array();
	for (const auto& gate : buffers) gatesJson["buffers"].push_back(gate.dumpState());
	gatesJson["tristateBuffers"] = nlohmann::json::array();
	for (const auto& gate : tristateBuffers) gatesJson["tristateBuffers"].push_back(gate.dumpState());
	gatesJson["constantGates"] = nlohmann::json::array();
	for (const auto& gate : constantGates) gatesJson["constantGates"].push_back(gate.dumpState());
	gatesJson["constantResetGates"] = nlohmann::json::array();
	for (const auto& gate : constantResetGates) gatesJson["constantResetGates"].push_back(gate.dumpState());
	gatesJson["copySelfOutputGates"] = nlohmann::json::array();
	for (const auto& gate : copySelfOutputGates) gatesJson["copySelfOutputGates"].push_back(gate.dumpState());
	gatesJson["portsToIntGates"] = nlohmann::json::array();
	for (const auto& gate : portsToIntGates) gatesJson["portsToIntGates"].push_back(gate.dumpState());
	stateJson["simulatorIdProvider"] = simulatorIdProvider.dumpState();
	stateJson["outputDependencies"] = nlohmann::json::object();
	for (const auto& [outputId, deps] : outputDependencies) {
		nlohmann::json depsJson = nlohmann::json::array();
		for (const auto& dep : deps) {
			depsJson.push_back(dep.dumpState());
		}
		stateJson["outputDependencies"][std::to_string(outputId.get())] = depsJson;
	}
	stateJson["gateLocations"] = nlohmann::json::object();
	for (const auto& [gateId, location] : gateLocations) {
		stateJson["gateLocations"][std::to_string(gateId.get())] = location.dumpState();
	}
	stateJson["averageTickrate"] = averageTickrate.load();
	stateJson["tickrateHalfLife"] = tickrateHalflife;
	return stateJson;
}

nlohmann::json LogicSimulator::StateChange::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	stateJson["simulatorId"] = id.get();
	stateJson["newState"] = logicstate_to_string(state);
	return stateJson;
}

nlohmann::json LogicSimulator::GateDependency::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	return gateId.get();
}

nlohmann::json LogicSimulator::GateLocation::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	stateJson["gateType"] = simgatetype_to_string(gateType);
	stateJson["gateIndex"] = gateIndex;
	return stateJson;
}
