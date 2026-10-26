#pragma once

#include "nws/detail/url_encode.hpp"
#include "nws/error.hpp"

#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_set>
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
		: fetch_fn_(std::move(fetch_fn)), params_(std::move(initial_params)) {
		if (params_.cursor && !params_.cursor->empty()) {
			seen_cursors_.insert(params_.cursor->value);
		}
	}

	/// Fetch the next page of results
	[[nodiscard]] Result<std::vector<T>> next() {
		if (exhausted_) {
			return std::vector<T>{};
		}

		Result<PaginatedResponse<T>> result = fetch_fn_(params_);
		if (!result.has_value()) {
			return std::unexpected(result.error());
		}

		PaginatedResponse<T>& page = *result;
		if (page.has_more()) {
			if (!seen_cursors_.insert(page.next_cursor->value).second) {
				exhausted_ = true;
				return std::unexpected(Error::parse("Pagination cursor repeated"));
			}
			params_.cursor = std::move(page.next_cursor);
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
			Result<std::vector<T>> page = next();
			if (!page.has_value()) {
				return std::unexpected(page.error());
			}
			all.insert(all.end(), std::make_move_iterator(page->begin()),
					   std::make_move_iterator(page->end()));
		}
		return all;
	}

private:
	// One cursor per page prevents arbitrary continuation cycles.
	std::unordered_set<std::string> seen_cursors_;
	FetchFn fetch_fn_;
	PaginationParams params_;
	bool exhausted_{false};
};

/// Build query string parameters for pagination
[[nodiscard]] inline std::string build_paginated_query(const PaginationParams& params) {
	std::string query;
	if (params.cursor.has_value() && !params.cursor->empty()) {
		query += "cursor=" + detail::encode_query_value(params.cursor->value);
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
