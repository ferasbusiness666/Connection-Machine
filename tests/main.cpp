#include <gtest/gtest.h>
#include "computerAPI/directoryManager.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    DirectoryManager::findDirectories();
    return RUN_ALL_TESTS();
}
