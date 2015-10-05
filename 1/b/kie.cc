#include <iostream>
#include <limits>

template<class T, class Acc>
void solve(size_t n) {
    Acc s{0};
    T min_odd{std::numeric_limits<T>::max()};
    using std::min;

    for(size_t i = 0; i < n; ++i) {
        T val;
        std::cin >> val;
        s += val;
        if(val % 2 == 1)
            min_odd = min(val, min_odd);
    }

    if(s % 2 == 0)
        std::cout << s << std::endl;
    else if(min_odd != s && min_odd != std::numeric_limits<T>::max())
        std::cout << s - min_odd << std::endl;
    else
        std::cout << "NIESTETY" << std::endl;
}

int main() {
    std::ios_base::sync_with_stdio(false);
    size_t n;
    std::cin >> n;
    solve<int, long long>(n);
}
