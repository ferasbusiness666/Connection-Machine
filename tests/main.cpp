#include <gtest/gtest.h>
#include "computerAPI/directoryManager.h"

std::thread::id mainThreadId = std::this_thread::get_id();

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    DirectoryManager::findDirectories();
    return RUN_ALL_TESTS();
}
