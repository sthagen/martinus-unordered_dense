#include <ankerl/unordered_dense.h> // for map, operator==

#include <app/counting_allocator.h>

#include <third-party/nanobench.h>

#if __has_include("tsl/sparse_map.h")
#    include "tsl/sparse_map.h"
#    define HAS_TSL_SPARSE_MAP() 1
#else
#    define HAS_TSL_SPARSE_MAP() 0
#endif

#include <doctest.h>
#include <fmt/ostream.h>

#include <deque>
#include <filesystem>
#include <fstream>
#include <unordered_map>

template <typename Map>
void evaluate_map(Map& map) {
    auto rng = ankerl::nanobench::Rng{1234};

    auto num_elements = size_t(200'000);
    for (uint64_t i = 0; i < num_elements; ++i) {
        map[rng()] = i;
    }
    REQUIRE(map.size() == num_elements);
}

using hash_t = ankerl::unordered_dense::hash<uint64_t>;
using eq_t = std::equal_to<uint64_t>;
using pair_t = std::pair<uint64_t, uint64_t>;
using alloc_t = counting_allocator<pair_t>;

void save_measures(counts_for_allocator::measures_type const& measures, std::filesystem::path const& filename) {
    auto fout = std::ofstream(filename);
    for (auto [dur, bytes] : measures) {
        fmt::print(fout, "{}; {}\n", std::chrono::duration<double>(dur).count(), bytes);
    }
}

TEST_CASE("allocated_memory_std_vector" * doctest::skip()) {
    auto counters = counts_for_allocator{};
    {
        using vec_t = std::vector<pair_t, alloc_t>;
        using map_t = ankerl::unordered_dense::map<uint64_t, uint64_t, hash_t, eq_t, vec_t>;
        auto map = map_t(0, hash_t{}, eq_t{}, alloc_t{&counters});
        evaluate_map(map);
    }
    save_measures(counters.calc_measurements(), "allocated_memory_std_vector.txt");
}

#if HAS_TSL_SPARSE_MAP()

TEST_CASE("allocated_memory_tsl_sparse_map" * doctest::skip()) {
    auto counters = counts_for_allocator{};
    {
        using map_t = tsl::sparse_map<uint64_t, uint64_t, hash_t, eq_t, alloc_t>;
        auto map = map_t(alloc_t{&counters});
        evaluate_map(map);
    }
    save_measures(counters.calc_measurements(), "allocated_memory_tsl_sparse_map.txt");
}
#endif

TEST_CASE("allocated_memory_std_deque" * doctest::skip()) {
    auto counters = counts_for_allocator{};
    {
        using vec_t = std::deque<pair_t, alloc_t>;
        using map_t = ankerl::unordered_dense::map<uint64_t, uint64_t, hash_t, eq_t, vec_t>;
        auto map = map_t(0, hash_t{}, eq_t{}, alloc_t{&counters});
        evaluate_map(map);
    }
    save_measures(counters.calc_measurements(), "allocated_memory_std_deque.txt");
}

TEST_CASE("allocated_memory_segmented_vector" * doctest::skip()) {
    auto counters = counts_for_allocator{};
    {
        using vec_t = ankerl::unordered_dense::segmented_vector<pair_t, alloc_t>;
        using map_t = ankerl::unordered_dense::segmented_map<uint64_t, uint64_t, hash_t, eq_t, vec_t>;
        auto map = map_t{0, hash_t{}, eq_t{}, alloc_t{&counters}};
        evaluate_map(map);
    }
    save_measures(counters.calc_measurements(), "allocated_memory_segmented_vector.txt");
}
