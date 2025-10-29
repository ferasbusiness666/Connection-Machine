#include "kvStore.h"

std::unordered_map<std::string, std::weak_ptr<KVStore>> KVStore::storeInstances;
std::mutex KVStore::storeMutex;

std::shared_ptr<KVStore> KVStore::getStore(const std::string& storeName) {
	std::lock_guard<std::mutex> lock(storeMutex);
	if (auto it = storeInstances.find(storeName); it != storeInstances.end()) {
		if (auto sp = it->second.lock()) {
			return sp;
		}
	}

	auto sp = std::shared_ptr<KVStore>(new KVStore(storeName));
	storeInstances[storeName] = sp;

	return sp;
}

KVStore::KVStore(std::string storeName)
	: storeName(std::move(storeName)) {
}

KVStore::~KVStore() = default;

template<KVType kvType>
void KVStore::set(const std::string& key, const typename KVTypeToType<KVType(kvType)>::type& value) {
	std::lock_guard<std::mutex> lock(instanceMutex);
	store[key] = value;
}
template<KVType kvType>
std::optional<typename KVTypeToType<KVType(kvType)>::type>
KVStore::get(const std::string& key) const {
	std::lock_guard<std::mutex> lock(instanceMutex);
	auto it = store.find(key);
	if (it == store.end()) return std::nullopt;
	using T = typename KVTypeToType<KVType(kvType)>::type;
	if (const T* p = std::get_if<T>(&it->second)) return *p;
	logError("Type mismatch when getting key '{}' from KVStore '{}'", "KVStore", key, storeName);
	return std::nullopt;
}
