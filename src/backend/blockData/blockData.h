#ifndef blockData_h
#define blockData_h

#include "backend/container/block/connectionEnd.h"
#include "backend/dataUpdateEventManager.h"
#include "backend/position/position.h"
#include "util/bidirectionalMultiSecondKeyMap.h"

class BlockData {
	friend class BlockDataManager;
public:
	struct ConnectionData {
		ConnectionData(Vector positionOnBlock, bool isInput, unsigned int bitWidth = 1) : positionOnBlock(positionOnBlock), isInput(isInput), bitWidth(bitWidth) {}
		Vector positionOnBlock = Vector(0, 0);
		bool isInput = true;
		unsigned int bitWidth = 1;
	};

	BlockData(BlockType blockType, DataUpdateEventManager* dataUpdateEventManager);

	inline void sendBlockDataUpdate() { dataUpdateEventManager->sendEvent("blockDataUpdate"); }

	void setDefaultData(bool defaultData) noexcept;
	inline bool isDefaultData() const noexcept { return defaultData; }

	inline void setPrimitive(bool primitive) noexcept {
		this->primitive = primitive;
		sendBlockDataUpdate();
	}
	inline bool isPrimitive() const noexcept { return primitive; }

	void setSize(Size size) noexcept;
	inline Size getSize() const noexcept { return blockSize; }
	inline Size getSize(Orientation orientation) const noexcept { return orientation * blockSize; }

	inline BlockType getBlockType() const { return blockType; }

	inline void setIsPlaceable(bool placeable) noexcept {
		this->placeable = placeable;
		sendBlockDataUpdate();
	}
	inline bool isPlaceable() const noexcept { return placeable; }

	void setName(const std::string& name) noexcept;
	inline void setPath(const std::string& path) noexcept {
		this->path = path;
		sendBlockDataUpdate();
	}
	inline const std::string& getName() const noexcept { return name; }
	inline const std::string& getPath() const noexcept { return path; }

	// trys to set a connection input in the block. Returns success.
	void removeConnection(connection_end_id_t connectionId) noexcept;
	void setConnectionInput(Vector positionOnBlock, connection_end_id_t connectionId) noexcept;
	// trys to set a connection output in the block. Returns success.
	void setConnectionOutput(Vector positionOnBlock, connection_end_id_t connectionId) noexcept;

	inline std::optional<connection_end_id_t> getInputConnectionId(Vector positionOnBlock) const noexcept {
		if (defaultData) {
			if (positionOnBlock.dx == 0 && positionOnBlock.dy == 0) return 0;
			return std::nullopt;
		}
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == positionOnBlock && pair.second.isInput) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<connection_end_id_t> getOutputConnectionId(Vector positionOnBlock) const noexcept {
		if (defaultData) {
			if (positionOnBlock.dx == 0 && positionOnBlock.dy == 0) return 1;
			return std::nullopt;
		}
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == positionOnBlock && !pair.second.isInput) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<connection_end_id_t> getInputConnectionId(Vector vector, Orientation orientation) const noexcept {
		if (defaultData) {
			if (vector.dx == 0 && vector.dy == 0) return 0;
			return std::nullopt;
		}
		Vector noOrientationVec = orientation.inverseTransformVectorWithArea(vector, orientation*blockSize);
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == noOrientationVec && pair.second.isInput) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<connection_end_id_t> getOutputConnectionId(Vector vector, Orientation orientation) const noexcept {
		if (defaultData) {
			if (vector.dx == 0 && vector.dy == 0) return 1;
			return std::nullopt;
		}
		Vector noOrientationVec = orientation.inverseTransformVectorWithArea(vector, orientation*blockSize);
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == noOrientationVec && !pair.second.isInput) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<Vector> getConnectionVector(connection_end_id_t connectionId) const noexcept {
		if (defaultData) {
			if (connectionId <= 1) return Vector(0);
			return std::nullopt;
		}
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return std::nullopt;
		return iter->second.positionOnBlock;
	}
	inline std::optional<Vector> getConnectionVector(connection_end_id_t connectionId, Orientation orientation) const noexcept {
		if (defaultData) {
			if (connectionId <= 1) return Vector(0);
			return std::nullopt;
		}
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return std::nullopt;
		return orientation.transformVectorWithArea(iter->second.positionOnBlock, blockSize);
	}
	inline connection_end_id_t getConnectionCount() const noexcept {
		if (defaultData) return 2;
		return connections.size();
	}
	inline connection_end_id_t getInputConnectionCount() const noexcept {
		if (defaultData) return 1;
		return inputConnectionCount;
	}
	inline connection_end_id_t getOutputConnectionCount() const noexcept {
		if (defaultData) return 1;
		return connections.size() - inputConnectionCount;
	}
	inline bool connectionExists(connection_end_id_t connectionId) const noexcept { return defaultData ? connectionId <= 1 : connections.contains(connectionId); }
	inline bool isConnectionInput(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return connectionId == 0;
		auto iter = connections.find(connectionId);
		return iter != connections.end() && iter->second.isInput;
	}
	inline bool isConnectionOutput(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return connectionId == 1;
		auto iter = connections.find(connectionId);
		return iter != connections.end() && !(iter->second.isInput);
	}
	const std::unordered_map<connection_end_id_t, ConnectionData>& getConnections() const noexcept {
		assert((!defaultData) && "this will be empty if defaultData is true");
		return connections;
	}
	void setConnectionIdName(connection_end_id_t connectionId, const std::string& name);
	inline std::optional<std::string> getConnectionIdToName(connection_end_id_t connectionId) const {
		if (defaultData) return connectionId == 1 ? "Out" : "In";
		const std::string* str = connectionIdNames.get(connectionId);
		if (str) return *str;
		return std::nullopt;
	}
	inline connection_end_id_t getConnectionBitWidth(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return 1;
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return 0;
		return iter->second.bitWidth;
	}
	inline void setConnectionBitWidth(connection_end_id_t connectionId, unsigned int bitWidth) noexcept {
		if (bitWidth == 0) {
			logError("Cant set the bit width of a connection to 0", "BlockData");
		}
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return;
		iter->second.bitWidth = bitWidth;
	}

private:
	BlockType blockType;
	bool defaultData = true;
	bool primitive = true; // true if defined by default (And, Or, Xor...)
	bool placeable = true;
	std::string name = "Unnamed Block";
	std::string path = "Basic";
	Size blockSize = Size(1);
	connection_end_id_t inputConnectionCount = 0;
	std::unordered_map<connection_end_id_t, ConnectionData> connections;
	BidirectionalMultiSecondKeyMap<connection_end_id_t, std::string> connectionIdNames;
	DataUpdateEventManager* dataUpdateEventManager;
};

#endif /* blockData_h */
