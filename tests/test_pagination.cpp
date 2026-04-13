#include "nws/pagination.hpp"

#include <gtest/gtest.h>

namespace nws {
namespace {

TEST(CursorTest, EmptyByDefault) {
	Cursor c;
	EXPECT_TRUE(c.empty());
	EXPECT_FALSE(static_cast<bool>(c));
}

TEST(CursorTest, NonEmpty) {
	Cursor c{"abc123"};
	EXPECT_FALSE(c.empty());
	EXPECT_TRUE(static_cast<bool>(c));
}

TEST(PaginatedResponseTest, HasMoreWithCursor) {
	PaginatedResponse<int> resp;
	resp.data = {1, 2, 3};
	resp.next_cursor = Cursor{"next"};
	EXPECT_TRUE(resp.has_more());
}

TEST(PaginatedResponseTest, NoMoreWithoutCursor) {
	PaginatedResponse<int> resp;
	resp.data = {1, 2, 3};
	EXPECT_FALSE(resp.has_more());
}

TEST(PaginatedResponseTest, NoMoreWithEmptyCursor) {
	PaginatedResponse<int> resp;
	resp.data = {1, 2, 3};
	resp.next_cursor = Cursor{""};
	EXPECT_FALSE(resp.has_more());
}

TEST(BuildPaginatedQueryTest, EmptyParams) {
	PaginationParams params;
	EXPECT_EQ(build_paginated_query(params), "");
}

TEST(BuildPaginatedQueryTest, LimitOnly) {
	PaginationParams params;
	params.limit = 50;
	EXPECT_EQ(build_paginated_query(params), "limit=50");
}

TEST(BuildPaginatedQueryTest, CursorOnly) {
	PaginationParams params;
	params.cursor = Cursor{"abc"};
	EXPECT_EQ(build_paginated_query(params), "cursor=abc");
}

TEST(BuildPaginatedQueryTest, BothParams) {
	PaginationParams params;
	params.cursor = Cursor{"abc"};
	params.limit = 100;
	auto result = build_paginated_query(params);
	EXPECT_NE(result.find("cursor=abc"), std::string::npos);
	EXPECT_NE(result.find("limit=100"), std::string::npos);
}

TEST(PaginatedIteratorTest, SinglePage) {
	int call_count = 0;
	PaginatedIterator<int> iter([&](const PaginationParams&) -> Result<PaginatedResponse<int>> {
		++call_count;
		PaginatedResponse<int> resp;
		resp.data = {1, 2, 3};
		// No next_cursor = last page
		return resp;
	});

	auto page = iter.next();
	ASSERT_TRUE(page.has_value());
	EXPECT_EQ(page->size(), 3);
	EXPECT_TRUE(iter.is_exhausted());
	EXPECT_EQ(call_count, 1);
}

TEST(PaginatedIteratorTest, MultiplePages) {
	int call_count = 0;
	PaginatedIterator<int> iter([&](const PaginationParams&) -> Result<PaginatedResponse<int>> {
		++call_count;
		PaginatedResponse<int> resp;
		if (call_count == 1) {
			resp.data = {1, 2};
			resp.next_cursor = Cursor{"page2"};
		} else {
			resp.data = {3, 4};
		}
		return resp;
	});

	auto all = iter.collect_all();
	ASSERT_TRUE(all.has_value());
	EXPECT_EQ(all->size(), 4);
	EXPECT_EQ(call_count, 2);
}

TEST(PaginatedIteratorTest, ErrorPropagation) {
	PaginatedIterator<int> iter([](const PaginationParams&) -> Result<PaginatedResponse<int>> {
		return std::unexpected(Error::network("fail"));
	});

	auto page = iter.next();
	ASSERT_FALSE(page.has_value());
	EXPECT_EQ(page.error().code, ErrorCode::NetworkError);
}

TEST(PaginatedIteratorTest, ExhaustedReturnsEmpty) {
	int call_count = 0;
	PaginatedIterator<int> iter([&](const PaginationParams&) -> Result<PaginatedResponse<int>> {
		++call_count;
		return PaginatedResponse<int>{{1}, std::nullopt};
	});

	auto page1 = iter.next();
	ASSERT_TRUE(page1.has_value());
	EXPECT_TRUE(iter.is_exhausted());

	auto page2 = iter.next();
	ASSERT_TRUE(page2.has_value());
	EXPECT_TRUE(page2->empty());
	EXPECT_EQ(call_count, 1); // Should not call fetch again
}

} // namespace
} // namespace nws
