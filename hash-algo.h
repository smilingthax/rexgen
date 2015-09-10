#ifndef _HASH_ALGO_H
#define _HASH_ALGO_H

// adapted from N3980

class fnv1a
{
    static constexpr std::size_t prime_ = sizeof(size_t)<64 ? 16777619u : 1099511628211u;
    static constexpr std::size_t offset_ = sizeof(size_t)<64 ? 2166136261u : 14695981039346656037u;

    std::size_t state_ = offset_;
public:
    using result_type = std::size_t;

    void
    operator()(void const* key, std::size_t len) noexcept
    {
        unsigned char const* p = static_cast<unsigned char const*>(key);
        unsigned char const* const e = p + len;
        for (; p < e; ++p)
            state_ = (state_ ^ *p) * prime_;
    }

    explicit
    operator result_type() noexcept
    {
        return state_;
    }
};

#endif
