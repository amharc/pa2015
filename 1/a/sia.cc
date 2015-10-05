#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stack>
#include <vector>

//#define DEBUG

#ifdef DEBUG
constexpr const bool debug = true;
#else
constexpr const bool debug = false;
#endif

template<class Height = long long, class Time = long long,
         class Rate = long long, class Position = size_t>
class solver_t {
    public:
    void solve() && {
        prepare();

        for(int i = 0; i < m; ++i) {
            read_line();
        }
    }

    private:
    Position n;
    int m;
    std::vector<Rate> growth_rate;
    std::vector<Rate> partial_sums;

    struct block_t {
        Position begin;
        Time since;
        Height level;

        block_t(const Position &begin, const Time &since, const Height &level)
        : begin{begin}
        , since{since}
        , level{level}
        { }

        friend std::ostream& operator<<(std::ostream &str, const block_t &block) {
            return str << "Block([" << block.begin << "..] since " << block.since
                       << " at level " << block.level << ")";
        }
    };

    std::stack<block_t> blocks;

    void prepare() {
        std::cin >> n >> m;
        growth_rate.emplace_back(0);

        std::copy_n(
            std::istream_iterator<Height>(std::cin),
            n,
            std::back_inserter(growth_rate)
        );

        std::sort(
            std::begin(growth_rate),
            std::end(growth_rate)
        );

        std::partial_sum(
            std::begin(growth_rate),
            std::end(growth_rate),
            std::back_inserter(partial_sums)
        );

        blocks.emplace(1, 0, 0);
    }

    Height height_at(const Position &idx, const Time &day, const block_t &block) const {
        return block.level + (day - block.since) * growth_rate[idx];
    }

    Height height_sum(const Position &left, const Position &right, const Time &day,
            const block_t &block, const Height &above = 0) const { // [left..right]
        static_assert(std::is_signed<Height>::value, "Height must be signed");
        auto base = static_cast<Height>((right - left + 1) * (block.level - above));
        auto rates = static_cast<Height>(partial_sums[right] - partial_sums[left - 1]);
        return base + rates * (day - block.since);
    }

    Position first_not_less_than(const Height &level, const Time &day, const block_t &block,
            const Position &end) const {
        Position left{block.begin}, right{end};

        while(left != right) {
            auto mid = (left + right) / 2;
            
            if(height_at(mid, day, block) < level) {
                left = mid + 1;
            }
            else {
                right = mid;
            }
        }

        return left;
    }

    void read_line() {
        Time day;
        Height level;
        std::cin >> day >> level;

        Height sum{0};
        auto end = n + 1;

        while(!blocks.empty()) {
            const auto &block = blocks.top();
            auto idx = first_not_less_than(level, day, block, end);
            auto gain = height_sum(idx, end - 1, day, block, level);
            sum += gain;
            end = idx;

            if(debug) {
                std::cerr << day << " " << level << ": " << block << " -> "
                          << idx << ", gain: " << gain << std::endl;
            }

            if(idx <= block.begin) {
                blocks.pop();
            }
            else {
                break;
            }
        }

        if(end <= n) {
            blocks.emplace(end, day, level);
        }

        std::cout << sum << "\n";
    }
};


int main() {
    std::ios_base::sync_with_stdio(false);
    solver_t<>{}.solve();
}
