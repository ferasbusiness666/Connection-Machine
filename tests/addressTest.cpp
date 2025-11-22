#include <gtest/gtest.h>

#include "backend/address.h"
#include "backend/position/position.h"

TEST(AddressTest, DefaultConstruction) {
	Address address;

	EXPECT_EQ(address.size(), 0);
	EXPECT_TRUE(address.toString().empty());
}

TEST(AddressTest, ConstructionWithPosition) {
	Position pos(3, -4);
	Address address(pos);

	ASSERT_EQ(address.size(), 1);
	EXPECT_EQ(address.getPosition(0), pos);
	EXPECT_EQ(address.toString(), "(3, -4)");
}

TEST(AddressTest, AddAndNestPositions) {
	Address address(Position(0, 0));
	address.addBlockId(Position(1, 1));

	ASSERT_EQ(address.size(), 2);
	EXPECT_EQ(address.getPosition(0), Position(0, 0));
	EXPECT_EQ(address.getPosition(1), Position(1, 1));

	address.nestPosition(Position(-5, 6));

	ASSERT_EQ(address.size(), 3);
	EXPECT_EQ(address.getPosition(0), Position(-5, 6));
	EXPECT_EQ(address.toString(), "(-5, 6).(0, 0).(1, 1)");
}
