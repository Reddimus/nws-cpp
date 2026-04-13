#pragma once

#include "nws/error.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nws {

/// Opaque cursor for pagination
struct Cursor {
	std::string value;

	[[nodiscard]] bool empty() const noexcept { return value.empty(); }
	explicit operator bool() const noexcept { return !value.empty(); }
};

/// Parameters for paginated requests
struct PaginationParams {
	std::optional<Cursor> cursor;
	std::optional<std::int32_t> limit;
};

/// A page of results with optional continuation cursor
template <typename T>
struct PaginatedResponse {
	std::vector<T> data;
	std::optional<Cursor> next_cursor;

	[[nodiscard]] bool has_more() const noexcept {
		return next_cursor.has_value() && !next_cursor->empty();
	}
};

/// Iterator for automatically fetching pages
template <typename T>
class PaginatedIterator {
public:
	using FetchFn = std::function<Result<PaginatedResponse<T>>(const PaginationParams&)>;

	explicit PaginatedIterator(FetchFn fetch_fn, PaginationParams initial_params = {})
		: fetch_fn_(std::move(fetch_fn)), params_(std::move(initial_params)) {}

	/// Fetch the next page of results
	[[nodiscard]] Result<std::vector<T>> next() {
		if (exhausted_) {
			return std::vector<T>{};
		}

		auto result = fetch_fn_(params_);
		if (!result.has_value()) {
			return std::unexpected(result.error());
		}

		auto& page = *result;
		if (page.has_more()) {
			params_.cursor = page.next_cursor;
		} else {
			exhausted_ = true;
		}

		return std::move(page.data);
	}

	/// Check if all pages have been consumed
	[[nodiscard]] bool is_exhausted() const noexcept { return exhausted_; }

	/// Collect all remaining pages into a single vector
	[[nodiscard]] Result<std::vector<T>> collect_all() {
		std::vector<T> all;
		while (!exhausted_) {
			auto page = next();
			if (!page.has_value()) {
				return std::unexpected(page.error());
			}
			all.insert(all.end(), std::make_move_iterator(page->begin()),
					   std::make_move_iterator(page->end()));
		}
		return all;
	}

private:
	FetchFn fetch_fn_;
	PaginationParams params_;
	bool exhausted_{false};
};

/// Build query string parameters for pagination
[[nodiscard]] inline std::string build_paginated_query(const PaginationParams& params) {
	std::string query;
	if (params.cursor.has_value() && !params.cursor->empty()) {
		query += "cursor=" + params.cursor->value;
	}
	if (params.limit.has_value()) {
		if (!query.empty()) {
			query += '&';
		}
		query += "limit=" + std::to_string(*params.limit);
	}
	return query;
}

} // namespace nws
