#ifndef idVector_h
#define idVector_h

#include "id.h"

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

#endif /* idVector_h */
