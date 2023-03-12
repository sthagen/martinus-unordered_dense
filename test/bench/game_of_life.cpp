#include <ankerl/unordered_dense.h> // for map

#include <app/counting_allocator.h>
#include <app/name_of_type.h> // for name_of_type

#include <app/doctest.h> // for TestCase, skip, ResultBuilder
#include <fmt/core.h>    // for format, print

#include <vector>

#if __has_include("boost/unordered/unordered_flat_map.hpp")
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wold-style-cast"
#    include "boost/unordered/unordered_flat_map.hpp"
#    define HAS_BOOST_UNORDERED_FLAT_MAP() 1 // NOLINT(cppcoreguidelines-macro-usage)
#else
#    define HAS_BOOST_UNORDERED_FLAT_MAP() 0 // NOLINT(cppcoreguidelines-macro-usage)
#endif

class vec2 {
    uint32_t m_xy;

public:
    constexpr vec2(uint16_t x, uint16_t y)
        : m_xy{(static_cast<uint32_t>(x) << 16U) | y} {}

    constexpr explicit vec2(uint32_t xy)
        : m_xy(xy) {}

    [[nodiscard]] constexpr auto pack() const -> uint32_t {
        return m_xy;
    };

    constexpr void add_x(uint32_t x) {
        m_xy += x << 16U;
    }

    constexpr void add_y(uint32_t y) {
        m_xy += y;
    }

    template <typename Op>
    constexpr void for_each_surrounding(Op&& op) const {
        op(m_xy - 0x10000 - 1);
        op(m_xy - 0x10000);
        op(m_xy - 0x10000 + 1);
        op(m_xy - 1);
        // op(m_xy);
        op(m_xy + 1);
        op(m_xy + 0x10000 - 1);
        op(m_xy + 0x10000);
        op(m_xy + 0x10000 + 1);
    }
};

template <typename Map>
void game_of_life(std::string_view name, size_t nsteps, size_t final_population, Map map1, std::vector<vec2> state) {
    auto before = std::chrono::steady_clock::now();
    map1.clear();
    auto map2 = map1; // copy the empty map so we get the allocator

    for (auto& v : state) {
        v.add_x(UINT16_MAX / 2);
        v.add_y(UINT16_MAX / 2);
        map1[v.pack()] = true;
        v.for_each_surrounding([&](uint32_t xy) {
            map1.emplace(xy, false);
        });
    }

    auto* m1 = &map1;
    auto* m2 = &map2;
    for (size_t i = 0; i < nsteps; ++i) {
        for (auto const kv : *m1) {
            auto const& pos = kv.first;
            auto alive = kv.second;
            int neighbors = 0;
            vec2{pos}.for_each_surrounding([&](uint32_t xy) {
                if (auto x = m1->find(xy); x != m1->end()) {
                    neighbors += x->second;
                }
            });
            if ((alive && (neighbors == 2 || neighbors == 3)) || (!alive && neighbors == 3)) {
                (*m2)[pos] = true;
                vec2{pos}.for_each_surrounding([&](uint32_t xy) {
                    m2->emplace(xy, false);
                });
            }
        }
        m1->clear();
        std::swap(m1, m2);
    }

    size_t count = 0;
    for (auto const kv : *m1) {
        count += kv.second;
    }

    REQUIRE(count == final_population);
    auto after = std::chrono::steady_clock::now();
    fmt::print("{}s {} {}\n", std::chrono::duration<double>(after - before).count(), name, name_of_type<Map>());
}

TEST_CASE_MAP("gameoflife_r-pentomino" * doctest::test_suite("bench") * doctest::skip(), uint32_t, bool) {
    // https://conwaylife.com/wiki/R-pentomino
    auto map = map_t();
    game_of_life("R-pentomino", 10'000, 116, map, {{1, 0}, {2, 0}, {0, 1}, {1, 1}, {1, 2}});
}

TEST_CASE_MAP("gameoflife_acorn" * doctest::test_suite("bench") * doctest::skip(), uint32_t, bool) {
    // https://conwaylife.com/wiki/R-pentomino
    auto map = map_t();
    game_of_life("Acorn", 5206, 633, map, {{1, 0}, {3, 1}, {0, 2}, {1, 2}, {4, 2}, {5, 2}, {6, 2}});
}

using hash_t = ankerl::unordered_dense::hash<uint32_t>;
using eq_t = std::equal_to<uint32_t>;
using pair_t = std::pair<uint32_t, bool>;
using alloc_t = counting_allocator<pair_t>;

TEST_CASE("gameoflife_gotts-dots" * doctest::test_suite("bench") * doctest::skip()) {
    // https://conwaylife.com/wiki/Gotts_dots^
    auto state = std::vector<vec2>{
        {0, 0},    {0, 1},    {0, 2},                                                                                 // 1
        {4, 11},   {5, 12},   {6, 13},   {7, 12},   {8, 11},                                                          // 2
        {9, 13},   {9, 14},   {9, 15},                                                                                // 3
        {185, 24}, {186, 25}, {186, 26}, {186, 27}, {185, 27}, {184, 27}, {183, 27}, {182, 26},                       // 4
        {179, 28}, {180, 29}, {181, 29}, {179, 30},                                                                   // 5
        {182, 32}, {183, 31}, {184, 31}, {185, 31}, {186, 31}, {186, 32}, {186, 33}, {185, 34},                       // 6
        {175, 35}, {176, 36}, {170, 37}, {176, 37}, {171, 38}, {172, 38}, {173, 38}, {174, 38}, {175, 38}, {176, 38}, // 7
    };
    // size_t nsteps = 2000;
    // size_t final_population = 4599;

    // size_t nsteps = 4000;
    // size_t final_population = 7754;

    size_t nsteps = 10000;
    size_t final_population = 16665;

    {
        auto counters = counts_for_allocator{};
        using map_t = ankerl::unordered_dense::map<uint32_t, bool, hash_t, eq_t, alloc_t>;
        auto map = map_t{0, hash_t{}, eq_t{}, alloc_t{&counters}};
        game_of_life("Gotts dots", nsteps, final_population, map, state);
        counters.save("gottsdots_map.txt");
    }
    {
        auto counters = counts_for_allocator{};
        using map_t = ankerl::unordered_dense::segmented_map<uint32_t, bool, hash_t, eq_t, alloc_t>;
        auto map = map_t{0, hash_t{}, eq_t{}, alloc_t{&counters}};
        game_of_life("Gotts dots", nsteps, final_population, map, state);
        counters.save("gottsdots_segmented_map.txt");
    }

#if HAS_BOOST_UNORDERED_FLAT_MAP()
    {
        auto counters = counts_for_allocator{};
        using map_t = boost::unordered_flat_map<uint32_t, bool, hash_t, eq_t, alloc_t>;
        auto map = map_t{0, hash_t{}, eq_t{}, alloc_t{&counters}};
        game_of_life("Gotts dots", nsteps, final_population, map, state);
        counters.save("gottsdots_boost_unordered_flat_map.txt");
    }
#endif
}

#if 0

    // https://conwaylife.com/wiki/Acorn
    game_of_life(bench, "Acorn", 5206, 633, {{1, 0}, {3, 1}, {0, 2}, {1, 2}, {4, 2}, {5, 2}, {6, 2}});

    // https://conwaylife.com/wiki/Jaydot
    game_of_life(bench, "Jaydot", 6929, 1124, {{1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 1}, {1, 3}, {1, 4}, {2, 4}, {0, 5}});

    // https://conwaylife.com/wiki/Bunnies
    game_of_life(bench, "Bunnies", 17332, 1744, {{0, 0}, {6, 0}, {2, 1}, {6, 1}, {2, 2}, {5, 2}, {7, 2}, {1, 3}, {3, 3}});
}


    // https://conwaylife.com/wiki/Puffer_2
    game_of_life(bench,
                 "Puffer 2",
                 2000,
                 7400,
                 {
                     {1, 0}, {2, 0}, {3, 0},  {15, 0}, {16, 0}, {17, 0}, // line 0
                     {0, 1}, {3, 1}, {14, 1}, {17, 1},                   // line 1
                     {3, 2}, {8, 2}, {9, 2},  {10, 2}, {17, 2},          // line 2
                     {3, 3}, {8, 3}, {11, 3}, {17, 3},                   // line 3
                     {2, 4}, {7, 4}, {16, 4},                            // line 4
                 });
}

#endif