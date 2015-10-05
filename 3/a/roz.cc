#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <limits>
#include <vector>

#ifdef DEBUG
static constexpr const bool debug = true;
#else
static constexpr const bool debug = false;
#endif

using vertexid_t = ssize_t;
using gauge_t = int32_t;
using cost_t = int64_t;

struct graph_t {
    ssize_t n, m;
    struct vertex_t {
        std::vector<vertexid_t> adj;
        gauge_t gauge;

        vertex_t() : gauge{-1} { }
    };
    std::vector<vertex_t> vertices;

    friend std::istream& operator>>(std::istream &str, graph_t &input) {
        str >> input.n >> input.m;
        input.vertices.clear();
        input.vertices.resize(input.n);

        for(ssize_t i = 1; i < input.n; ++i) {
            vertexid_t a, b;
            str >> a >> b;
            --a, --b;
            input.vertices[a].adj.push_back(b);
            input.vertices[b].adj.push_back(a);
        }

        for(ssize_t i = 0; i < input.m; ++i) {
            str >> input.vertices[i].gauge;
        }

        return str;
    }
};

class solver_t {
    public:
    solver_t(const solver_t&) = default;
    solver_t(solver_t&&) = default;

    template<class Graph>
    solver_t(Graph &&graph) : graph(std::forward<Graph>(graph)) { }

    cost_t operator()() const && {
        if(graph.n == graph.m) {
            assert(graph.n == 2);
            return std::abs(graph.vertices[0].gauge - graph.vertices[1].gauge);
        }

        return dfs(graph.n - 1, -1).cost;
    }

    private:
    using gauge_pair_t = std::pair<gauge_t, gauge_t>;

    struct result_t {
        cost_t cost;
        gauge_pair_t gauges;
    };

    result_t dfs(vertexid_t vertexid, vertexid_t parentid) const {
        if(vertexid < graph.m) {
            auto gauge = graph.vertices[vertexid].gauge;
            return {0, {gauge, gauge}};
        }

        cost_t cost = 0;
        std::vector<gauge_pair_t> results;

        for(const auto neighbourid: graph.vertices[vertexid].adj) {
            if(neighbourid != parentid) {
                const auto r = dfs(neighbourid, vertexid);
                cost += r.cost;
                results.push_back(std::move(r.gauges));
            }
        }

        auto result = compute(std::move(results));
        result.cost += cost;
        
        if(debug) {
            std::cerr << vertexid << ": " << result.cost << " -> " << result.gauges.first << ":" << result.gauges.second << std::endl;
        }

        return result;
    }

    static gauge_pair_t merge(const std::vector<gauge_pair_t> &gauge_pairs) {
        if(debug) {
            std::cerr << "Merging: ";
            for(const auto &gauge_pair: gauge_pairs)
                std::cerr << gauge_pair.first << ":" << gauge_pair.second << " ";
            std::cerr << std::endl;
        }

        std::vector<gauge_t> gauges;
        gauges.reserve(2 * gauge_pairs.size());

        for(const auto &gauge_pair: gauge_pairs) {
            gauges.push_back(gauge_pair.first);
            gauges.push_back(gauge_pair.second);
        } 

        std::sort(gauges.begin(), gauges.end());

        auto idx = gauges.size() / 2;

        return {gauges[idx - 1], gauges[idx]};
    }

    static cost_t costfor(const gauge_t gauge, const std::vector<gauge_pair_t> &gauge_pairs) {
        cost_t cost = 0;
        for(const auto &gauge_pair: gauge_pairs) {
            if(gauge < gauge_pair.first)
                cost += gauge_pair.first - gauge;
            else if(gauge > gauge_pair.second)
                cost += gauge - gauge_pair.second;
        }
        return cost;
    }

    static result_t compute(const std::vector<gauge_pair_t> &gauge_pairs) {
        const auto gauge_pair = merge(gauge_pairs);
        const auto cost = costfor(gauge_pair.first, gauge_pairs);
        return {cost, gauge_pair};
    }

    graph_t graph;
};

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    graph_t graph;
    std::cin >> graph;
    std::cout << solver_t{std::move(graph)}() << std::endl;
}
