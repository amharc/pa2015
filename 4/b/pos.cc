#include <cassert>
#include <array>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>

#include "poszukiwania.h"
#include "message.h"

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

template<class T, class E, class Fn, class Init>
T iterate(T base, Init init, E exponent, Fn &&fn) {
    T result = init;

    while(exponent > 0) {
        if(exponent % 2 == 1)
            result = fn(result, base);
        base = fn(base, base);
        exponent /= 2;
    }

    return result;
}


// ------------------------ hashing -------------------------

using hash_t = int32_t;
using bigger_hash_t = int64_t;

struct hash_setup_t {
    hash_t prime, base;
};

static constexpr const hash_setup_t setup[] = {
//    { (1ll << 62) - 153, (1ll << 50) - 131 }
//    { (1ll << 62) - 471, (1ll << 42) - 11 }
//    { (1ll << 62) - 203, (1ll << 56) - 5 }
//    { (1 << 30) - 107, (1 << 28) - 213 },
    { (1 << 30) - 153, (1 << 29) - 43 },
    { (1 << 30) - 161, (1 << 29) - 3 }
};

static constexpr const size_t prime_count = sizeof(setup) / sizeof(*setup);

using hash_array_t = std::array<hash_t, prime_count>;

__attribute__((pure))
hash_array_t make_powers(uint64_t exponent) {
    hash_array_t arr;
    for(size_t idx = 0; idx < prime_count; ++idx) {
        const auto op = [&idx](const hash_t x, const hash_t y) {
            return static_cast<hash_t>((static_cast<bigger_hash_t>(x) * y) % setup[idx].prime);
        };
        const auto pw = iterate(setup[idx].base, 1, exponent, op);
        arr[idx] = pw;
    }
    return arr;
}

struct hash_block_t {
    hash_array_t hash;
    ssize_t length;

    hash_block_t() : length(0) {
        std::fill(std::begin(hash), std::end(hash), 0);
    }

    explicit hash_block_t(bigger_hash_t value) : length(1) {
        for(size_t idx = 0; idx < prime_count; ++idx)
            hash[idx] = value % setup[idx].prime;
    }

    __attribute__((pure))
    hash_block_t operator+(const hash_block_t &rhs) const {
        hash_block_t result;
        result.length = length + rhs.length;

        for(size_t idx = 0; idx < prime_count; ++idx) {
            // g++ does a better job with inlining this stuff when it is defined locally
            const auto op = [&idx](const hash_t x, const hash_t y) {
                return static_cast<hash_t>((static_cast<bigger_hash_t>(x) * y) % setup[idx].prime);
            };
            const auto pw = iterate(setup[idx].base, 1, length, op);
            result.hash[idx] = hash[idx] + op(pw, rhs.hash[idx]);
            if(result.hash[idx] >= setup[idx].prime)
                result.hash[idx] -= setup[idx].prime;
        }

        return result;
    }

    __attribute__((pure))
    hash_block_t extend(const hash_block_t &rhs, const hash_array_t &powers) const {
        hash_block_t result;
        result.length = length + rhs.length;

        for(size_t idx = 0; idx < prime_count; ++idx) {
            const auto op = [&idx](const hash_t x, const hash_t y) {
                return static_cast<hash_t>((static_cast<bigger_hash_t>(x) * y) % setup[idx].prime);
            };
            result.hash[idx] = hash[idx] + op(powers[idx], rhs.hash[idx]);
            if(result.hash[idx] >= setup[idx].prime)
                result.hash[idx] -= setup[idx].prime;
        }

        return result;
    }

    hash_block_t operator+=(const hash_block_t &that) {
        return operator=(*this + that);
    }

    __attribute__((pure))
    hash_block_t trim(const hash_block_t &rhs) const {
        return trim(rhs, make_powers(length - rhs.length));
    }

    __attribute__((pure))
    hash_block_t trim(const hash_block_t &rhs, const hash_array_t &powers) const {
        hash_block_t result;
        result.length = length - rhs.length;

        for(size_t idx = 0; idx < prime_count; ++idx) {
            const auto op = [&idx](const hash_t x, const hash_t y) {
                return static_cast<hash_t>((static_cast<bigger_hash_t>(x) * y) % setup[idx].prime);
            };
            result.hash[idx] = hash[idx] - op(powers[idx], rhs.hash[idx]);
            if(result.hash[idx] < 0)
                result.hash[idx] += setup[idx].prime;
        }

        return result;
    }

    bool operator==(const hash_block_t &that) const {
        assert(length == that.length);
        return hash == that.hash;
    }
};

template<class Fn>
hash_block_t hash_sequence(ssize_t begin, ssize_t end, Fn &&fn) {
    hash_block_t result{};

    assert(begin <= end);

    if(begin != end)
        for(ssize_t idx = end - 1; idx >= begin; --idx) {
            result = hash_block_t{fn(idx)} + result;
        }

    return result;
}

struct request_t {
    hash_block_t hash;
    ssize_t hash_begin, hash_end;
    ssize_t begin, end;
};

// ------------------------ messaging -----------------------

enum class message_type_t {
    HASH_SIGNAL_PART,
    HASH_SEQUENCE_PART,
    HASH_SIGNAL_FULL,
    SUBSEQUENCE,
    RESULT,
    NOP
};

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

template<class T, size_t n>
void put(node_t target, const std::array<T, n> &arr) {
    for(size_t idx = 0; idx < n; ++idx)
        put(target, arr[idx]);
}

template<class T, size_t n>
void get(node_t target, std::array<T, n> *arr) {
    for(size_t idx = 0; idx < n; ++idx)
        get<T>(target, &(*arr)[idx]);
}

void put(node_t node, const hash_block_t &block) {
    put(node, block.length);
    put(node, block.hash);
}

void get(node_t node, hash_block_t *block) {
    get(node, &block->length);
    get(node, &block->hash);
}

void put(node_t node, const request_t &rq) {
    put(node, rq.hash);
    put(node, rq.hash_begin);
    put(node, rq.hash_end);
    put(node, rq.begin);
    put(node, rq.end);
}

void get(node_t node, request_t *rq) {
    get(node, &rq->hash);
    get(node, &rq->hash_begin);
    get(node, &rq->hash_end);
    get(node, &rq->begin);
    get(node, &rq->end);
}

void _send(node_t) { }

void _recv(node_t) { }

template<class Arg, class... Args>
void _send(node_t node, Arg&& value, Args&&... args) {
    put(node, std::forward<Arg>(value));
    _send(node, std::forward<Args>(args)...);
}

template<class Arg, class... Args>
void _recv(node_t node, Arg *value, Args*... args) {
    get(node, value);
    _recv(node, args...);
}

template<class... Args>
void send(node_t node, message_type_t type, Args&&... args) {
    _send(node, type, std::forward<Args>(args)...);
    Send(node);
}

template<class... Args>
void recv(node_t node, message_type_t type, Args*... args) {
    Receive(node);
    message_type_t got;
    _recv(node, &got);
    assert(got == type);
    _recv(node, args...);
}
}

using communication::send;
using communication::recv;

// --------------------- solution ------------------------------

constexpr const node_t MASTER = 0;

template<class LengthFun>
size_t block_length(size_t num_workers, LengthFun &&length) {
    const auto len = static_cast<size_t>(std::forward<LengthFun>(length)());
    const auto len_per_worker = (len + num_workers - 1) / num_workers;
    return len_per_worker;
}

class worker_t {
    public:
    worker_t(size_t worker_id, size_t num_workers)
    : worker_id{worker_id}
    , num_workers{num_workers}
    { }

    void do_hash_signal() {
        worker_hash_subsequence(message_type_t::HASH_SIGNAL_PART, SignalLength, SignalAt);
    }

    void do_hash_sequence() {
        worker_hash_subsequence(message_type_t::HASH_SEQUENCE_PART, SeqLength, SeqAt);
    }

    void do_compute() {
        request_t request;
        recv(MASTER, message_type_t::HASH_SIGNAL_FULL, &hash_signal);
        LOG(<< "Got hash of the signal" << std::endl);
        recv(MASTER, message_type_t::SUBSEQUENCE, &request);
        LOG(<< "Got subsequence" << std::endl);
        size_t result = 0;

        send(MASTER, message_type_t::NOP, result);

        if(request.begin != 0) {
            const auto signal_length = static_cast<ssize_t>(SignalLength());

            LOG(<< " will calculate [" << request.begin << ", " << request.end << "),"
                   " first: [" << request.end - 1 << ", " << request.end - 1 + signal_length << ") knowing ["
                    << request.hash_begin << ", " << request.hash_end << ")" << std::endl);

            hash_block_t hash;

            if(request.hash_begin == request.hash_end)
                hash = hash_sequence(request.end - 1, request.end + signal_length - 1, SeqAt);
            else
                hash = hash_sequence(request.end - 1, request.hash_begin, SeqAt) + request.hash +
                    hash_sequence(request.hash_end, request.end + signal_length - 1, SeqAt);

            assert(hash.length == signal_length);
            assert(hash_signal.length == signal_length);

            const auto trim_powers = make_powers(signal_length - 1);

            for(ssize_t pos = request.end - 1; pos >= request.begin;) {
                if(hash == hash_signal) {
                    result++;
                }

                if(--pos < request.begin)
                    break;

                hash = hash_block_t{SeqAt(pos)} + hash.trim(hash_block_t{SeqAt(pos + signal_length)}, trim_powers);
            }

            LOG(<< "matching among [" << request.begin << ", " << request.end << "): " << result << std::endl);
        }

        send(MASTER, message_type_t::RESULT, result);
    }

    private:
    template<class Length, class At>
    void worker_hash_subsequence(const message_type_t type, Length &&length, At &&at) {
        const auto len = static_cast<ssize_t>(length());
        const auto len_per_worker = block_length(num_workers, std::forward<Length>(length));
        const auto begin = std::min<ssize_t>(1 + len_per_worker * worker_id, 1 + len);
        const auto end = std::min<ssize_t>(1 + len_per_worker * (worker_id + 1), 1 + len);
        LOG(<< "Computing [" << begin << "," << end << "), per worker: " << len_per_worker << std::endl);
        const auto hash = hash_sequence(begin, end, std::forward<At>(at));
        send(MASTER, type, hash);
    }

    size_t worker_id;
    size_t num_workers;
    hash_block_t hash_signal;
};

class master_t {
    public:
    master_t(node_t first_worker, node_t last_worker)
    : num_workers(last_worker - first_worker + 1)
    , first_worker{first_worker}
    , last_worker{last_worker}
    { }

    void do_hash_signal() {
        hash_block_t hash;
        for(node_t worker = first_worker; worker <= last_worker; ++worker) {
            hash_block_t current;
            recv(worker, message_type_t::HASH_SIGNAL_PART, &current);
            hash += current;
        }

        hash_signal = hash;
        assert(hash_signal.length == static_cast<ssize_t>(SignalLength()));

        for(node_t worker = first_worker; worker <= last_worker; ++worker) {
            send(worker, message_type_t::HASH_SIGNAL_FULL, hash);
        }
    }

    void do_hash_sequence() {
        hash_sequence.resize(num_workers);
        for(node_t worker = first_worker; worker <= last_worker; ++worker) {
            recv(worker, message_type_t::HASH_SEQUENCE_PART, &hash_sequence[worker - first_worker]);
        }
    }

    size_t do_compute() {
       const auto seq_length = static_cast<ssize_t>(SeqLength());
       const auto signal_length = static_cast<ssize_t>(SignalLength());

       const auto possible = seq_length - signal_length + 1;
       const auto possible_per_worker = (possible + num_workers) / num_workers;

       LOG(<< "possible: " << possible << ", per worker: " << possible_per_worker << std::endl);

       hash_block_t hash;
       auto hash_begin = 1 + seq_length, hash_end = 1 + seq_length;
       auto hash_block_begin = hash_sequence.size(), hash_block_end = hash_sequence.size();
       auto last = possible + 1;
       node_t worker_cnt = 0;

       while(last > 1) {
           const auto idx = std::max<decltype(last)>(1, last - possible_per_worker);

           while(hash_block_begin > 0 && last - 1 + hash_sequence[hash_block_begin - 1].length <= hash_begin) {
               hash_block_begin--;

               hash = hash_sequence[hash_block_begin] + hash;
               hash_begin -= hash_sequence[hash_block_begin].length;
           }

           while(hash_block_end > hash_block_begin && last - 1 + signal_length < hash_end) {
               hash_block_end--;
               hash = hash.trim(hash_sequence[hash_block_end]);
               hash_end -= hash_sequence[hash_block_end].length;
           }

           LOG(<< "last: [" << last << ", " << last + signal_length << "), hashed: [" << hash_begin << ", " << hash_end << ") "
               << "at " << hash_block_begin << ", " << hash_block_end << std::endl);
           assert(hash.length == hash_end - hash_begin);
           assert(worker_cnt < num_workers);

           send(static_cast<node_t>(first_worker + worker_cnt), message_type_t::SUBSEQUENCE,
                request_t{hash, hash_begin, hash_end, idx, last});
           last = idx;
           LOG(<< "Sent to " << first_worker + worker_cnt << std::endl);
           worker_cnt++;
       }

       for(node_t worker = first_worker + worker_cnt; worker <= last_worker; ++worker)
           send(worker, message_type_t::SUBSEQUENCE, request_t{hash, hash_begin, hash_end, 0, 0});
    }

    size_t do_collect() {
       size_t sum = 0;
       for(node_t worker = first_worker; worker <= last_worker; ++worker) {
           size_t cur;
           recv(worker, message_type_t::NOP, &cur);
           recv(worker, message_type_t::RESULT, &cur);
           sum += cur;
       }

       return sum;
    }

    private:
    ssize_t num_workers;
    node_t first_worker, last_worker;
    hash_block_t hash_signal;
    std::vector<hash_block_t> hash_sequence;
};

template<class T>
void perform(T &&t) {
    t.do_hash_signal();
    LOG(<< "Done do_hash_signal" << std::endl);
    t.do_hash_sequence();
    LOG(<< "Done do_hash_sequence" << std::endl);
    t.do_compute();
    LOG(<< "Done do_compute" << std::endl);
}

int main() {
    const auto my_id = MyNodeId();
    const auto number_of_nodes = NumberOfNodes();

    if(my_id == MASTER) {
        master_t master(1, number_of_nodes - 1);
        perform(master);
        std::cout << master.do_collect() << std::endl;
    }
    else
        perform(worker_t(my_id - 1, number_of_nodes - 1));

    LOG(<< "Exiting" << std::endl);
}
