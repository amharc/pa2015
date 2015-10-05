#include "sabotaz.h"
#include "message.h"

#include <cassert>
#include <iostream>
#include <functional>
#include <vector>
#include <sstream>

//#define DEBUG

#ifdef DEBUG
static constexpr const bool debug = true;
#else
static constexpr const bool debug = false;
#endif

#define LOG(x) \
    if(debug) { \
        std::stringstream str; \
        str << "Node " << MyNodeId() << "/" << NumberOfNodes() << ": "; \
        str x; \
        std::cerr << str.str(); \
    }

// ------------------------ messaging -----------------------

using node_t = int;

namespace communication {
template<unsigned size>
struct communication_traits { };

#define GEN_TRAITS(type, name) \
    template<> \
    struct communication_traits<sizeof(type)> { \
        template<class T> \
        static void put(node_t target, T value) { \
            static_assert(sizeof(T) == sizeof(type), "Size mismatch between T and " #type);\
            Put ## name(target, static_cast<type>(value)); \
        } \
\
        template<class T> \
        static T get(node_t target) { \
            static_assert(sizeof(T) == sizeof(type), "Size mismatch between T and " #type);\
            return static_cast<T>(Get ## name(target)); \
        } \
    };

GEN_TRAITS(char, Char)
GEN_TRAITS(int, Int)
GEN_TRAITS(long long, LL)

template<class T>
void put(node_t target, T value) {
    communication_traits<sizeof(value)>::put(target, value);
}

template<class T>
void get(node_t target, T *number) {
    *number = communication_traits<sizeof(*number)>::template get<T>(target);
}

void send(node_t target) {
    Send(target);
}

void receive(node_t from) {
    Receive(from);
}
}

// ------------------------------------------------------------------------------------------------

class graph_t {
    public:
    using vertexid_t = int32_t;
    using dfstime_t = int32_t;

    static constexpr const dfstime_t UNVISITED = -1;
    static constexpr const vertexid_t NO_PARENT = -1;

    graph_t(size_t vertex_count)
    : vertices(vertex_count, vertex_t{})
    { }

    void add_edge(vertexid_t a, vertexid_t b) {
        vertices[a].adj.push_back(b);
        vertices[b].adj.push_back(a);
    }

    void clear() {
        for(auto &vertex: vertices)
            vertex.adj.clear();
    }

    void dfs() {
        for(auto &vertex: vertices)
            vertex.order = UNVISITED;

        dfstime_t time = 1;
        for(vertexid_t vertexid = 0; vertexid < static_cast<vertexid_t>(vertices.size()); ++vertexid) {
            auto &vertex = vertices[vertexid];
            if(vertex.order == UNVISITED)
                dfs(vertexid, NO_PARENT, time);
        }
    }

    template<class Fn>
    void iter_important(Fn &&fn) const {
        for(vertexid_t vertexid = 0; vertexid < static_cast<vertexid_t>(vertices.size()); ++vertexid) {
            auto &vertex = vertices[vertexid];
            if(vertex.parentid != NO_PARENT) {
                fn(vertexid, vertex.parentid);
            }
            
            size_t parent_count = 0;
            for(auto adjid: vertex.adj) {
                auto &adj = vertices[adjid];
                if(adj.order == vertex.low && adjid != vertexid && (adjid != vertex.parentid || parent_count++ == 1)) {
                    fn(adjid, vertexid);
                    break;
                }
            }
        }
    }

    template<class Fn>
    void iter_bridges(Fn &&fn) const {
        for(vertexid_t vertexid = 0; vertexid < static_cast<vertexid_t>(vertices.size()); ++vertexid) {
            auto &vertex = vertices[vertexid];
            if(vertex.parentid != NO_PARENT && vertex.low == vertex.order)
                fn(vertexid, vertex.parentid);
        }
    }

    private:
    struct vertex_t {
        std::vector<vertexid_t> adj;
        vertexid_t parentid;
        dfstime_t order;
        dfstime_t low;
    };

    std::vector<vertex_t> vertices;

    void dfs(vertexid_t vertexid, vertexid_t parentid, dfstime_t &time) {
        auto &vertex = vertices[vertexid];
        vertex.order = vertex.low = time++;
        vertex.parentid = parentid;
        size_t parent_count = 0;
        
        for(auto adjid: vertex.adj) {
            auto &adj = vertices[adjid];
            if(adj.order == UNVISITED) {
                dfs(adjid, vertexid, time);
                vertex.low = std::min(vertex.low, adj.low);
            }
            else if(adjid != parentid || parent_count++) {
                vertex.low = std::min(vertex.low, adj.order);
            }
        }
    }
};

class solver_t {
    public:
    solver_t()
    : vertex_count(NumberOfIsles())
    , graph(vertex_count)
    { }

    void map(size_t begin, size_t end) {
        LOG(<< "map(" << begin << ", " << end << ")");
        graph.clear();
        for(size_t idx = begin; idx < end; ++idx)
            graph.add_edge(
                static_cast<graph_t::vertexid_t>(BridgeEndA(idx)),
                static_cast<graph_t::vertexid_t>(BridgeEndB(idx))
            );
        graph.dfs();
    }

    void reduce(const std::vector<node_t> &from) {
        prune();
        for(auto node: from) {
            using namespace std::placeholders;
            make_edge_reader(node, std::bind(&graph_t::add_edge, &graph, _1, _2))();
        }
        graph.dfs();
    }

    void send(node_t target) {
        graph.iter_important(edge_writer_t{target});
    }

    void prune() {
        using namespace std::placeholders;

        graph_t new_graph(vertex_count);
        graph.iter_important(std::bind(&graph_t::add_edge, &new_graph, _1, _2));
        graph = std::move(new_graph);
    }

    size_t number_of_bridges() const {
        size_t res = 0;
        graph.iter_bridges([&res](graph_t::vertexid_t, graph_t::vertexid_t) { ++res; });
        return res;
    }

    private:
    size_t vertex_count, edge_count;
    graph_t graph;

    static constexpr const graph_t::vertexid_t NEXT_MESSAGE = -1;
    static constexpr const graph_t::vertexid_t END_OF_STREAM = -2;

    class edge_writer_t {
        public:
        edge_writer_t(node_t target)
        : target(target)
        , records(0)
        { }

        edge_writer_t(const edge_writer_t&) = delete;

        ~edge_writer_t() {
            communication::put(target, END_OF_STREAM);
            communication::send(target);
        }

        void operator()(graph_t::vertexid_t a, graph_t::vertexid_t b) {
            LOG(<< "to node " << target << ": " << a << " " << b << std::endl);
            communication::put(target, a);
            communication::put(target, b);
            records++;
            if(records >= RECORDS_IN_MESSAGE) {
                communication::put(target, NEXT_MESSAGE);
                communication::send(target);
                records = 0;
            }
        }

        private:
        node_t target;
        size_t records;
        static constexpr const size_t RECORDS_IN_MESSAGE = 7900 / (2 * sizeof(graph_t::vertexid_t));
    };

    template<class Callback>
    class edge_reader_t {
        public:
        template<class CallbackRef>
        edge_reader_t(node_t from, CallbackRef &&callback)
        : from(from)
        , callback(std::forward<CallbackRef>(callback))
        { }

        edge_reader_t(const edge_reader_t&) = delete;

        void operator()() {
            communication::receive(from);
            while(true) {
                graph_t::vertexid_t a, b;
                communication::get(from, &a);

                if(a == NEXT_MESSAGE) {
                    communication::receive(from);
                    continue;
                }
                else if(a == END_OF_STREAM) {
                    return;
                }
                else {
                    communication::get(from, &b);
                    LOG(<< "from node " << from << ": " << a << " " << b << std::endl);
                    callback(a, b);
                }
            }
        }

        private:
        node_t from;
        Callback callback;
    };

    template<class Callback>
    edge_reader_t<typename std::decay<Callback>::type> make_edge_reader(node_t from, Callback &&callback) {
        return {from, callback};
    }
};

int main() {
    constexpr const node_t DIV = 8;

    const auto my_id = MyNodeId();
    const auto edge_count = NumberOfBridges();
    const auto node_count = std::min(static_cast<node_t>(edge_count), static_cast<node_t>(NumberOfNodes()));

    if(my_id >= node_count)
        return 0;

    const auto edges_per_worker = (edge_count + node_count - 1) / node_count;

    solver_t solver;
    solver.map(my_id * edges_per_worker, std::min<size_t>(edge_count, (my_id + 1) * edges_per_worker));

    for(auto div = DIV; div <= DIV * node_count; div *= DIV) {
        LOG(<< "round " << div << " finished" << std::endl);

        if(my_id % div != 0) {
            solver.send(my_id - my_id % div);
            return 0;
        }

        std::vector<node_t> from;
        from.reserve(DIV);
        for(int idx = 1; idx < DIV; idx++) {
            node_t node = my_id + idx * div / DIV;
            if(node >= node_count)
                break;
            from.push_back(node);
        }
        solver.reduce(from);
    }

    std::cout << solver.number_of_bridges() << std::endl;
}
