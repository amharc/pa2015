#include <algorithm>
#include <iostream>
#include <iterator>
#include <queue>
#include <vector>

//#define DEBUG

#ifdef DEBUG
constexpr const bool debug = true;
#else
constexpr const bool debug = false;
#endif

class graph_t {
    private:
        std::vector<std::vector<size_t>> adj;
    public:
        using vertex_t = size_t;

        void resize(size_t sz) {
            adj.clear();
            adj.resize(sz);
        }

        void add_edge(vertex_t a, vertex_t b) {
            adj[a].push_back(b);
            adj[b].push_back(a);
        }

        const std::vector<size_t>& operator[](size_t idx) const {
            return adj[idx];
        }

        auto begin() const -> decltype(adj.cbegin()) {
            return adj.cbegin();
        }

        auto end() const -> decltype(adj.cend()) {
            return adj.cend();
        }
};

struct input_t {
    graph_t graph;
    size_t n, m, d;

    friend std::istream& operator>>(std::istream &str, input_t &input) {
        str >> input.n >> input.m >> input.d;
        input.graph.resize(input.n);
        for(size_t i = 0; i < input.m; ++i) {
            graph_t::vertex_t u, v;
            str >> u >> v;
            input.graph.add_edge(u - 1, v - 1);
        }
        return str;
    }
};

class output_t {
    public:
    using vertex_t = graph_t::vertex_t;

    template<class... Args>
    output_t(Args&&... args) : data(std::forward<Args>(args)...) {
        std::sort(data.begin(), data.end());
    }

    friend std::ostream& operator<<(std::ostream &str, const output_t &output) {
        if(output.data.empty())
            str << "NIE" << std::endl;
        else {
            str << output.data.size() << std::endl;
            for(const auto v: output.data)
                str << v + 1 << " ";
            str << std::endl;
        }

        return str;
    }

    private:
    std::vector<vertex_t> data;
};

class solver_t {
    public:
        using vertex_t = graph_t::vertex_t;
        using component_t = size_t;

        solver_t() = default;
        solver_t(const solver_t &) = delete;
        solver_t(solver_t &&) = default;

        template<class Input>
        explicit solver_t(Input &&input) 
        : input(std::forward<Input>(input))
        , data(input.n)
        { }

        output_t operator()() && {
            fill_degrees();
            prepare_queue();
            eliminate();
            auto c = get_component();
            return retrieve_component(c);
        }

    private:
        struct vertex_data {
            static constexpr const component_t NOT_VISITED = -1;
            bool removed;
            size_t degree;
            component_t component;

            vertex_data()
            : removed{false}
            , degree{0}
            , component{NOT_VISITED}
            { }
        };

        void fill_degrees() {
            for(size_t i = 0; i < input.n; ++i)
                for(vertex_t j: input.graph[i])
                    data[j].degree++;
        }

        void prepare_queue() {
            for(size_t i = 0; i < input.n; ++i)
                if(data[i].degree < input.d) {
                    queue.emplace(i);
                    data[i].removed = true;
                }
        }

        void eliminate() {
            while(!queue.empty()) {
                const auto u = queue.front();
                queue.pop();

                if(debug)
                    std::cerr << "removing " << u << std::endl;

                for(const auto v: input.graph[u]) {
                    if(!data[v].removed && --data[v].degree < input.d) {
                        data[v].removed = true;
                        queue.emplace(v);
                    }
                }
            }
        }

        size_t dfs(const vertex_t v, const component_t c) {
            if(data[v].component != vertex_data::NOT_VISITED)
                return 0;

            if(debug)
                std::cerr << "dfs(" << v << ", " << c << ")" << std::endl;

            data[v].component = c;
            size_t res = 1;
            for(const auto u: input.graph[v])
                if(!data[u].removed)
                    res += dfs(u, c);

            return res;
        }

        component_t get_component() {
            size_t best_size = 0;
            component_t best = vertex_data::NOT_VISITED;

            for(size_t i = 0; i < input.n; ++i)
                if(!data[i].removed && data[i].component == vertex_data::NOT_VISITED) {
                    size_t current = dfs(i, i);
                    
                    if(debug)
                        std::cerr << "Component with " << i << ": " << current << std::endl;

                    if(current > best_size) {
                        best_size = current;
                        best = i;
                    }
                }

            return best;
        }

        std::vector<vertex_t> retrieve_component(const component_t c) const {
            if(c == vertex_data::NOT_VISITED)
                return {};

            std::vector<vertex_t> res;
            for(size_t i = 0; i < input.n; ++i)
                if(data[i].component == c)
                    res.push_back(i);
            return res;
        }

        const input_t input;
        std::vector<vertex_data> data;
        std::queue<vertex_t> queue;
};

int main() {
    std::ios_base::sync_with_stdio(false);
    input_t input;
    std::cin >> input;
    std::cout << solver_t{std::move(input)}();
}
