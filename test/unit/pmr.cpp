#include <ankerl/unordered_dense.h>

#include <doctest.h>
#include <fmt/format.h>

#include <memory_resource>

class logging_memory_resource : public std::pmr::memory_resource {
    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override {
        fmt::print("+ {} bytes, {} alignment\n", bytes, alignment);
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        fmt::print("- {} bytes, {} alignment, {} ptr\n", bytes, alignment, p);
        return std::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
    }

    [[nodiscard]] auto do_is_equal(const std::pmr::memory_resource& other) const noexcept -> bool override {
        return this == &other;
    }
};

class track_peak_memory_resource : public std::pmr::memory_resource {
    uint64_t m_peak = 0;
    uint64_t m_current = 0;
    uint64_t m_num_allocs = 0;
    uint64_t m_num_deallocs = 0;
    mutable uint64_t m_num_is_equals = 0;

    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override {
        ++m_num_allocs;
        m_current += bytes;
        if (m_current > m_peak) {
            m_peak = m_current;
        }
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        if (p == nullptr) {
            return;
        }
        ++m_num_deallocs;
        m_current -= bytes;
        return std::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
    }

    [[nodiscard]] auto do_is_equal(const std::pmr::memory_resource& other) const noexcept -> bool override {
        ++m_num_is_equals;
        return this == &other;
    }

public:
    [[nodiscard]] auto current() const -> uint64_t {
        return m_current;
    }

    [[nodiscard]] auto peak() const -> uint64_t {
        return m_peak;
    }

    [[nodiscard]] auto num_allocs() const -> uint64_t {
        return m_num_allocs;
    }

    [[nodiscard]] auto num_deallocs() const -> uint64_t {
        return m_num_deallocs;
    }

    [[nodiscard]] auto num_is_equals() const -> uint64_t {
        return m_num_is_equals;
    }
};

TEST_CASE("pmr") {
    auto mr = track_peak_memory_resource();
    {
        REQUIRE(mr.current() == 0);
        auto map = ankerl::unordered_dense::pmr::map<uint64_t, uint64_t>(&mr);
        REQUIRE(mr.current() == 0);

        for (size_t i = 0; i < 1; ++i) {
            map[i] = i;
        }
        REQUIRE(mr.current() != 0);

        // gets a copy, but it has the same memory resource
        auto alloc = map.get_allocator();
        REQUIRE(alloc.resource() == &mr);
    }
    REQUIRE(mr.current() == 0);
}

void show([[maybe_unused]] track_peak_memory_resource const& mr, [[maybe_unused]] std::string_view name) {
    // fmt::print("{}: {} allocs, {} deallocs, {} is_equals\n", name, mr.num_allocs(), mr.num_deallocs(), mr.num_is_equals());
}

TEST_CASE("pmr_copy") {
    auto mr1 = track_peak_memory_resource();
    auto map1 = ankerl::unordered_dense::pmr::map<uint64_t, uint64_t>(&mr1);
    map1[1] = 2;

    auto mr2 = track_peak_memory_resource();
    auto map2 = ankerl::unordered_dense::pmr::map<uint64_t, uint64_t>(&mr2);
    map2[3] = 4;
    show(mr1, "mr1");
    show(mr2, "mr2");

    map1 = map2;
    REQUIRE(map1.size() == 1);
    REQUIRE(map1.find(3) != map1.end());
    show(mr1, "mr1");
    show(mr2, "mr2");

    REQUIRE(mr1.num_allocs() == 3);
    REQUIRE(mr1.num_deallocs() == 1);
    REQUIRE(mr1.num_is_equals() == 0);

    REQUIRE(mr2.num_allocs() == 2);
    REQUIRE(mr2.num_deallocs() == 0);
    REQUIRE(mr2.num_is_equals() == 0);
}

TEST_CASE("pmr_move_different_mr") {
    auto mr1 = track_peak_memory_resource();
    auto map1 = ankerl::unordered_dense::pmr::map<uint64_t, uint64_t>(&mr1);
    map1[1] = 2;

    auto mr2 = track_peak_memory_resource();
    auto map2 = ankerl::unordered_dense::pmr::map<uint64_t, uint64_t>(&mr2);
    map2[3] = 4;
    show(mr1, "mr1");
    show(mr2, "mr2");

    map1 = std::move(map2);
    REQUIRE(map1.size() == 1);
    REQUIRE(map1.find(3) != map1.end());
    show(mr1, "mr1");
    show(mr2, "mr2");

    REQUIRE(mr1.num_allocs() == 2);
    REQUIRE(mr1.num_deallocs() == 1);
    REQUIRE(mr1.num_is_equals() == 0);

    REQUIRE(mr2.num_allocs() == 2);
    REQUIRE(mr2.num_deallocs() == 0);
    REQUIRE(mr2.num_is_equals() == 1);
}

TEST_CASE("pmr_move_same_mr") {
    auto mr1 = track_peak_memory_resource();
    auto map1 = ankerl::unordered_dense::pmr::map<uint64_t, uint64_t>(&mr1);
    map1[1] = 2;
    REQUIRE(mr1.num_allocs() == 2);
    REQUIRE(mr1.num_deallocs() == 0);
    REQUIRE(mr1.num_is_equals() == 0);

    auto map2 = ankerl::unordered_dense::pmr::map<uint64_t, uint64_t>(&mr1);
    map2[3] = 4;
    show(mr1, "mr1");

    map1 = std::move(map2);
    REQUIRE(map1.size() == 1);
    REQUIRE(map1.find(3) != map1.end());
    show(mr1, "mr1");

    REQUIRE(mr1.num_allocs() == 4);
    REQUIRE(mr1.num_deallocs() == 2);
    REQUIRE(mr1.num_is_equals() == 0);
}
