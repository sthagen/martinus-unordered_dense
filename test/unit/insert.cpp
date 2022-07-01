#include <ankerl/unordered_dense.h>

#include <doctest.h>

TEST_CASE("insert") {
    using Map = ankerl::unordered_dense::map<unsigned int, int>;
    auto map = Map();
    typename Map::value_type val(123U, 321);
    map.insert(val);
    REQUIRE(map.size() == 1);

    REQUIRE(map[123U] == 321);
}

TEST_CASE("insert_hint") {
    auto map = ankerl::unordered_dense::map<unsigned int, int>();
    auto it = map.insert(map.begin(), {1, 2});

    auto vt = std::pair<unsigned int, float>{3, 4.2};
    map.insert(it, vt);
    REQUIRE(map.size() == 2);
    REQUIRE(map[1] == 2);
    REQUIRE(map[3] == 4);

    auto const vt2 = decltype(map)::value_type{10, 11};
    map.insert(it, vt2);
    REQUIRE(map.size() == 3);
    REQUIRE(map[10] == 11);

    it = map.emplace_hint(it, std::piecewise_construct, std::forward_as_tuple(123), std::forward_as_tuple(321));
    REQUIRE(map.size() == 4);
    REQUIRE(map[123] == 321);
}
