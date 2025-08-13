#pragma once

#include <bitset>

// for std::pair hash
// ref https://zhuanlan.zhihu.com/p/659182292
template <typename T>
inline void hashCombine(std::size_t &seed, const T &v)
{
    seed ^= std::hash<T>()(v) + 0x9E3779B9 + (seed << 6) + (seed >> 2);
}

template <typename U, typename V>
struct std::hash<std::pair<U, V>>
{
    size_t operator()(const std::pair<U, V> &tt) const
    {
        size_t seed = 0;
        hashCombine(seed, tt.first);
        hashCombine(seed, tt.second);
        return seed;
    }
};
