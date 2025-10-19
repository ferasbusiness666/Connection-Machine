#ifndef blockData_h
#define blockData_h

#include "backend/container/block/connectionEnd.h"
#include "backend/dataUpdateEventManager.h"
#include "backend/position/position.h"
#include "util/bidirectionalMultiSecondKeyMap.h"
#include "util/vec2.h"

class BlockData {
	friend class BlockDataManager;
public:
	struct ConnectionData {
		enum PortType {
			INPUT,
			OUTPUT,
			BIDIRECTIONAL,
			NONE
		};
		ConnectionData(
			Vector positionOnBlock,
			PortType portType,
			std::variant<unsigned int, std::vector<unsigned int>> bitConfiguration = static_cast<unsigned int>(1)
		) :
			positionOnBlock(positionOnBlock),
			portType(portType),
			bitConfiguration(bitConfiguration) {}
		Vector positionOnBlock = Vector(0, 0);
		PortType portType = PortType::INPUT;
		std::variant<unsigned int, std::vector<unsigned int>> bitConfiguration;
		unsigned int getBitWidth() const noexcept {
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				return std::get<unsigned int>(bitConfiguration);
			} else {
				return std::get<std::vector<unsigned int>>(bitConfiguration).size();
			}
		}
		std::vector<unsigned int> getLaneIds() const noexcept {
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				std::vector<unsigned int> vec;
				for (unsigned int i = 0; i < std::get<unsigned int>(bitConfiguration); i++) {
					vec.push_back(i);
				}
				return vec;
			} else {
				return std::get<std::vector<unsigned int>>(bitConfiguration);
			}
		}
		unsigned int getFirstLaneId() const noexcept {
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				return 0;
			} else {
				return std::get<std::vector<unsigned int>>(bitConfiguration).at(0);
			}
		}
		// TODO: improve the performance of the below functions by improving the data structure used to store bitConfiguration
		unsigned int getLaneId(unsigned int index) const noexcept {
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				assert(index < std::get<unsigned int>(bitConfiguration));
				return index;
			} else {
				return std::get<std::vector<unsigned int>>(bitConfiguration).at(index);
			}
		}
		unsigned int getMaxLaneId() const noexcept {
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				return std::get<unsigned int>(bitConfiguration) - 1;
			} else {
				return std::get<std::vector<unsigned int>>(bitConfiguration).back();
			}
		}
		bool containsLaneId(unsigned int laneId) const noexcept {
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				return laneId < std::get<unsigned int>(bitConfiguration);
			} else {
				auto& vec = std::get<std::vector<unsigned int>>(bitConfiguration);
				return std::find(vec.begin(), vec.end(), laneId) != vec.end();
			}
		}
		unsigned int getIndexOfLaneId(unsigned int laneId) const noexcept {
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				assert(laneId < std::get<unsigned int>(bitConfiguration));
				return laneId;
			} else {
				auto& vec = std::get<std::vector<unsigned int>>(bitConfiguration);
				auto iter = std::find(vec.begin(), vec.end(), laneId);
				assert(iter != vec.end());
				return std::distance(vec.begin(), iter);
			}
		}
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

	void setIsBus(bool bus) noexcept {
		this->bus = bus;
		sendBlockDataUpdate();
	}
	inline bool isBus() const noexcept { return bus; }

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
	inline const std::string& getName() const noexcept { return name; }
	void setPath(const std::string& path) noexcept;
	inline const std::string& getPath() const noexcept { return path; }
	void setTexturePath(const std::string& texturePath) noexcept;
	inline const std::string& getTexturePath() const noexcept { return texturePath; }
	void setTextureTileSize(Vec2Int tileSize) noexcept;
	Vec2Int getTextureTileSize() const noexcept { return textureTileSize; }
	void setTextureSmallestCordTile(Vec2Int smallestCordTile) noexcept;
	Vec2Int getTextureSmallestCordTile() const noexcept { return textureSmallestCordTile; }
	void setTextureBlockTileSize(Vec2Int blockSizeInTiles) noexcept;
	Vec2Int getTextureBlockTileSize() const noexcept { return textureBlockTileSize; }
	void setTextureBlockStateOffset(Vec2Int textureBlockStateOffset) noexcept;
	Vec2Int getTextureBlockStateOffset() const noexcept { return textureBlockStateOffset; }
	void setUsesTileMapTexture(bool usesTileMapTexture) noexcept;
	bool getUsesTileMapTexture() const noexcept { return usesTileMapTexture; }

	void removeConnection(connection_end_id_t connectionId) noexcept;
	// trys to set a input connection in the block.
	void setConnectionInput(Vector positionOnBlock, connection_end_id_t connectionId) noexcept;
	// trys to set a output connection in the block.
	void setConnectionOutput(Vector positionOnBlock, connection_end_id_t connectionId) noexcept;
	// trys to set a bidirectional connection in the block.
	void setConnectionBidirectional(Vector positionOnBlock, connection_end_id_t connectionId) noexcept;

	inline std::optional<connection_end_id_t> getInputConnectionId(Vector positionOnBlock) const noexcept {
		if (defaultData) {
			if (positionOnBlock.dx == 0 && positionOnBlock.dy == 0) return 0;
			return std::nullopt;
		}
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == positionOnBlock && pair.second.portType == ConnectionData::PortType::INPUT) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<connection_end_id_t> getOutputConnectionId(Vector positionOnBlock) const noexcept {
		if (defaultData) {
			if (positionOnBlock.dx == 0 && positionOnBlock.dy == 0) return 1;
			return std::nullopt;
		}
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == positionOnBlock && pair.second.portType == ConnectionData::PortType::OUTPUT) return pair.first;
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
			if (pair.second.positionOnBlock == noOrientationVec && pair.second.portType == ConnectionData::PortType::INPUT) return pair.first;
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
			if (pair.second.positionOnBlock == noOrientationVec && pair.second.portType == ConnectionData::PortType::OUTPUT) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<connection_end_id_t> getBidirectionalConnectionId(Vector vector, Orientation orientation) const noexcept {
		if (defaultData) {
			if (vector.dx == 0 && vector.dy == 0) return 1;
			return std::nullopt;
		}
		Vector noOrientationVec = orientation.inverseTransformVectorWithArea(vector, orientation*blockSize);
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == noOrientationVec && pair.second.portType == ConnectionData::PortType::BIDIRECTIONAL) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<connection_end_id_t> getInputOrBidirectionalConnectionId(Vector vector, Orientation orientation) const noexcept {
		if (defaultData) {
			if (vector.dx == 0 && vector.dy == 0) return 1;
			return std::nullopt;
		}
		Vector noOrientationVec = orientation.inverseTransformVectorWithArea(vector, orientation*blockSize);
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == noOrientationVec && pair.second.portType != ConnectionData::PortType::OUTPUT) return pair.first;
		}
		return std::nullopt;
	}
	inline std::optional<connection_end_id_t> getOutputOrBidirectionalConnectionId(Vector vector, Orientation orientation) const noexcept {
		if (defaultData) {
			if (vector.dx == 0 && vector.dy == 0) return 1;
			return std::nullopt;
		}
		Vector noOrientationVec = orientation.inverseTransformVectorWithArea(vector, orientation*blockSize);
		for (auto& pair : connections) {
			if (pair.second.positionOnBlock == noOrientationVec && pair.second.portType != ConnectionData::PortType::INPUT) return pair.first;
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
		return outputConnectionCount;
	}
	inline connection_end_id_t getBidirectionalConnectionCount() const noexcept {
		if (defaultData) return 0;
		return connections.size() - inputConnectionCount - outputConnectionCount;
	}
	inline bool connectionExists(connection_end_id_t connectionId) const noexcept { return defaultData ? connectionId <= 1 : connections.contains(connectionId); }
	inline bool isConnectionInput(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return connectionId == 0;
		auto iter = connections.find(connectionId);
		return iter != connections.end() && iter->second.portType == ConnectionData::PortType::INPUT;
	}
	inline bool isConnectionOutput(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return connectionId == 1;
		auto iter = connections.find(connectionId);
		return iter != connections.end() && iter->second.portType == ConnectionData::PortType::OUTPUT;
	}
	inline bool isConnectionBidirectional(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return false;
		auto iter = connections.find(connectionId);
		return iter != connections.end() && iter->second.portType == ConnectionData::PortType::BIDIRECTIONAL;
	}
	inline ConnectionData::PortType getConnectionPortType(connection_end_id_t connectionId) const noexcept {
		if (defaultData) {
			if (connectionId == 0) return ConnectionData::PortType::INPUT;
			if (connectionId == 1) return ConnectionData::PortType::OUTPUT;
			return ConnectionData::PortType::NONE;
		}
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return ConnectionData::PortType::NONE;
		return iter->second.portType;
	}
	const std::unordered_map<connection_end_id_t, ConnectionData>& getConnections() const noexcept {
		assert((!defaultData) && "this will be empty if defaultData is true");
		return connections;
	}
	const ConnectionData* getConnectionData(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return nullptr;
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return nullptr;
		return &iter->second;
	}
	void setConnectionIdName(connection_end_id_t connectionId, const std::string& name);
	inline std::optional<std::string> getConnectionIdToName(connection_end_id_t connectionId) const {
		if (defaultData) return connectionId == 1 ? "Out" : "In";
		const std::string* str = connectionIdNames.get(connectionId);
		if (str) return *str;
		return std::nullopt;
	}
	inline unsigned int getConnectionBitWidth(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return 1;
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return 0;
		return iter->second.getBitWidth();
	}
	inline const std::variant<unsigned int, std::vector<unsigned int>>* getConnectionBitConfiguration(connection_end_id_t connectionId) const noexcept {
		if (defaultData) return nullptr;
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return nullptr;
		return &iter->second.bitConfiguration;
	}
	inline const std::vector<unsigned int> getPortsWithLaneId(unsigned int laneId) const noexcept {
		assert(isBus() && "Only bus blocks have lane ids");
		std::vector<unsigned int> ports; // TODO: make this not linear search
		for (auto& pair : connections) {
			auto laneIds = pair.second.getLaneIds();
			if (std::find(laneIds.begin(), laneIds.end(), laneId) != laneIds.end()) {
				ports.push_back(pair.first);
			}
		}
		return ports;
	}
	inline unsigned int getLaneCount() const noexcept {
		assert(isBus() && "Only bus blocks have lane counts");
		unsigned int laneCount = 0;
		for (auto& pair : connections) {
			laneCount = std::max(laneCount, pair.second.getMaxLaneId() + 1);
		}
		return laneCount;
	}
	inline void setConnectionBitConfiguration(connection_end_id_t connectionId, std::variant<unsigned int, std::vector<unsigned int>> bitConfiguration) noexcept {
		if (std::holds_alternative<std::vector<unsigned int>>(bitConfiguration) && std::get<std::vector<unsigned int>>(bitConfiguration).empty()) {
			logError("Cant set the bit configuration of a connection to be empty", "BlockData");
		} else if (std::holds_alternative<unsigned int>(bitConfiguration) && std::get<unsigned int>(bitConfiguration) == 0) {
			logError("Cant set the bit configuration of a connection to be 0", "BlockData");
		}
		auto iter = connections.find(connectionId);
		if (iter == connections.end()) return;
		iter->second.bitConfiguration = bitConfiguration;
	}
	inline bool hasBlockState() const noexcept {
		if (defaultData) return true;
		return (primitive) && !(bus);
	}

private:
	BlockType blockType;
	bool defaultData = true;
	bool primitive = true; // true if defined by default (And, Or, Xor...)
	bool placeable = true;
	bool bus = false;
	std::string name = "Unnamed Block";
	std::string path = "Basic";
	std::string texturePath = "";
	bool usesTileMapTexture = false;
	Vec2Int textureTileSize = {0, 0}; // mean that the whole texture is 1 tile.
	Vec2Int textureSmallestCordTile = {0, 0};
	Vec2Int textureBlockTileSize = {1, 1};
	Vec2Int textureBlockStateOffset = {1, 1};
	Size blockSize = Size(1);
	connection_end_id_t inputConnectionCount = 0;
	connection_end_id_t outputConnectionCount = 0;
	std::unordered_map<connection_end_id_t, ConnectionData> connections;
	BidirectionalMultiSecondKeyMap<connection_end_id_t, std::string> connectionIdNames;
	DataUpdateEventManager* dataUpdateEventManager;
};

#endif /* blockData_h */
