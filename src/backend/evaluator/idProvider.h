#ifndef idProvider_h
#define idProvider_h

template<typename T>
class IdProvider {
public:
	IdProvider() : nextId(0) {}

	inline T getNewId() {
		if (unusedIds.empty()) {
			return nextId++;
		} else {
			T id = *unusedIds.begin();
			unusedIds.erase(unusedIds.begin());
			return id;
		}
	}
	inline T getNewId(T preferredId) {
		if (unusedIds.size() * 2 < nextId && unusedIds.contains(preferredId)) {
			unusedIds.erase(preferredId);
			return preferredId;
		}
		if (preferredId == nextId || unusedIds.empty()) {
			return nextId++;
		} else {
			T id = *unusedIds.begin();
			unusedIds.erase(unusedIds.begin());
			return id;
		}
	}
	inline void releaseId(T id) {
		if (id > nextId || unusedIds.contains(id)) {
			return;
		}
		unusedIds.insert(id);
	}
	inline bool isIdUsed(T id) const {
		return id < nextId && !unusedIds.contains(id);
	}
	inline T getNextId() const {
		return nextId;
	}
	inline void reset() {
		nextId = 0;
		unusedIds.clear();
	}
	inline std::vector<T> getUsedIds() const {
		std::vector<T> usedIds;
		for (T id = 0; id < nextId; ++id) {
			if (!unusedIds.contains(id)) {
				usedIds.push_back(id);
			}
		}
		return usedIds;
	}
private:
	T nextId;
	std::set<T> unusedIds;
};

#endif /* idProvider_h */
