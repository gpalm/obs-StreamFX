/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2018 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#pragma once
#include <cinttypes>
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

extern "C" {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <obs.h>
#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/vec4.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

// Constants
#define S_PI 3.1415926535897932384626433832795        // PI = pi
#define S_PI2 6.283185307179586476925286766559        // 2PI = 2 * pi
#define S_PI2_SQROOT 2.506628274631000502415765284811 // sqrt(2 * pi)
#define S_RAD 57.295779513082320876798154814105       // 180/pi
#define S_DEG 0.01745329251994329576923690768489      // pi/180
#define D_DEG_TO_RAD(x) (x * S_DEG)
#define D_RAD_TO_DEG(x) (x * S_RAD)

#define D_STR(s) #s
#define D_VSTR(s) D_STR(s)

namespace streamfx::util {
	bool inline are_property_groups_broken()
	{
		return obs_get_version() < MAKE_SEMANTIC_VERSION(24, 0, 0);
	}

	obs_property_t* obs_properties_add_tristate(obs_properties_t* props, const char* name, const char* desc);

	inline bool is_tristate_enabled(int64_t tristate)
	{
		return tristate == 1;
	}

	inline bool is_tristate_disabled(int64_t tristate)
	{
		return tristate == 0;
	}

	inline bool is_tristate_default(int64_t tristate)
	{
		return tristate == -1;
	}

	struct vec2a : public vec2 {
		// 16-byte Aligned version of vec2
		static void* operator new(std::size_t count);
		static void* operator new[](std::size_t count);
		static void  operator delete(void* p);
		static void  operator delete[](void* p);
	};

#ifdef _MSC_VER
	__declspec(align(16))
#endif
		struct vec3a : public vec3 {
		// 16-byte Aligned version of vec3
		static void* operator new(std::size_t count);
		static void* operator new[](std::size_t count);
		static void  operator delete(void* p);
		static void  operator delete[](void* p);
	};

#ifdef _MSC_VER
	__declspec(align(16))
#endif
		struct vec4a : public vec4 {
		// 16-byte Aligned version of vec4
		static void* operator new(std::size_t count);
		static void* operator new[](std::size_t count);
		static void  operator delete(void* p);
		static void  operator delete[](void* p);
	};

	std::pair<int64_t, int64_t> size_from_string(std::string text, bool allowSquare = true);

	namespace math {
		template<typename T>
		inline T pow(T base, T exp)
		{
			T res = 1;
			while (exp) {
				if (exp & 1)
					res *= base;
				exp >>= 1;
				base *= base;
			}
			return res;
		}

		// Proven by tests to be the fastest implementation on Intel and AMD CPUs.
		// Ranking: log10, loop < bitscan < pow
		// loop and log10 trade blows, usually almost identical.
		// loop is used for integers, log10 for anything else.
		template<typename T>
		inline bool is_power_of_two(T v)
		{
			return T(1ull << uint64_t(floor(log10(T(v)) / log10(2.0)))) == v;
		}

		template<typename T>
		inline bool is_power_of_two_loop(T v)
		{
			bool have_bit = false;
			for (std::size_t index = 0; index < (sizeof(T) * 8); index++) {
				bool cur = (v & (static_cast<T>(1ull) << index)) != 0;
				if (cur) {
					if (have_bit)
						return false;
					have_bit = true;
				}
			}
			return true;
		}

#pragma push_macro("P_IS_POWER_OF_TWO_AS_LOOP")
#define P_IS_POWER_OF_TWO_AS_LOOP(x)    \
	template<>                          \
	inline bool is_power_of_two(x v)    \
	{                                   \
		return is_power_of_two_loop(v); \
	}
		P_IS_POWER_OF_TWO_AS_LOOP(int8_t)
		P_IS_POWER_OF_TWO_AS_LOOP(uint8_t)
		P_IS_POWER_OF_TWO_AS_LOOP(int16_t)
		P_IS_POWER_OF_TWO_AS_LOOP(uint16_t)
		P_IS_POWER_OF_TWO_AS_LOOP(int32_t)
		P_IS_POWER_OF_TWO_AS_LOOP(uint32_t)
		P_IS_POWER_OF_TWO_AS_LOOP(int64_t)
		P_IS_POWER_OF_TWO_AS_LOOP(uint64_t)
#undef P_IS_POWER_OF_TWO_AS_LOOP
#pragma pop_macro("P_IS_POWER_OF_TWO_AS_LOOP")

		template<typename T>
		inline uint64_t get_power_of_two_exponent_floor(T v)
		{
			return uint64_t(floor(log10(T(v)) / log10(2.0)));
		}

		template<typename T>
		inline uint64_t get_power_of_two_exponent_ceil(T v)
		{
			return uint64_t(ceil(log10(T(v)) / log10(2.0)));
		}

		template<typename T, typename C>
		inline bool is_equal(T target, C value)
		{
			return (target > (value - std::numeric_limits<T>::epsilon()))
				   && (target < (value + std::numeric_limits<T>::epsilon()));
		}

		template<typename T>
		inline bool is_close(T target, T value, T delta)
		{
			return (target > (value - delta)) && (target < (value + delta));
		}

		template<typename T>
		inline std::vector<T> pascal_triangle(size_t n)
		{
			std::vector<T> line;
			line.push_back(1);
			for (uint64_t k = 0; k < n; k++) {
				T v = static_cast<T>(line.at(k) * static_cast<double_t>(n - k) / static_cast<double_t>(k + 1));
				line.push_back(v);
			}
			return line;
		}

		template<typename T>
		inline T gaussian(T x, T o /*, T u = 0*/)
		{
			// u/µ can be simulated by subtracting that value from x.
			//static const double_t pi            = 3.1415926535897932384626433832795;
			//static const double_t two_pi        = pi * 2.;
			static const double_t two_pi_sqroot = 2.506628274631000502415765284811; //sqrt(two_pi);

			if (is_equal<double_t>(0, o)) {
				return T(std::numeric_limits<double_t>::infinity());
			}

			// g(x) = (1 / o√(2Π)) * e(-(1/2) * ((x-u)/o)²)
			double_t left_e      = 1. / (o * two_pi_sqroot);
			double_t mid_right_e = ((x /* - u*/) / o);
			double_t right_e     = -0.5 * mid_right_e * mid_right_e;
			double_t final       = left_e * exp(right_e);

			return T(final);
		}

		template<typename T>
		inline T lerp(T a, T b, double_t v)
		{
			return static_cast<T>((static_cast<double_t>(a) * (1.0 - v)) + (static_cast<double_t>(b) * v));
		}

		template<typename T>
		class kalman1D {
			T _q_process_noise_covariance;
			T _r_measurement_noise_covariance;
			T _x_value_of_interest;
			T _p_estimation_error_covariance;
			T _k_kalman_gain;

			public:
			kalman1D()
				: _q_process_noise_covariance(0), _r_measurement_noise_covariance(0), _x_value_of_interest(0),
				  _p_estimation_error_covariance(0), _k_kalman_gain(0.0)
			{}
			kalman1D(T pnc, T mnc, T eec, T value)
				: _q_process_noise_covariance(pnc), _r_measurement_noise_covariance(mnc), _x_value_of_interest(value),
				  _p_estimation_error_covariance(eec), _k_kalman_gain(0.0)
			{}
			~kalman1D() {}

			T filter(T measurement)
			{
				_p_estimation_error_covariance += _q_process_noise_covariance;
				_k_kalman_gain =
					_p_estimation_error_covariance / (_p_estimation_error_covariance + _r_measurement_noise_covariance);
				_x_value_of_interest += _k_kalman_gain * (measurement - _x_value_of_interest);
				_p_estimation_error_covariance = (1 - _k_kalman_gain) * _p_estimation_error_covariance;
				return _x_value_of_interest;
			}

			T get()
			{
				return _x_value_of_interest;
			}
		};
	} // namespace math

	inline std::size_t aligned_offset(std::size_t align, std::size_t pos)
	{
		return ((pos / align) + 1) * align;
	}
	void* malloc_aligned(std::size_t align, std::size_t size);
	void  free_aligned(void* mem);
} // namespace streamfx::util
