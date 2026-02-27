#ifndef id_h
#define id_h

#define DECLARE_ID_TYPE(TypeName, RepType)                                                                                                                               \
	struct TypeName##__TAG__;                                                                                                                                            \
	using TypeName = Id<TypeName##__TAG__, RepType>

template <class Tag, class Rep>
class Id {
public:
	using tag = Tag;
	using rep = Rep;

	constexpr Id() = default;
	constexpr Id(Rep v) : value(v) { }

	constexpr Rep get() const noexcept { return value; }

	friend constexpr bool operator==(Id, Id) = default;
	friend constexpr auto operator<=>(const Id&, const Id&) = default;

	class iterator {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = Id<Tag, Rep>;
		using difference_type = std::ptrdiff_t;
		using reference = Id<Tag, Rep>;
		using pointer = void;

		iterator() = default;
		explicit iterator(Id<Tag, Rep> id) : x(id.get()) { }

		reference operator*() const { return Id<Tag, Rep>(x); }
		iterator& operator++() {
			++x;
			return *this;
		}
		iterator operator++(int) {
			auto t = *this;
			++(*this);
			return t;
		}
		iterator& operator--() {
			--x;
			return *this;
		}
		iterator operator--(int) {
			auto t = *this;
			--(*this);
			return t;
		}

		iterator& operator+=(difference_type n) {
			x += n;
			return *this;
		}
		iterator& operator-=(difference_type n) {
			x -= n;
			return *this;
		}
		iterator operator+(difference_type n) const { return iterator(this->x + n); }
		iterator operator-(difference_type n) const { return iterator(this->x - n); }

		difference_type operator-(const iterator& o) const { return static_cast<difference_type>(x - o.x); }

		bool operator==(const iterator& o) const = default;
		auto operator<=>(const iterator& o) const = default;

	private:
		Rep x;
	};
private:
	Rep value{};
};

template <class Tag, class Rep>
class IdRange {
public:
	using iterator = typename Id<Tag, Rep>::iterator;
	IdRange(Id<Tag, Rep> beginId, Id<Tag, Rep> endId) : b(iterator(beginId)), e(iterator(endId)) { }
	iterator begin() const { return b; }
	iterator end() const { return e; }
private:
	iterator b, e;
};

template <class Tag, class Rep>
IdRange<Tag, Rep> range(Id<Tag, Rep> beginId, Id<Tag, Rep> endId) {
	return IdRange<Tag, Rep>(beginId, endId);
}

namespace std {
template <class Tag, class Rep>
struct hash<Id<Tag, Rep>> {
	size_t operator()(const Id<Tag, Rep>& id) const noexcept(noexcept(std::hash<Rep>{}(id.get()))) { return std::hash<Rep>{}(id.get()); }
};
template <class Tag, class Rep>
string to_string(const Id<Tag, Rep>& id) {
	return std::to_string(id.get());
}
}; // namespace std

namespace fmt {
template <class Tag, class Rep>
struct formatter<Id<Tag, Rep>> : formatter<Rep> {
	template <typename FormatContext>
	auto format(const Id<Tag, Rep>& id, FormatContext& ctx) const /* GCOVR_EXCL_FUNCTION */ {
		return formatter<Rep>::format(id.get(), ctx);
	}
};
} // namespace fmt

template <class>
struct is_id : std::false_type { };

template <class Tag, class Rep>
struct is_id<Id<Tag, Rep>> : std::true_type { };

template <class T>
inline constexpr bool is_id_v = is_id<std::remove_cvref_t<T>>::value;

template <class>
struct id_traits;

template <class Tag, class Rep>
struct id_traits<Id<Tag, Rep>> {
	using tag = Tag;
	using rep = Rep;
};

template <class T>
concept IdType = is_id_v<T>;

template <IdType IdT>
class IdProvider {
public:
	using id_type = std::remove_cv_t<IdT>;
	using rep = typename id_traits<id_type>::rep;

	constexpr IdProvider(rep initialValue) : nextId(initialValue), initialValue(initialValue) { }

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
		if (unusedIds.contains(preferredId.get())) {
			unusedIds.erase(preferredId.get());
			return preferredId;
		}
		if (preferredId.get() == nextId) return id_type(nextId++);
		return getNewId();
	}
	constexpr void releaseId(id_type id) {
		if (!isIdUsed(id)) {
			return;
		}
		unusedIds.insert(id.get());
	}
	constexpr bool isIdUsed(id_type id) const { return id.get() < nextId && id.get() >= initialValue && !unusedIds.contains(id.get()); }
	constexpr id_type peekNext() const { return id_type(nextId); }
	void reset() {
		nextId = initialValue;
		unusedIds.clear();
	}
	std::vector<id_type> getUsedIds() const {
		std::vector<id_type> usedIds;
		for (rep id = initialValue; id < nextId; ++id) {
			if (!unusedIds.contains(id)) {
				usedIds.push_back(id_type(id));
			}
		}
		return usedIds;
	}
	nlohmann::json dumpState() const /* GCOVR_EXCL_FUNCTION */ {
		nlohmann::json stateJson;
		stateJson["nextId"] = nextId;
		stateJson["initialValue"] = initialValue;
		stateJson["unusedIds"] = nlohmann::json::array();
		for (const rep id : unusedIds) {
			stateJson["unusedIds"].push_back(id);
		}
		return stateJson;
	}
private:
	rep nextId;
	rep initialValue;
	std::set<rep> unusedIds;
};

template <IdType IdT>
class LinearIdProvider {
public:
	using id_type = std::remove_cv_t<IdT>;
	using rep = typename id_traits<id_type>::rep;

	constexpr LinearIdProvider(rep initialValue) : nextId(initialValue) { }

	constexpr id_type getNewId() { return id_type(nextId++); }
	constexpr id_type peekNext() const { return id_type(nextId); }
	constexpr id_type lastIdProvided() const { return id_type(nextId - 1); }
	void reset() { nextId = 0; }

private:
	rep nextId;
};

template <IdType IdT, class T>
class IdVector {
public:
	using id_type = std::remove_cv_t<IdT>;
	using tag = typename id_traits<id_type>::tag;
	using rep = typename id_traits<id_type>::rep;
	using value_type = T;

	static_assert(std::is_integral_v<rep>, "Id::rep must be an integral type");

	constexpr IdVector() = default;
	explicit IdVector(id_type n) : storage(rep(n)) { }
	IdVector(id_type n, const T& value) : storage(rep(n), value) { }

	inline id_type size() const noexcept { return id_type{ rep(storage.size()) }; }
	inline bool empty() const noexcept { return storage.empty(); }

	inline id_type capacity() const noexcept { return id_type{ rep(storage.capacity()) }; }

	inline void reserve(id_type n) { storage.reserve(n.get()); }
	inline void resize(id_type n) { storage.resize(n.get()); }
	inline void resize(id_type n, const T& value) { storage.resize(n.get(), value); }
	inline void resizeWithOffset(id_type n, int offset) { storage.resize(n.get() + offset); }
	inline void resizeWithOffset(id_type n, int offset, const T& value) { storage.resize(n.get() + offset, value); }

	inline void smartResize(id_type n) { // exponential growth strategy
		if (n.get() > storage.size()) {
			size_t newSize = storage.size() == 0 ? 1 : storage.size();
			while (newSize < n.get()) {
				newSize *= 2;
			}
			storage.resize(newSize);
		}
	}
	inline void smartResize(id_type n, T& value) {
		if (n.get() > storage.size()) {
			size_t oldSize = storage.size();
			size_t newSize = oldSize == 0 ? 1 : oldSize;
			while (newSize < n.get()) {
				newSize *= 2;
			}
			storage.resize(newSize, value);
		}
	}
	inline void smartResizeWithOffset(id_type n, int offset) {
		if (n.get() + offset > storage.size()) {
			size_t newSize = storage.size() == 0 ? 1 : storage.size();
			while (newSize < n.get() + offset) {
				newSize *= 2;
			}
			storage.resize(newSize);
		}
	}
	inline void smartResizeWithOffset(id_type n, int offset, const T& value) {
		if (n.get() + offset > storage.size()) {
			size_t oldSize = storage.size();
			size_t newSize = oldSize == 0 ? 1 : oldSize;
			while (newSize < n.get() + offset) {
				newSize *= 2;
			}
			storage.resize(newSize, value);
		}
	}

	inline void clear() noexcept { storage.clear(); }

	inline T& operator[](id_type id) { return storage[id.get()]; }
	inline const T& operator[](id_type id) const { return storage[id.get()]; }

	inline T& at(id_type id) { return storage.at(id.get()); }
	inline const T& at(id_type id) const { return storage.at(id.get()); }

	inline id_type push_back(const T& v) {
		storage.push_back(v);
		return last_id_();
	}
	inline id_type push_back(T&& v) {
		storage.push_back(std::move(v));
		return last_id_();
	}
	template <class... Args>
	inline id_type emplace_back(Args&&... args) {
		storage.emplace_back(std::forward<Args>(args)...);
		return last_id_();
	}

	inline id_type begin_id() const noexcept { return id_type(0); }
	inline id_type end_id() const noexcept { return size(); }

	inline bool contains(id_type id) const noexcept { return id.get() < storage.size(); }

	inline IdRange<tag, rep> ids() const { return range(begin_id(), end_id()); }

	inline auto begin() noexcept { return storage.begin(); }
	inline auto end() noexcept { return storage.end(); }
	inline auto begin() const noexcept { return storage.begin(); }
	inline auto end() const noexcept { return storage.end(); }
	inline const std::vector<T>& raw() const noexcept { return storage; }
	inline std::vector<T>& raw() noexcept { return storage; }

private:
	inline id_type last_id_() const {
		auto n = storage.size();
		return id_type{ rep(n - 1) };
	}

	std::vector<T> storage;
};

#define NO_ID_MAP
#ifndef NO_ID_MAP
template <IdType IdT, class T>
class IdMap {
public:
	static const unsigned int BlockSize = 1024;

	using id_type = std::remove_cv_t<IdT>;
	using tag = typename id_traits<id_type>::tag;
	using rep = typename id_traits<id_type>::rep;
	using value_type = std::pair<id_type, T>;

	struct reference {
		const id_type first;
		T& second;
	};

	struct const_reference {
		const id_type first;
		const T& second;
	};

	struct pointer {
		reference ref;
		reference* operator->() { return &ref; }
	};

	struct const_pointer {
		const_reference ref;
		const_reference* operator->() { return &ref; }
	};

	static_assert(std::is_integral_v<rep>, "Id::rep must be an integral type");

	class iterator {
		friend class IdMap<IdT, T>;
	public:
		iterator& operator++() {
			if (index == std::numeric_limits<rep>::max()) return *this;
			// search rest of page
			const std::pair<rep, std::vector<std::optional<T>>>& page = idMap->storage[index / BlockSize];
			assert(!page.second.empty());
			unsigned int relIndex = index - page.first;
			if (page.second.size() - 1 == relIndex) { // at end of block
				size_t blockIndex = index / BlockSize + 1;
				// search blocks
				while (blockIndex < idMap->storage.size()) {
					if (!idMap->storage[blockIndex].second.empty()) { // if we find a non empty block the first item will be valid
						index = idMap->storage[blockIndex].first;
						return *this;
					}
					blockIndex += 1;
				}
				// at end
				index = std::numeric_limits<rep>::max();
				return *this;
			}
			// if not at end of block then there is more data in this block
			do {
				relIndex++;
				assert(page.second.size() > relIndex);
			} while (page.second[relIndex] == std::nullopt);
			index = page.first + relIndex;
			return *this;
		}
		iterator operator++(int) {
			iterator tmp = *this;
			this->operator++();
			return tmp;
		}
		reference operator*() const {
			std::pair<rep, std::vector<std::optional<T>>>& page = idMap->storage[index / BlockSize];
			return reference{ index, page.second[index - page.first].value() };
		}
		pointer operator->() const { return pointer{ operator*() }; }
		bool operator==(const iterator other) const {
			assert(this->idMap == other.idMap);
			return this->index == other.index;
		}
	private:
		iterator(IdMap<IdT, T>& idMap, rep index) : idMap(&idMap), index(index) {}
		IdMap<IdT, T>* idMap;
		rep index;
	};
	class const_iterator {
		friend class IdMap<IdT, T>;
	public:
		const_iterator& operator++() {
			if (index == std::numeric_limits<rep>::max()) return *this;
			// search rest of page
			const std::pair<rep, std::vector<std::optional<T>>>& page = idMap->storage[index / BlockSize];
			assert(!page.second.empty());
			unsigned int relIndex = index - page.first;
			if (page.second.size() - 1 == relIndex) { // at end of block
				size_t blockIndex = index / BlockSize + 1;
				// search blocks
				while (blockIndex < idMap->storage.size()) {
					if (!idMap->storage[blockIndex].second.empty()) { // if we find a non empty block the first item will be valid
						index = idMap->storage[blockIndex].first;
						return *this;
					}
					blockIndex += 1;
				}
				// at end
				index = std::numeric_limits<rep>::max();
				return *this;
			}
			// if not at end of block then there is more data in this block
			do {
				relIndex++;
				assert(page.second.size() > relIndex);
			} while (page.second[relIndex] == std::nullopt);
			index = page.first + relIndex;
			return *this;
		}
		const_iterator operator++(int) {
			const_iterator tmp = *this;
			this->operator++();
			return tmp;
		}
		const_reference operator*() const {
			const std::pair<rep, std::vector<std::optional<T>>>& page = idMap->storage[index / BlockSize];
			return const_reference{ index, page.second[index - page.first].value() };
		}
		const_pointer operator->() const { return const_pointer{ operator*() }; }
		bool operator==(const const_iterator other) const {
			assert(this->idMap == other.idMap);
			return this->index == other.index;
		}
	private:
		const_iterator(const IdMap<IdT, T>& idMap, rep index) : idMap(&idMap), index(index) {}
		const IdMap<IdT, T>* idMap;
		rep index;
	};

	constexpr IdMap() = default;

	inline size_t size() const noexcept { return cur_size; }
	inline bool empty() const noexcept { return cur_size == 0; }

	inline void clear() noexcept { storage.clear(); }

	inline T& operator[](id_type id) { return *emplaceWithKey(id).first; }

	inline T& at(id_type id) { return *find(id); }
	inline const T& at(id_type id) const { return *find(id); }

	inline bool contains(id_type id) const noexcept { return find(id) != end(); }

	std::pair<iterator, bool> insert(const value_type& value) { return emplaceWithKey(value.first, value.second); }
	// iterator insert(iterator pos, const value_type& value) { return emplaceWithKey(pos.index, value.second); }
	// iterator insert(const_iterator pos, const value_type& value) { return emplaceWithKey(pos.index, value.second); }
	template <class... Args>
	std::pair<iterator, bool> try_emplace(id_type id, Args&&... args) { return emplaceWithKey(id, std::forward<Args>(args)...); }

	iterator erase(iterator pos) { // assume iter valid
		cur_size--;
		rep index = pos.index;
		auto& page = storage[index / BlockSize];
		if (index == page.first) { // start of page
			if (page.second.size() == 1) {
				if (storage.size() - 1 == index / BlockSize) {
					storage.resize(storage.size() - 1);
					return end();
				}
				++pos;
				page.second.clear();
				return pos;
			}
			++pos;
			assert(pos != end());
			std::move(page.second.begin() + (pos.index - page.first), page.second.end(), page.second.begin());
			page.second.resize(page.second.size() - (pos.index - index));
			page.first = pos.index;
			return pos;
		}
		++pos;
		rep relIndex = index - page.first;
		if (relIndex == page.second.size() - 1) { // end of page
			do {
				assert(relIndex != 0);
				--relIndex;
			} while(!page.second[relIndex].has_value());
			page.second.resize(relIndex + 1);
			return pos;
		}
		// in page
		assert(pos != end());
		// std::move(page.second.begin() + (pos.index - page.first), page.second.end(), page.second.begin() + relIndex);
		// page.second.resize(page.second.size() - (pos.index - index));
		page.second[relIndex] = std::nullopt;
		return pos;
	}

	inline iterator find(id_type id) {
		rep index = id.get();
		if (storage.size() <= index / BlockSize) return end();
		const auto& page = storage[index / BlockSize];
		if (page.second.empty() || page.first > index || page.first + page.second.size() <= index) return end();
		if (page.second[index - page.first].has_value()) return iterator(*this, index);
		return end();
	}
	inline const_iterator find(id_type id) const {
		rep index = id.get();
		if (storage.size() <= index / BlockSize) return end();
		const auto& page = storage[index / BlockSize];
		if (page.second.empty() || page.first > index || page.first + page.second.size() <= index) return end();
		if (page.second[index - page.first].has_value()) return const_iterator(*this, index);
		return end();
	}

	inline iterator begin() noexcept {
		for (unsigned int blockIndex = 0; blockIndex < storage.size(); blockIndex++) {
			const auto& page = storage[blockIndex];
			if (!page.second.empty()) {
				return iterator(*this, page.first); // guaranteed to be a value
			}
		}
		return end();
	}
	inline iterator end() noexcept { return iterator(*this, std::numeric_limits<rep>::max()); }
	inline const_iterator begin() const noexcept {
		for (unsigned int blockIndex = 0; blockIndex < storage.size(); blockIndex++) {
			const auto& page = storage[blockIndex];
			if (!page.second.empty()) {
				return const_iterator(*this, page.first); // guaranteed to be a value
			}
		}
		return end();
	}
	inline const_iterator end() const noexcept { return const_iterator(*this, std::numeric_limits<rep>::max()); }

private:
	template <class... Args>
	std::pair<iterator, bool> emplaceWithKey(id_type id, Args&&... args) {
		rep index = id.get();
		if (storage.size() <= index / BlockSize) {
			storage.resize(index / BlockSize + 1);
			auto& page = storage[index / BlockSize];
			page.first = index;
			page.second.emplace_back(std::in_place, std::forward<Args>(args)...);
			cur_size++;
			return { iterator(*this, index), true };
		}
		auto& page = storage[index / BlockSize];
		if (page.second.empty()) {
			page.first = index;
			page.second.emplace_back(std::in_place, std::forward<Args>(args)...);
			cur_size++;
			return { iterator(*this, index), true };
		}
		if (page.first > index) {
			size_t sizeBefore = page.second.size();
			size_t newSpaceNeeded = page.first - index;
			page.second.resize(newSpaceNeeded + sizeBefore);
			std::move_backward(page.second.begin(), page.second.begin() + sizeBefore, page.second.end());
			page.second[0].emplace(std::forward<Args>(args)...);
			std::fill(page.second.begin() + 1, page.second.begin() + newSpaceNeeded, std::nullopt);
			page.first = index;
			cur_size++;
			return { iterator(*this, index), true };
		}
		size_t relIndex = index - page.first;
		if (relIndex >= page.second.size()) {
			page.second.resize(relIndex + 1, std::nullopt);
			page.second[relIndex].emplace(std::forward<Args>(args)...);
			cur_size++;
			return { iterator(*this, index), true };
		}
		if (page.second[relIndex] != std::nullopt) return { iterator(*this, index), false };
		page.second[relIndex].emplace(std::forward<Args>(args)...);
		cur_size++;
		return { iterator(*this, index), true };
	}

	inline id_type last_id_() const {
		auto n = storage.size();
		return id_type{ rep(n - 1) };
	}

	size_t cur_size = 0;
	std::vector<std::pair<rep, std::vector<std::optional<T>>>> storage;
};
#else
#define IdMap std::unordered_map
#endif

#endif /* id_h */
