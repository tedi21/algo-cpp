/* 
 * sudoku.cpp
 *
 *
 * @date 01-04-2023
 * @author Teddy DIDE
 * @version 1.00
 * Réalisation d'un algorithme de résolution de sudoku
 */

// clang-tidy sudoku_constraint.cpp -checks=cppcoreguidelines-* -- -std=c++20
// clang++-11 -std=c++20 sudoku/sudoku_constraint.cpp -o sudokuBin -Icommon

#include <iostream>
#include <chrono>
#include <array>
#include "carre.h"
#include "constraintSolver.h"

using solver_constraint_t = solver::ConstraintSolver<size_t, tda::Coord>;

int main() 
{
  //-> Problème
  const std::string grid[] = {
    "8 0 0 0 0 0 0 4 0",
    "3 0 0 8 0 0 5 6 0",
    "0 0 2 0 0 3 0 0 0",
    "5 0 0 0 0 0 0 0 4",
    "0 0 7 0 6 0 9 5 0",
    "0 0 0 9 0 0 0 0 2",
    "2 0 0 6 0 0 8 3 0",
    "0 0 0 0 0 0 0 0 9",
    "0 1 0 0 7 0 0 0 0"
  };
  //<-

  // Init
  constexpr std::size_t SquareSize = std::extent_v<decltype(grid)>;
  std::array<std::array<size_t,SquareSize>,SquareSize> values;
  for (size_t i = 0U; i <  SquareSize; ++i) 
  {
    const std::string& str = grid[i];
    for (size_t j = 0U; j <  str.size(); ++j)
    {
      const char c = str[j];
      if ('0' <= c && c <= '9') 
      {
        values[i][j/2U] = c - '0';
      }
    }
  }

  // Programmation par contrainte
  {
    solver_constraint_t algoC;

    algoC.setComparator([](const typename solver_constraint_t::variable_t& variable1,
                        const typename solver_constraint_t::variable_t& variable2)
    {
      return variable1.domainSize() < variable2.domainSize();
    } );

    algoC.setSelector([](const typename solver_constraint_t::variable_t& variable)
    {
      return *(variable.domain().begin());
    } );

    algoC.addConstraint([](solver_constraint_t& solver, const tda::Coord coord, const size_t value)
    {
      bool sastified = true;
      const size_t i = coord.X;
      const size_t j = coord.Y;
      // un seul chiffre sur une ligne
      for (size_t k = 0U; (k < SquareSize) && sastified; ++k)
      {
        if (k != j)
        {
          sastified = solver.exclude(value, {i, k});
        }
      }
      return sastified;
    });
  
    algoC.addConstraint([](solver_constraint_t& solver, const tda::Coord coord, const size_t value)
    {
      bool sastified = true;
      const size_t i = coord.X;
      const size_t j = coord.Y;
      // un seul chiffre sur une colonne
      for (size_t k = 0U; (k < SquareSize) && sastified; ++k)
      {
        if (k != i)
        {
          sastified = solver.exclude(value, {k, j});
        }
      }
      return sastified;
    });

    algoC.addConstraint([](solver_constraint_t& solver, const tda::Coord coord, const size_t value)
    {
      bool sastified = true;
      const size_t i = coord.X;
      const size_t j = coord.Y;
      // un seul chiffre danns un carré
      const size_t i1 = (i / 3U) * 3U;
      const size_t j2 = (j / 3U) * 3U;
      for (size_t k1 = i1; (k1 < i1 + 3U) && sastified; ++k1)
      {
        for (size_t k2 = j2; (k2 < j2 + 3U) && sastified; ++k2)
        {
          if ((k1 != i) || (k2 != j))
          {
              sastified = solver.exclude(value, {k1, k2});
          }
        }
      }
      return sastified;
    });

    const auto domain = tda::ValueEnum<SquareSize>;
    for (size_t i = 0U; i < SquareSize; ++i)
    {
      for (size_t j = 0U; j < SquareSize; ++j)
      {
        if (values[i][j] == 0U)
        {
          algoC.addVariable(domain.begin(), domain.end(), {i, j});
        }
        else
        {
          algoC.addVariable({values[i][j]}, {i, j});
        }
      }
    }

    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    bool result = algoC.solve();
    std::cout << "Duration=" << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count() << "ns" << std::endl;
    std::cout << "success=" << std::boolalpha << result << std::endl;
    for (size_t i = 0U; i < SquareSize; ++i)
    {
      for (size_t j = 0U; j < SquareSize; ++j)
      {
        const typename solver_constraint_t::variable_t& variable = algoC.get({i, j});
        if (variable.isInstantiated())
        {
          std::cout << variable.value();
        }
        else
        {
          std::cout << "{";
          std::for_each(variable.domain().begin(),
                        variable.domain().end(),
                        [](const size_t& value) { std::cout << value; });
          std::cout << "}";
        }
        std::cout << " | ";
      }
      std::cout << std::endl;
    }
  }

  return 0;
}
