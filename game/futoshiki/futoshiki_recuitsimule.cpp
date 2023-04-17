/* 
 * futoshiki.cpp
 *
 *
 * @date 01-10-2022
 * @author Teddy DIDE
 * @version 1.00
 * Réalisation d'un algorithme de résolution de futoshiki
 */

// clang-tidy futoshiki_recuitsimule.cpp -checks=cppcoreguidelines-* -- -std=c++20
// clang++-11 -std=c++20 futoshiki/futoshiki_recuitsimule.cpp -o futoshikiBin -Icommon -Ifutoshiki

#include <iostream>
#include <chrono>
#include "futoshiki.h"
#include "recuitSimule.h"

int main() {
  constexpr size_t SizeOfSquare = 4U;
  tda::Futoshiki<SizeOfSquare> f {
    {
      tda::InferiorConstraint{tda::Coord::From2D1Based(1U, 3U), tda::Direction::Left},
      tda::InferiorConstraint{tda::Coord::From2D1Based(2U, 1U), tda::Direction::Right},
      tda::InferiorConstraint{tda::Coord::From2D1Based(3U, 4U), tda::Direction::Down},
    },
    {
      tda::Assertion{tda::Coord::From2D1Based(4U, 1U), 1U},
      tda::Assertion{tda::Coord::From2D1Based(2U, 3U), 3U},
    }
  };

  solver::RecuitSimule<decltype(f), size_t, tda::Coord> algoRS(f);
  
  std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
  auto result = algoRS.start();
  std::cout << "Duration=" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms" << std::endl;
  std::cout << "Violations=" << result.getViolationsCount() << std::endl;
  std::cout << result;


  return 0;
}
