#ifndef id_h
#define id_h
#include <fmt/core.h>

template<class Tag, class Rep = std::uint32_t>
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
		auto format(const Id<Tag, Rep>& id, FormatContext& ctx) {
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
