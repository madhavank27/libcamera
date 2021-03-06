/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * utils.h - Miscellaneous utility functions
 */
#ifndef __LIBCAMERA_UTILS_H__
#define __LIBCAMERA_UTILS_H__

#include <algorithm>
#include <chrono>
#include <memory>
#include <ostream>
#include <string>
#include <sys/time.h>

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

namespace libcamera {

namespace utils {

const char *basename(const char *path);

/* C++11 doesn't provide std::make_unique */
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

char *secure_getenv(const char *name);
std::string readlink(const char *path);
std::string dirname(const std::string &path);

template<class InputIt1, class InputIt2>
unsigned int set_overlap(InputIt1 first1, InputIt1 last1,
			 InputIt2 first2, InputIt2 last2)
{
	unsigned int count = 0;

	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			++first1;
		} else {
			if (!(*first2 < *first1))
				count++;
			++first2;
		}
	}

	return count;
}

/* C++11 doesn't provide std::clamp */
template <typename T>
const T& clamp(const T& v, const T& lo, const T& hi)
{
	return std::max(lo, std::min(v, hi));
}

using clock = std::chrono::steady_clock;
using duration = std::chrono::steady_clock::duration;
using time_point = std::chrono::steady_clock::time_point;

struct timespec duration_to_timespec(const duration &value);
std::string time_point_to_string(const time_point &time);

#ifndef __DOXYGEN__
struct _hex {
	uint64_t v;
	unsigned int w;
};

std::basic_ostream<char, std::char_traits<char>> &
operator<<(std::basic_ostream<char, std::char_traits<char>> &stream, const _hex &h);
#endif

template<typename T>
_hex hex(T value, unsigned int width = 0);

#ifndef __DOXYGEN__
template<>
inline _hex hex<int32_t>(int32_t value, unsigned int width)
{
	return { static_cast<uint64_t>(value), width ? width : 8 };
}

template<>
inline _hex hex<uint32_t>(uint32_t value, unsigned int width)
{
	return { static_cast<uint64_t>(value), width ? width : 8 };
}

template<>
inline _hex hex<int64_t>(int64_t value, unsigned int width)
{
	return { static_cast<uint64_t>(value), width ? width : 16 };
}

template<>
inline _hex hex<uint64_t>(uint64_t value, unsigned int width)
{
	return { static_cast<uint64_t>(value), width ? width : 16 };
}
#endif

} /* namespace utils */

} /* namespace libcamera */

#endif /* __LIBCAMERA_UTILS_H__ */
