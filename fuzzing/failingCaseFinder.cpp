#include "fuzzTestcase.h"
#include "failingCaseFinder.h"
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"

std::unique_ptr<FuzzTestcase> FailingCaseFinder::findFailingCases(unsigned int maxAttempts) {
	for (unsigned int attempt = 0; attempt < maxAttempts; ++attempt) {
		logInfo("Attempting to generate failing testcase (attempt {}/{})", "", attempt + 1, maxAttempts);
		std::unique_ptr<FuzzTestcase> failingCase = tryMakeFailingCase();
		if (failingCase) {
			return failingCase;
		}
	}
	return nullptr;
}

namespace {

std::optional<connection_end_id_t> getRandomConnectionEnd(const BlockData* blockData, std::mt19937_64& gen, bool wantInput) {
	if (blockData->isDefaultData()) {
		if (wantInput) {
			return connection_end_id_t(0);
		} else {
			return connection_end_id_t(1);
		}
	}
	int numConnections = blockData->getBidirectionalConnectionCount().get();
	if (wantInput) numConnections += blockData->getInputConnectionCount().get();
	else numConnections += blockData->getOutputConnectionCount().get();
	if (numConnections == 0) return std::nullopt;
	std::uniform_int_distribution<int> dist(0, numConnections - 1);
	int index = dist(gen);
	const std::unordered_map<connection_end_id_t, BlockData::ConnectionData>& connections = blockData->getConnections();
	int i = 0;
	for (auto& pair : connections) {
		if (wantInput) {
			if (pair.second.portType == BlockData::ConnectionData::PortType::INPUT || pair.second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
				if (i == index) return pair.first;
				i++;
			}
		} else {
			if (pair.second.portType == BlockData::ConnectionData::PortType::OUTPUT || pair.second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
				if (i == index) return pair.first;
				i++;
			}
		}
	}
	return std::nullopt;
}

} // namespace

std::unique_ptr<FuzzTestcase> FailingCaseFinder::tryMakeFailingCase() {
	std::unique_ptr<FuzzTestcase> testcase = std::make_unique<FuzzTestcase>();
	Environment environment { false };
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	SharedCircuit circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	SharedEvaluator tEval = environment.getBackend().getEvaluator(evalId); // test evaluator
	SharedEvaluator rEval = nullptr; // reference evaluator

	std::mt19937_64 gen;
	std::uniform_int_distribution<int> distPos(-20, 20);
	gen.seed(std::random_device {}());
	bool runRealistic = (gen() % 2) == 0;
	int numEditOperations = 1000;
	int numTestOperations = 10;
	int numTicksBetweenTests = 3;
	int numStatesSetPerTest = 200;

	std::vector<block_id_t> blockIds;

	BlockDataManager& blockDataManager = environment.getBackend().getBlockDataManager();

	logInfo("Generating edit operations...", "FailingCaseFinder::tryMakeFailingCase");

	for (int i = 0; i < numEditOperations; ++i) {
		int operation = gen() % 8; // 0,1: place, 2: remove, 3,4,5: connect, 6,7: disconnect
		if (operation <= 1) { // place block
			Orientation orientation(Rotation(gen() % 4), (gen() % 2) == 0);
			BlockType blockType = BlockType(1 + (gen() % blockDataManager.maxBlockId()));
			if (!blockDataManager.blockExists(blockType)) continue;
			Position pos(distPos(gen), distPos(gen));
			bool success = circuit->tryInsertBlock(pos, orientation, blockType);
			if (!success) continue;
			testcase->addEditAction(PlaceBlockAction { pos, blockType, orientation });
			const Block* block = circuit->getBlockContainer().getBlock(pos);
			blockIds.push_back(block->id());
		} else if (operation <= 2) {
			if (blockIds.empty()) continue;
			std::uniform_int_distribution<size_t> distIndex(0, blockIds.size() - 1);
			size_t index = distIndex(gen);
			block_id_t blockId = blockIds[index];
			const Block* block = circuit->getBlockContainer().getBlock(blockId);
			if (block == nullptr) continue;
			Position pos = block->getPosition();
			bool success = circuit->tryRemoveBlock(pos);
			if (!success) continue;
			testcase->addEditAction(RemoveBlockAction { pos });
			std::swap(blockIds[index], blockIds.back());
			blockIds.pop_back();
		} else if (operation <= 5) { // connect
			if (blockIds.empty()) continue;
			block_id_t blockIdA = blockIds[gen() % blockIds.size()];
			block_id_t blockIdB = blockIds[gen() % blockIds.size()];
			const Block* blockA = circuit->getBlockContainer().getBlock(blockIdA);
			const Block* blockB = circuit->getBlockContainer().getBlock(blockIdB);
			if (blockA == nullptr || blockB == nullptr) continue;
			const BlockData* blockDataA = blockDataManager.getBlockData(blockA->type());
			const BlockData* blockDataB = blockDataManager.getBlockData(blockB->type());
			if (blockDataA == nullptr || blockDataB == nullptr) continue;
			std::optional<connection_end_id_t> connA = getRandomConnectionEnd(blockDataA, gen, false);
			std::optional<connection_end_id_t> connB = getRandomConnectionEnd(blockDataB, gen, true);
			if (!connA || !connB) continue;
			Position posA = blockA->getConnectionPosition(*connA).value();
			Position posB = blockB->getConnectionPosition(*connB).value();
			bool success = circuit->tryCreateConnection(posA, posB);
			if (!success) continue;
			testcase->addEditAction(CreateConnectionAction { posA, posB });
		} else if (operation <= 7) { // disconnect
			if (blockIds.empty()) continue;
			block_id_t blockIdA = blockIds[gen() % blockIds.size()];
			block_id_t blockIdB = blockIds[gen() % blockIds.size()];
			const Block* blockA = circuit->getBlockContainer().getBlock(blockIdA);
			const Block* blockB = circuit->getBlockContainer().getBlock(blockIdB);
			if (blockA == nullptr || blockB == nullptr) continue;
			const BlockData* blockDataA = blockDataManager.getBlockData(blockA->type());
			const BlockData* blockDataB = blockDataManager.getBlockData(blockB->type());
			if (blockDataA == nullptr || blockDataB == nullptr) continue;
			std::optional<connection_end_id_t> connA = getRandomConnectionEnd(blockDataA, gen, false);
			std::optional<connection_end_id_t> connB = getRandomConnectionEnd(blockDataB, gen, true);
			if (!connA || !connB) continue;
			Position posA = blockA->getConnectionPosition(*connA).value();
			Position posB = blockB->getConnectionPosition(*connB).value();
			bool success = circuit->tryRemoveConnection(posA, posB);
			if (!success) continue;
			testcase->addEditAction(RemoveConnectionAction { posA, posB });
		}
	}

	evaluator_id_t refEvalId = environment.getBackend().createEvaluator(circuitId).value();
	rEval = environment.getBackend().getEvaluator(refEvalId);
	tEval->setRealistic(runRealistic);
	rEval->setRealistic(runRealistic);
	testcase->setRealistic(runRealistic);
	tEval->resetStates();
	rEval->resetStates();
	std::vector<simulator_id_t> simulatorIdsTest;
	std::vector<simulator_id_t> simulatorIdsRef;
	std::unordered_map<block_id_t, Position> blockIdToPosition;

	logInfo("Mapping block IDs to positions...", "FailingCaseFinder::tryMakeFailingCase");

	for (block_id_t blockId : blockIds) {
		const Block* block = circuit->getBlockContainer().getBlock(blockId);
		if (block == nullptr) continue;
		Position pos = block->getPosition();
		blockIdToPosition[blockId] = pos;
		simulatorIdsTest.push_back(tEval->getBlockSimulatorId(pos));
		simulatorIdsRef.push_back(rEval->getBlockSimulatorId(pos));
		const BlockData* blockData = blockDataManager.getBlockData(block->type());
		if (blockData == nullptr) continue;
		if (blockData->isDefaultData()) {
			std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdTest = tEval->getPinSimulatorId(pos);
			std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdRef = rEval->getPinSimulatorId(pos);
			if (std::holds_alternative<simulator_id_t>(simIdTest) && std::holds_alternative<simulator_id_t>(simIdRef)) {
				simulatorIdsTest.push_back(std::get<simulator_id_t>(simIdTest));
				simulatorIdsRef.push_back(std::get<simulator_id_t>(simIdRef));
			} else if (std::holds_alternative<std::vector<simulator_id_t>>(simIdTest) && std::holds_alternative<std::vector<simulator_id_t>>(simIdRef)) {
				std::vector<simulator_id_t>& vecTest = std::get<std::vector<simulator_id_t>>(simIdTest);
				std::vector<simulator_id_t>& vecRef = std::get<std::vector<simulator_id_t>>(simIdRef);
				simulatorIdsTest.insert(simulatorIdsTest.end(), vecTest.begin(), vecTest.end());
				simulatorIdsRef.insert(simulatorIdsRef.end(), vecRef.begin(), vecRef.end());
			} else {
				return testcase;
			}
		} else {
			const std::unordered_map<connection_end_id_t, BlockData::ConnectionData>& connections = blockData->getConnections();
			for (const auto& [connectionId, connectionData] : connections) {
				if (connectionData.portType == BlockData::ConnectionData::PortType::INPUT) {
					continue;
				}
				std::optional<Position> portPositionOpt = block->getConnectionPosition(connectionId);
				Position portPosition = portPositionOpt.value();
				std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdTest = tEval->getPinSimulatorId(portPosition);
				std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdRef = rEval->getPinSimulatorId(portPosition);
				if (std::holds_alternative<simulator_id_t>(simIdTest) && std::holds_alternative<simulator_id_t>(simIdRef)) {
					simulatorIdsTest.push_back(std::get<simulator_id_t>(simIdTest));
					simulatorIdsRef.push_back(std::get<simulator_id_t>(simIdRef));
				} else if (std::holds_alternative<std::vector<simulator_id_t>>(simIdTest) && std::holds_alternative<std::vector<simulator_id_t>>(simIdRef)) {
					std::vector<simulator_id_t>& vecTest = std::get<std::vector<simulator_id_t>>(simIdTest);
					std::vector<simulator_id_t>& vecRef = std::get<std::vector<simulator_id_t>>(simIdRef);
					if (vecTest.size() != vecRef.size()){
						return testcase;
					}
					simulatorIdsTest.insert(simulatorIdsTest.end(), vecTest.begin(), vecTest.end());
					simulatorIdsRef.insert(simulatorIdsRef.end(), vecRef.begin(), vecRef.end());

				} else {
					return testcase;
				}
			}
		}
	}

	logInfo("Beginning test operations...", "FailingCaseFinder::tryMakeFailingCase");

	for (int i = 0; i < numTestOperations; ++i) {
		for (int j = 0; j < numStatesSetPerTest; ++j) {
			if (blockIds.empty()) continue;
			block_id_t blockId = blockIds[gen() % blockIds.size()];
			Position pos = blockIdToPosition.at(blockId);
			logic_state_t state = logic_state_t(gen() % 4);
			rEval->setState(pos, state);
			tEval->setState(pos, state);
			testcase->addTestAction(SetBlockStateAction { pos, state });
		}

		std::vector<logic_state_t> statesTest = tEval->getStatesFromSimulatorIds(simulatorIdsTest);
		std::vector<logic_state_t> statesRef = rEval->getStatesFromSimulatorIds(simulatorIdsRef);
		for (size_t k = 0; k < statesTest.size(); ++k) {
			if (statesTest[k] != statesRef[k]) {
				return testcase;
			}
		}
		tEval->tickStep(numTicksBetweenTests);
		rEval->tickStep(numTicksBetweenTests);
		testcase->addTestAction(TickEvalAction { static_cast<unsigned int>(numTicksBetweenTests) });
		statesTest = tEval->getStatesFromSimulatorIds(simulatorIdsTest);
		statesRef = rEval->getStatesFromSimulatorIds(simulatorIdsRef);
		for (size_t k = 0; k < statesTest.size(); ++k) {
			if (statesTest[k] != statesRef[k]) {
				return testcase;
			}
		}
	}

	return nullptr;
}
