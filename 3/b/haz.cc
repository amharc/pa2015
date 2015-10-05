#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <utility>
#include <vector>

#ifdef DEBUG
static constexpr const bool debug = true;
#else
static constexpr const bool debug = false;
#endif

template<class Money>
struct input_t {
    int64_t n, m;
    std::vector<Money> initial;
    std::string cycle_string;

    friend std::istream& operator>>(std::istream &str, input_t &input) {
        str >> input.n;
        input.initial.clear();
        input.initial.reserve(input.n);
        std::copy_n(std::istream_iterator<Money>(str), input.n, std::back_inserter(input.initial));
        str >> input.m >> input.cycle_string;
        return str;
    }
};

template<class Money>
class cycles_t {
    using cycle_positions_t = std::set<std::pair<Money, int64_t>>;
    using cycle_id_t = int64_t;
    using cycle_pos_t = int64_t;

    public:
    cycles_t(int64_t step, const std::string &string) 
    : nodes(string.length(), node_t{})
    {
        cycle_id_t cycle_count = 0;
        if(debug)
            std::cerr << "Got: " << string << std::endl;

        for(int64_t idx = 0; idx < static_cast<int64_t>(string.length()); ++idx) {
            if(debug)
                std::cerr << "Node " << idx << std::endl;

            if(nodes[idx].cycle_id != NO_CYCLE)
                continue;

            Money base = 0;
            Money minbase = 0;
            cycle_pos_t counter = 0;
            cycle_t cycle;

            traverse_cycle(idx, step, string.size(),
                [this, &cycle, &base, &minbase, &counter, cycle_count, &string](int64_t jdx) {
                    auto &node = nodes[jdx];
                    node.cycle_id = cycle_count;

                    node.cycle_pos = counter;

                    auto value = char_to_diff(string[jdx]);

                    base += value;
                    cycle.positions.emplace(base, counter);
                    
                    node.min_left = minbase;
                    node.partial_sum = base;
                    node.value = value;

                    minbase = std::min(minbase, base);
                    ++counter;
                }
            );

            cycle.sum = base;
            cycle.length = counter;

            for(int i = 0; i < 5; ++i) {
                traverse_cycle(idx, step, string.size(),
                    [this, &cycle, &base, &counter, &string](int64_t jdx) {
                        base += char_to_diff(string[jdx]);
                        cycle.positions.emplace(base, counter++);
                    }
                );
            }

            minbase = std::numeric_limits<Money>::max();

            traverse_cycle(idx - step, -step, string.size(),
                [this, &minbase](int64_t jdx) {
                    if(debug)
                        std::cerr << "visiting " << jdx << std::endl;
                    auto &node = nodes[jdx];
                    minbase = std::min(minbase, node.partial_sum);
                    node.min_right = minbase;
                }
            );

            cycles.push_back(std::move(cycle));
            ++cycle_count;
        }

        if(debug) {
            for(int64_t idx = 0; idx < static_cast<int64_t>(string.length()); ++idx) {
                const auto &node = nodes[idx];
                std::cerr << "At " << idx << ": " << node.cycle_id << ":" << node.cycle_pos
                    << " " << node.partial_sum << ", " << node.min_left << ", " << node.min_right
                    << std::endl;

                std::cerr << "\t min safe: " << min_safe_for(idx) << std::endl;
            }

            std::cerr << "#########################################" << std::endl;

            for(const auto &p: cycles) {
                std::cerr << "Cycle " << std::endl;

                for(const auto &kv: p.positions)
                    std::cerr << "\t" << kv.first << " at " << kv.second << std::endl;
            }
        }
    }

    template<class Moves>
    Moves get_moves(int64_t idx, Money money) const {
        static_assert(std::is_signed<Moves>::value, "Moves must be signed");
        static_assert(std::numeric_limits<Moves>::max() >= std::numeric_limits<Money>::max(), "Moves must be >= than Money");

        if(money == 0)
            return 0;

        const auto &node = nodes[idx];
        const auto &cycle = cycles[node.cycle_id];
        const auto min_safe = min_safe_for(idx);

        if(cycle.sum >= 0 && (min_safe == LOOP || min_safe <= money))
            return LOOP;

        if(debug)
            std::cerr << money << " -> " << min_safe << ", cycle sum: " << cycle.sum << std::endl;

        const auto full_cycles = std::max<Moves>(0, cycle.sum >= 0 || money < min_safe ? 0 : 1 + (min_safe - money) / cycle.sum);

        auto moves = static_cast<Moves>(full_cycles * cycle.length);

        if(debug)
            std::cerr << full_cycles << " full cycles, i.e. " << moves << std::endl;

        money += full_cycles * cycle.sum;

        if(debug)
            std::cerr << money << " remaining, min_safe = " << min_safe << std::endl;

        assert(0 <= money);
        assert(money < min_safe);

        moves += static_cast<Moves>(how_long_with(idx, money));

        if(debug)
            std::cerr << "total: " << moves << std::endl;
        return moves;
    }

    static constexpr const int64_t NO_CYCLE = -1;
    static constexpr const Money LOOP = -1;

    private:
    struct cycle_t {
        cycle_positions_t positions;
        Money sum;
        int64_t length;
    };

    struct node_t {
        cycle_id_t cycle_id;
        cycle_pos_t cycle_pos;
        Money min_left, min_right;
        Money partial_sum, value;

        node_t()
        : cycle_id{NO_CYCLE}
        , cycle_pos{0}
        , min_left{}
        , min_right{}
        , partial_sum{}
        , value{}
        { }
    };

    std::vector<cycle_t> cycles;
    std::vector<node_t> nodes;

    static constexpr Money char_to_diff(char x) {
        return x == 'W' ? 1 : -1;
    }

    template<class Fun>
    static void traverse_cycle(cycle_pos_t start, int64_t step, int64_t size, Fun &&fun) {
        static_assert(std::is_signed<cycle_pos_t>::value, "cycle_pos_t should be signed");

        start = (start % size + size) % size;
        auto current = start;
        do {
            fun(current);
            current = ((current + step) % size + size) % size;
        } while(current != start);
    }

    Money min_safe_for(int64_t idx) const {
        const auto &node = nodes[idx];
        const auto &cycle = cycles[node.cycle_id];

        const auto cand_right = node.min_right - node.partial_sum + node.value;
        const auto cand_left = node.min_left + cycle.sum - node.partial_sum + node.value;

        if(debug)
            std::cerr << "min_safe_for(" << idx << ") -> " << cand_right << " " << cand_left << std::endl;
        
        const auto cand = std::min(cand_right, cand_left) - 1;

        if(cand >= 0)
            return LOOP;
        else
            return -cand;
    }

    int64_t how_long_with(int64_t idx, Money money) const {
        if(money == 0)
            return 0;
        const auto &node = nodes[idx];
        const auto &cycle = cycles[node.cycle_id];

        const auto value = node.partial_sum - node.value - money;
        const auto it = cycle.positions.lower_bound({value, node.cycle_pos});

        assert(it != cycle.positions.end());

        const auto my_pos = node.cycle_pos;
        const auto that_pos = it->second;
        if(debug)
            std::cerr << "jump " << my_pos << " -> " << that_pos << std::endl;
        const auto diff = that_pos - my_pos;

        assert(it->first == value);
        assert(diff >= 0);
        assert(diff < 3 * cycle.length);
        return diff + 1;
    }

};

template<class Money, class Moves>
class solver_t {
    public:
    solver_t(const solver_t&) = default;
    solver_t(solver_t &&) = default;

    template<class Input>
    solver_t(Input &&input) 
    : input(std::forward<Input>(input))
    , cycles{this->input.n, this->input.cycle_string}
    { } 

    Moves operator()() && {
        static constexpr const auto null = std::numeric_limits<Moves>::max();

        auto res = null;
        for(int64_t idx = 0; idx < input.n; ++idx) {
            auto value = cycles.template get_moves<Moves>(idx % input.cycle_string.size(), input.initial[idx]);
            if(value != cycles_t<Money>::LOOP) {
                res = std::min(res, static_cast<Moves>(input.n) * (value - 1) + idx + 1);
            }
        }

        if(res == null)
            return -1;
        else
            return res;
    }

    private:
    input_t<Money> input;
    cycles_t<Money> cycles;
};

template<class Money, class Moves>
class slow_solver_t {
    public:
    slow_solver_t(const slow_solver_t&) = default;
    slow_solver_t(slow_solver_t &&) = default;

    template<class Input>
    slow_solver_t(Input &&input) 
    : input(std::forward<Input>(input))
    { }

    Moves operator()() && {
        Moves res = 0;

        int64_t i = 0, j = 0;
        while(true) {
            if(input.initial[i] == 0)
                return res;

            if(input.cycle_string[j] == 'W')
                input.initial[i]++;
            else
                input.initial[i]--;

            ++res;

            if(input.initial[i] == 0)
                return res;

            i = (i + 1) % input.n;
            j = (j + 1) % input.cycle_string.size();
        }
    }

    private:
    input_t<Money> input;
};

int main(int argc, char**) {
    using money_t = int64_t;
    using moves_t = int64_t;

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    input_t<money_t> input;
    std::cin >> input;

    if(argc > 1)
        std::cout << slow_solver_t<money_t, moves_t>{std::move(input)}() << std::endl;
    else
        std::cout << solver_t<money_t, moves_t>{std::move(input)}() << std::endl;
}
