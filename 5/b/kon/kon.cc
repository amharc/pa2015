#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <stack>
#include <vector>

//#define DEBUG

#ifdef DEBUG
static constexpr const bool debug = true;
#else
static constexpr const bool debug = false;
#endif

class graph_t {
    using dfstime_t = int32_t;
    static constexpr const dfstime_t NEVER = 0;

    public:
    using vertex_id_t = int32_t;
    static constexpr const vertex_id_t NO_VERTEX = -1;

    graph_t()
    : state{state_t::NO_SCC}
    , timer{1}
    { }

    friend std::istream& operator>>(std::istream &str, graph_t &graph) {
        size_t n, m;
        str >> n >> m;

        graph.vertices.clear();
        graph.vertices.resize(n);

        for(size_t i = 0; i < m; ++i) {
            vertex_id_t a, b;
            str >> a >> b;
            --a, --b;
            graph.vertices[a].adj.push_back(b);
        }

        return str;
    }

    void solve() {
        for(vertex_id_t vertex_id = 0; vertex_id < static_cast<vertex_id_t>(vertices.size()); ++vertex_id) {
            if(debug)
                std::cerr << "vertex " << vertex_id << std::endl;
            const auto &vertex = vertices[vertex_id];
            if(vertex.order == NEVER)
                dfs(vertex_id);
        }
    }

    void print_result(std::ostream &str) const {
        switch(state) {
            case state_t::NO_SCC:
                str << "NIE" << std::endl;
                return;

            case state_t::ONE_SCC:
                str << result.size() << std::endl;
                for(auto vertex_id: result)
                    str << vertex_id + 1 << " ";
                str << std::endl;
                return;

            case state_t::MULTIPLE_SCC:
                str << 0 << std::endl << std::endl;
                return;
        }
    }

    private:
    struct vertex_t {
        std::vector<vertex_id_t> adj;
        dfstime_t low, order;
        dfstime_t high, postorder;
        vertex_id_t scc_root_id;
        bool on_stack;

        vertex_t()
        : low{NEVER}
        , order{NEVER}
        , high{NEVER}
        , postorder{NEVER}
        , on_stack{false}
        { }
    };

    enum class state_t {
        NO_SCC,
        ONE_SCC,
        MULTIPLE_SCC
    };

    std::vector<vertex_t> vertices;
    std::stack<vertex_id_t> stack;

    state_t state;
    std::vector<vertex_id_t> result;

    dfstime_t timer;

    void dfs(vertex_id_t vertex_id) {
        if(debug) {
            std::cerr << "Entering vertex " << vertex_id << std::endl;
        }

        auto &vertex = vertices[vertex_id];
        vertex.order = vertex.low = timer++;
        vertex.on_stack = true;
        stack.push(vertex_id);

        for(auto adj_id: vertex.adj) {
            const auto &adj = vertices[adj_id];

            if(adj.order == NEVER) {
                dfs(adj_id);
                vertex.low = std::min(vertex.low, adj.low);
            }
            else if(adj.on_stack) {
                vertex.low = std::min(vertex.low, adj.order);
            }
        }

        vertex.postorder = vertex.high = timer++;

        if(vertex.low == vertex.order) {
            std::vector<vertex_id_t> scc;

            vertex_id_t scc_vertex_id;
            do {
                scc_vertex_id = stack.top();
                stack.pop();
                vertices[scc_vertex_id].on_stack = false;
                vertices[scc_vertex_id].scc_root_id = vertex_id;
                scc.push_back(scc_vertex_id);
            } while(vertex_id != scc_vertex_id);

            process_scc(std::move(scc));
        }
    }

    void process_scc(std::vector<vertex_id_t> &&scc) {
        if(debug) {
            std::cerr << "a strongly connected component: ";
            std::copy(std::begin(scc), std::end(scc), std::ostream_iterator<vertex_id_t>(std::cerr, " "));
            std::cerr << std::endl;
        }

        if(scc.size() == 1)
            return;

        switch(state) {
            case state_t::ONE_SCC:
                state = state_t::MULTIPLE_SCC;
                /* fall-through */
            case state_t::MULTIPLE_SCC:
                return;
            case state_t::NO_SCC:
                state = state_t::ONE_SCC;

                std::sort(std::begin(scc), std::end(scc),
                    [this](vertex_id_t lhs, vertex_id_t rhs) { return vertices[lhs].postorder < vertices[rhs].postorder; }
                );

                const auto scc_root_id = vertices[scc[0]].scc_root_id;

                auto lowest = std::numeric_limits<vertex_id_t>::max();
                for(const auto vertex_id: scc) {
                    auto &vertex = vertices[vertex_id];

                    if(debug)
                        std::cerr << "vertex " << vertex_id << ", postorder = " << vertex.postorder << std::endl;

                    for(const auto neighbour_id: vertex.adj) {
                        const auto &neighbour = vertices[neighbour_id];

                        if(neighbour.scc_root_id != scc_root_id)
                            continue;

                        if(debug)
                            std::cerr << "\tedge " << vertex_id << " -> " << neighbour_id << std::endl;

                        vertex.high = std::max(vertex.high, neighbour.high);

                        if(vertex.postorder < neighbour.postorder) {
                            if(debug)
                                std::cerr << "\t\tis a back edge, clearing result" << std::endl;

                            lowest = std::min(lowest, neighbour.postorder);
                            result.clear();
                        }
                        else if(vertex.postorder <= neighbour.high) {
                            while(!result.empty() && vertices[result.back()].postorder > neighbour.postorder) {
                                if(debug)
                                    std::cerr << "\t\tpopping " << result.back() << std::endl;

                                result.pop_back();
                            }
                        }
                    }

                    if(debug)
                        std::cerr << "exiting " << vertex_id << ", high = " << vertex.high << ", lowest = " << lowest << std::endl;

                    if(vertex.postorder <= lowest) {
                        result.push_back(vertex_id);
                        
                        if(debug)
                            std::cerr << "\tpushing " << vertex_id << std::endl;
                    }
                }
                std::sort(std::begin(result), std::end(result));
        }
    }
};

int main() {
    std::ios_base::sync_with_stdio(false);

    graph_t graph;
    std::cin >> graph;

    graph.solve();
    graph.print_result(std::cout);
}
