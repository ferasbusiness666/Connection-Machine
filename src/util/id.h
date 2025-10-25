#ifndef id_h
#define id_h

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
private:
	Rep value{};
};

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