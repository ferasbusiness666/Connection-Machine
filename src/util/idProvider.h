#ifndef idProvider_h
#define idProvider_h

#include "id.h"

template<IdType IdT>
class IdProvider {
public:
	using id_type = std::remove_cv_t<IdT>;
	using rep = typename id_traits<id_type>::rep;

	constexpr IdProvider(rep initialValue) : nextId(initialValue) {}

	constexpr id_type getNewId() {
		if (unusedIds.empty()) {
			return id_type(nextId++);
		} else {
			rep id = *unusedIds.begin();
			unusedIds.erase(unusedIds.begin());
			return id_type(id);
		}
	}
	constexpr id_type getNewId(id_type preferredId) {
		if (unusedIds.size() * 2 < nextId && unusedIds.contains(preferredId.get())) {
			unusedIds.erase(preferredId.get());
			return preferredId;
		}
		if (preferredId.get() == nextId || unusedIds.empty()) {
			return id_type(nextId++);
		} else {
			rep id = *unusedIds.begin();
			unusedIds.erase(unusedIds.begin());
			return id_type(id);
		}
	}
	constexpr void releaseId(id_type id) {
		if (!isIdUsed(id)) {
			return;
		}
		unusedIds.insert(id.get());
	}
	constexpr bool isIdUsed(id_type id) const {
		return id.get() < nextId && !unusedIds.contains(id.get());
	}
	constexpr id_type peekNext() const {
		return id_type(nextId);
	}
	void reset() {
		nextId = 0;
		unusedIds.clear();
	}
	std::vector<id_type> getUsedIds() const {
		std::vector<id_type> usedIds;
		for (rep id = 0; id < nextId; ++id) {
			if (!unusedIds.contains(id)) {
				usedIds.push_back(id_type(id));
			}
		}
		return usedIds;
	}
private:
	rep nextId;
	std::set<rep> unusedIds;
};

template<IdType IdT>
class LinearIdProvider {
public:
	using id_type = std::remove_cv_t<IdT>;
	using rep = typename id_traits<id_type>::rep;

	constexpr LinearIdProvider(rep initialValue) : nextId(initialValue) {}

	constexpr id_type getNewId() {
		return id_type(nextId++);
	}
	constexpr id_type peekNext() const {
		return id_type(nextId);
	}
	constexpr id_type lastIdProvided() const {
		return idType(nextId - 1);
	}
	void reset() {
		nextId = 0;
	}

private:
	rep nextId;
};

#endif /* idProvider_h */
