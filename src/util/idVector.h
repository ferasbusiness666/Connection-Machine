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
    explicit IdVector(id_type n)                  : storage_(to_index_(n)) {}
    IdVector(id_type n, const T& value)           : storage_(to_index_(n), value) {}

    id_type size() const noexcept                { return id_type{to_rep_(storage_.size())}; }
    bool    empty() const noexcept               { return storage_.empty(); }

    id_type capacity() const noexcept            { return id_type{to_rep_(storage_.capacity())}; }

    void reserve(id_type n)                      { storage_.reserve(to_index_(n)); }
    void resize(id_type n)                       { storage_.resize(to_index_(n)); }
    void resize(id_type n, const T& value)       { storage_.resize(to_index_(n), value); }
    void resizeWithOffset(id_type n, int offset) { storage_.resize(to_index_(n) + offset); }
    void resizeWithOffset(id_type n, int offset, const T& value) { storage_.resize(to_index_(n) + offset, value); }

    void clear() noexcept                        { storage_.clear(); }

    T& operator[](id_type id)                    { return storage_[to_index_(id)]; }
    const T& operator[](id_type id) const        { return storage_[to_index_(id)]; }

    T& at(id_type id)                            { return storage_.at(to_index_(id)); }
    const T& at(id_type id) const                { return storage_.at(to_index_(id)); }

    id_type push_back(const T& v) {
        storage_.push_back(v);
        return last_id_();
    }
    id_type push_back(T&& v) {
        storage_.push_back(std::move(v));
        return last_id_();
    }
    template<class... Args>
    id_type emplace_back(Args&&... args) {
        storage_.emplace_back(std::forward<Args>(args)...);
        return last_id_();
    }

    id_type begin_id() const noexcept { return id_type(0); }
    id_type end_id()   const noexcept { return size(); }

    bool contains(id_type id) const noexcept {
        auto i = to_index_(id);
        return i < storage_.size();
    }

    IdRange<tag, rep> ids() const {
        return range(begin_id(), end_id());
    }

    auto begin() noexcept { return storage_.begin(); }
    auto end()   noexcept { return storage_.end(); }
    auto begin() const noexcept { return storage_.begin(); }
    auto end()   const noexcept { return storage_.end(); }
    const std::vector<T>& raw() const noexcept { return storage_; }
    std::vector<T>&       raw()       noexcept { return storage_; }

private:
    static std::size_t to_index_(id_type id) {
        if constexpr (std::is_signed_v<rep>) {
            auto v = id.get();
            return v < 0 ? static_cast<std::size_t>(0) : static_cast<std::size_t>(v);
        } else {
            return static_cast<std::size_t>(id.get());
        }
    }
    static rep to_rep_(std::size_t i) {
        return static_cast<rep>(i);
    }

    id_type last_id_() const {
        auto n = storage_.size();
        return id_type{to_rep_(n - 1)};
    }

    std::vector<T> storage_;
};

#endif /* idVector_h */
