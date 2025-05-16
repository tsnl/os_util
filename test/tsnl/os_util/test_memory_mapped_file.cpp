#include <tsnl/os_util/memory_mapped_file.hpp>

#include <filesystem>
#include <fstream>
#include <random>
#include <vector>

#include <cstdlib>

#include <gtest/gtest.h>

namespace tsnl::os_util {

class memory_mapped_file_test : public ::testing::Test {
protected:
    memory_mapped_file_test() = default;
    ~memory_mapped_file_test() override = default;
};

TEST_F(memory_mapped_file_test, read_file) {
    auto file_path = std::filesystem::temp_directory_path() / "test_memory_mapped_file.bin";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<int> size_dist(1024, 2048);

    // Generate random bytes:
    std::vector<std::byte> data(size_dist(rng), std::byte{0});
    for (auto& byte : data) {
        byte = std::byte{static_cast<std::byte>(byte_dist(rng))};
    }

    // Write bytes to a test file
    std::ofstream ofs(file_path);
    ASSERT_TRUE(ofs.is_open());
    ofs.write(reinterpret_cast<char*>(data.data()), data.size());
    ofs.close();

    // Map the file and check the contents
    memory_mapped_file f(file_path, memory_mapped_file::flag::read);
    EXPECT_EQ(f.size(), data.size());
    EXPECT_EQ(f.access().size(), data.size());
    EXPECT_EQ(std::memcmp(f.access().data(), data.data(), data.size()), 0);
}

} // namespace tsnl::os_util
