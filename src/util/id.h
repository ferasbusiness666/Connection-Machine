#ifndef id_h
#define id_h
#include <fmt/core.h>

#define DECLARE_ID_TYPE(TypeName, TagName, RepType) \
	struct TagName; \
	using TypeName = Id<TagName, RepType>

template<class Tag, class Rep>
class Id {
public:
	using tag = Tag;
	using rep = Rep;

	constexpr Id() = default;
	explicit constexpr Id(Rep v) : value(v) {}

	constexpr Rep get() const noexcept { return value; }

	friend constexpr bool operator==(Id, Id) = default;
	friend constexpr auto operator<=>(const Id&, const Id&) = default;

    class id_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = Id<Tag, Rep>;
        using difference_type   = std::ptrdiff_t;
        using reference         = Id<Tag, Rep>;
        using pointer           = void;

        id_iterator() = default;
        explicit id_iterator(Id<Tag, Rep> id) : x(id.get()) {}

        reference operator*()  const { return Id<Tag, Rep>(x); }
        id_iterator& operator++()    { ++x; return *this; }
        id_iterator  operator++(int) { auto t=*this; ++(*this); return t; }
        id_iterator& operator--()    { --x; return *this; }
        id_iterator  operator--(int) { auto t=*this; --(*this); return t; }

        id_iterator& operator+=(difference_type n) { x += n; return *this; }
        id_iterator& operator-=(difference_type n) { x -= n; return *this; }
        id_iterator  operator+(difference_type n) const { return *this+n; }
        id_iterator  operator-(difference_type n) const { return *this-n; }

        difference_type operator-(const id_iterator& o) const {
            return static_cast<difference_type>(x - o.x);
        }

        bool operator==(const id_iterator& o) const = default;
        auto operator<=>(const id_iterator& o) const = default;

    private:
		Rep x;
    };
private:
	Rep value{};
};

template<class Tag, class Rep>
class IdRange {
public:
	using id_iterator = typename Id<Tag, Rep>::id_iterator;
	IdRange(Id<Tag, Rep> beginId, Id<Tag, Rep> endId) : b(id_iterator(beginId)), e(id_iterator(endId)) {}
	id_iterator begin() const { return b; }
	id_iterator end()   const { return e; }
private:
	id_iterator b, e;
};

template<class Tag, class Rep>
IdRange<Tag, Rep> range(Id<Tag, Rep> beginId, Id<Tag, Rep> endId) {
	return IdRange<Tag, Rep>(beginId, endId);
}

namespace std {
	template<class Tag, class Rep>
	struct hash<Id<Tag, Rep>> {
		size_t operator()(const Id<Tag, Rep>& id) const
			noexcept(noexcept(std::hash<Rep>{}(id.get()))) {
			return std::hash<Rep>{}(id.get());
		}
	};
	template<class Tag, class Rep>
	string to_string(const Id<Tag, Rep>& id) {
		return std::to_string(id.get());
	}
};

namespace fmt {
	template<class Tag, class Rep>
	struct formatter<Id<Tag, Rep>> : formatter<Rep> {
		template<typename FormatContext>
		auto format(const Id<Tag, Rep>& id, FormatContext& ctx) const {
			return formatter<Rep>::format(id.get(), ctx);
		}
	};
}


template<class>
struct is_id : std::false_type {};

template<class Tag, class Rep>
struct is_id<Id<Tag, Rep>> : std::true_type {};

template<class T>
inline constexpr bool is_id_v = is_id<std::remove_cvref_t<T>>::value;

template<class>
struct id_traits;

template<class Tag, class Rep>
struct id_traits<Id<Tag, Rep>> {
	using tag = Tag;
	using rep = Rep;
};

template<class T>
concept IdType = is_id_v<T>;

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

template<IdType IdT, class T>
class IdVector {
public:
    using id_type = std::remove_cv_t<IdT>;
    using tag = typename id_traits<id_type>::tag;
    using rep = typename id_traits<id_type>::rep;
    using value_type = T;

    static_assert(std::is_integral_v<rep>, "Id::rep must be an integral type");

    constexpr IdVector() = default;
    explicit IdVector(id_type n)                  : storage_(rep(n)) {}
    IdVector(id_type n, const T& value)           : storage_(rep(n), value) {}

    inline id_type size() const noexcept                { return id_type{rep(storage_.size())}; }
    inline bool    empty() const noexcept               { return storage_.empty(); }

    inline id_type capacity() const noexcept            { return id_type{rep(storage_.capacity())}; }

    inline void reserve(id_type n)                      { storage_.reserve(n.get()); }
    inline void resize(id_type n)                       { storage_.resize(n.get()); }
    inline void resize(id_type n, const T& value)       { storage_.resize(n.get(), value); }
    inline void resizeWithOffset(id_type n, int offset) { storage_.resize(n.get() + offset); }
    inline void resizeWithOffset(id_type n, int offset, const T& value) { storage_.resize(n.get() + offset, value); }

    inline void clear() noexcept                        { storage_.clear(); }

    inline T& operator[](id_type id)                    { return storage_[id.get()]; }
    inline const T& operator[](id_type id) const        { return storage_[id.get()]; }

    inline T& at(id_type id)                            { return storage_.at(id.get()); }
    inline const T& at(id_type id) const                { return storage_.at(id.get()); }

    inline id_type push_back(const T& v) {
        storage_.push_back(v);
        return last_id_();
    }
    inline id_type push_back(T&& v) {
        storage_.push_back(std::move(v));
        return last_id_();
    }
    template<class... Args>
    inline id_type emplace_back(Args&&... args) {
        storage_.emplace_back(std::forward<Args>(args)...);
        return last_id_();
    }

    inline id_type begin_id() const noexcept { return id_type(0); }
    inline id_type end_id()   const noexcept { return size(); }

    inline bool contains(id_type id) const noexcept {
        return id.get() < storage_.size();
    }

    inline IdRange<tag, rep> ids() const {
        return range(begin_id(), end_id());
    }

    inline auto begin() noexcept { return storage_.begin(); }
    inline auto end()   noexcept { return storage_.end(); }
    inline auto begin() const noexcept { return storage_.begin(); }
    inline auto end()   const noexcept { return storage_.end(); }
    inline const std::vector<T>& raw() const noexcept { return storage_; }
    inline std::vector<T>&       raw()       noexcept { return storage_; }

private:
    inline id_type last_id_() const {
        auto n = storage_.size();
        return id_type{rep(n - 1)};
    }

    std::vector<T> storage_;
};

#endif /* id_h */
