
#ifndef CARRE_H
#define CARRE_H

#include <array>
#include <numeric>

namespace tda {
  struct Coord {
    size_t X = 0U;
    size_t Y = 0U;

    constexpr static Coord From2D1Based(size_t x, size_t y) {
      return Coord{x - 1U, y - 1U};
    }

    constexpr static Coord FromArrayIndex(size_t len, size_t i) {
      return Coord{i % len, i / len};
    }

    auto operator<=>(const Coord& rhs) const = default;
  };

  template<size_t N>
  constexpr std::array<size_t, N> ValueEnum = [] {
      std::array<size_t, N> Init;
      std::iota(Init.begin(), Init.end(), 1U);
      return Init;
    }();
}

#endif