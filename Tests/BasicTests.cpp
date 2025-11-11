/*
 * BasicTests.cpp
 */
#include "catch_amalgamated.hpp"

TEST_CASE("Basic math tests", "[basic]")
{
	SECTION("Addition")
	{
		REQUIRE(1 + 1 == 2);
		REQUIRE(2 + 2 == 4);
	}
}
