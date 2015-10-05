#include <array>
#include <cmath>
#include <cinttypes>
#include <cstdint>
#include <iostream>
#include <cassert>

#ifdef TESTS
constexpr const bool tests = true;
#else
constexpr const bool tests = false;
#endif

// Matrices
template<class T, unsigned n, unsigned m>
using mat = std::array<std::array<T, m>, n>;

template<class T>
using mat2x2 = mat<T, 2, 2>;

template<class T>
struct defs {
    static constexpr const mat2x2<T> zero{{ {{0, 0}}, {{0, 0}} }};
    static constexpr const mat2x2<T> id  {{ {{1, 0}}, {{0, 1}} }};
    static constexpr const mat2x2<T> fib {{ {{1, 1}}, {{1, 0}} }};
};

template<class T>
constexpr const mat2x2<T> defs<T>::zero;

template<class T>
constexpr const mat2x2<T> defs<T>::id;

template<class T>
constexpr const mat2x2<T> defs<T>::fib;

// Helper multiplication functions
template<class T>
struct doubled {
    static constexpr const int type = 0;
};

template<>
struct doubled<int8_t> {
    using type = int16_t;
};

template<>
struct doubled<uint8_t> {
    using type = uint16_t;
};

template<>
struct doubled<int16_t> {
    using type = int32_t;
};

template<>
struct doubled<uint16_t> {
    using type = uint32_t;
};

template<>
struct doubled<int32_t> {
    using type = int64_t;
};

template<>
struct doubled<uint32_t> {
    using type = uint64_t;
};

#ifdef __SIZEOF_INT128__
template<>
struct doubled<int64_t> {
    using type = __int128_t;
};

template<>
struct doubled<uint64_t> {
    using type = __uint128_t;
};
#endif

template<class T, class U = typename doubled<T>::type>
__attribute__((const))
constexpr T multiply(T lhs, T rhs, T mod) {
    return static_cast<T>((static_cast<U>(lhs) * rhs) % mod);
}

template<class T, int U = doubled<T>::type>
__attribute__((const))
T multiply(T lhs, T rhs, T mod) {
    lhs %= mod;
    rhs %= mod;

    T result = 0;

    while(lhs) {
        if(lhs % 2 == 1) {
            result = (result + rhs);
            if(result >= mod)
                result -= mod;
        }

        lhs /= 2;
        rhs *= 2;
        if(rhs >= mod)
            rhs -= mod;
    }

    return result;
}

template<class T>
__attribute__((pure))
mat2x2<T> multiply(const mat2x2<T> &lhs, const mat2x2<T> &rhs, const T mod) {
    auto result = defs<T>::zero;

    for(int i = 0; i < 2; ++i)
        for(int k = 0; k < 2; ++k)
            for(int j = 0; j < 2; ++j)
                result[i][j] += multiply(lhs[i][k], rhs[k][j], mod);

    for(int i = 0; i < 2; ++i)
        for(int j = 0; j < 2; ++j)
            result[i][j] %= mod;

    return result;
}

template<class T, class Idx=uint64_t>
__attribute__((pure))
T fib(Idx idx, const T mod) {
    auto res = defs<T>::id;
    auto base = defs<T>::fib;
    
    if(idx == 0)
        return 0;

    --idx;

    while(idx) {
        if(idx % 2 == 1)
            res = multiply(res, base, mod);
        base = multiply(base, base, mod);
        idx /= 2;
    }

    return (res[1][0] + res[1][1]) % mod;
}

template<class T, class Idx=uint64_t>
void lift(const Idx idx, const T pattern, const T target, const T power, const Idx lower_period) {
    if(power > target) {
        T ans = idx + 6 * target;
        assert(fib(ans, target) == pattern);
        throw ans; // quasi-CPS :)
    }

    for(Idx i = 0; i < 10; ++i) {
        auto candidx = idx + i * lower_period;
        auto cand = fib(candidx, power);
        if(cand % power == pattern % power)
            lift(candidx, pattern, target, power * 10, lower_period * 10);
    }
}

template<class T>
void solve(const T pattern, const T target) {
    try {
        for(int i = 0; i < 60; ++i) {
            auto cand = fib(i, 10u);
            if(cand % 10 == pattern % 10)
                lift<T, uint64_t>(i, pattern, target, 100, 60);
        }
    } catch(T result) {
        if(tests)
            std::cout << "TAK" << std::endl;
        else
            std::cout << result << std::endl;
        return;
    }

    std::cout << "NIE" << std::endl;
}

int main() {
    std::ios_base::sync_with_stdio(false);

    std::string str;
    while(std::cin >> str) {
        int64_t power = 1;
        for(size_t i = 0; i < str.size(); ++i)
            power *= 10;

        auto pattern = std::stoll(str);
        solve<uint64_t>(pattern, power);
    }
}
