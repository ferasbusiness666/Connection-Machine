#ifndef id_h
#define id_h

#define DECLARE_ID_TYPE(TypeName, RepType) \
	struct TagName ## __TAG__; \
	using TypeName = Id<TagName ## __TAG__, RepType>

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

    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = Id<Tag, Rep>;
        using difference_type   = std::ptrdiff_t;
        using reference         = Id<Tag, Rep>;
        using pointer           = void;

        iterator() = default;
        explicit iterator(Id<Tag, Rep> id) : x(id.get()) {}

        reference operator*()  const { return Id<Tag, Rep>(x); }
        iterator& operator++()    { ++x; return *this; }
        iterator  operator++(int) { auto t=*this; ++(*this); return t; }
        iterator& operator--()    { --x; return *this; }
        iterator  operator--(int) { auto t=*this; --(*this); return t; }

        iterator& operator+=(difference_type n) { x += n; return *this; }
        iterator& operator-=(difference_type n) { x -= n; return *this; }
        iterator  operator+(difference_type n) const { return *this+n; }
        iterator  operator-(difference_type n) const { return *this-n; }

        difference_type operator-(const iterator& o) const {
            return static_cast<difference_type>(x - o.x);
        }

        bool operator==(const iterator& o) const = default;
        auto operator<=>(const iterator& o) const = default;

    private:
		Rep x;
    };
private:
	Rep value{};
};

template<class Tag, class Rep>
class IdRange {
public:
	using iteratorerator = typename Id<Tag, Rep>::iteratorerator;
	IdRange(Id<Tag, Rep> beginId, Id<Tag, Rep> endId) : b(iteratorerator(beginId)), e(iteratorerator(endId)) {}
	iteratorerator begin() const { return b; }
	iteratorerator end()   const { return e; }
private:
	iteratorerator b, e;
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

#endif /* id_h */
