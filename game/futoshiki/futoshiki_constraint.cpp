/* 
 * futoshiki.cpp
 *
 *
 * @date 01-10-2022
 * @author Teddy DIDE
 * @version 1.00
 * Réalisation d'un algorithme de résolution de futoshiki
 */

// clang-tidy futoshiki_constraint.cpp -checks=cppcoreguidelines-* -- -std=c++20
// clang++-11 -std=c++20 futoshiki/futoshiki_constraint.cpp -o futoshikiBin -Icommon -Ifutoshiki

#include <iostream>
#include <chrono>
#include <array>
#include "futoshiki.h"
#include "constraintSolver.h"
#include "dependencyGraph.h"

using solver_constraint_t = solver::ConstraintSolver<size_t, tda::Coord>;

template<class T>
size_t max_bound(solver_constraint_t& solver, const tda::Coord& c, T first, T last)
{
  std::vector<std::size_t> column;
  std::vector<std::size_t> line;
  for (;first != last; ++first) 
  {
    const tda::Coord& sup = first->second.Sup();
    const size_t val = (*solver.get(sup).domain().rbegin());
    if (c.X == sup.X)
    {
      line.push_back(val);
    }
    else
    {
      column.push_back(val);
    }
  }
  const auto [minEL, maxEL] = std::minmax_element(line.begin(), line.end());
  const auto [minEC, maxEC] = std::minmax_element(column.begin(), column.end());
  if (!line.empty() && !column.empty())
  {
    return std::min({(*minEL) - 1U, (*maxEL) - line.size(), (*minEC) - 1U, (*maxEC) - column.size()});
  }
  else if (!line.empty())
  {
    return std::min((*minEL) - 1U, (*maxEL) - line.size());
  }
  else if (!column.empty())
  {
    return std::min((*minEC) - 1U, (*maxEC) - column.size());
  }
  else
  {
    return 0U;
  }
}

template<class T>
size_t min_bound(solver_constraint_t& solver, const tda::Coord& c, T first, T last)
{
  std::vector<std::size_t> column;
  std::vector<std::size_t> line;
  for (;first != last; ++first) 
  {
    const tda::Coord& inf = first->second.Inf();
    const size_t val = (*solver.get(inf).domain().begin());
    if (c.X == inf.X)
    {
      line.push_back(val);
    }
    else
    {
      column.push_back(val);
    }
  }
  const auto [minEL, maxEL] = std::minmax_element(line.begin(), line.end());
  const auto [minEC, maxEC] = std::minmax_element(column.begin(), column.end());
  if (!line.empty() && !column.empty())
  {
    return std::max({(*maxEL) + 1U, (*minEL) + line.size(), (*maxEC) + 1U, (*minEC) + column.size()});
  }
  else if (!line.empty())
  {
    return std::max((*maxEL) + 1U, (*minEL) + line.size());
  }
  else if (!column.empty())
  {
    return std::max((*maxEC) + 1U, (*minEC) + column.size());
  }
  else
  {
    return 0U;
  }
}

bool inequal(solver_constraint_t& solver, const std::vector<tda::Coord>& constraintsOrder, 
  const std::multimap<tda::Coord,tda::InferiorConstraint>& constraintsInfMap,
  const std::multimap<tda::Coord,tda::InferiorConstraint>& constraintsSupMap)
{
  bool sastified = true;
  for (auto it = constraintsOrder.begin(); it != constraintsOrder.end(); ++it)
  {
    const typename solver_constraint_t::variable_t var = solver.get(*it);
    if (!var.isInstantiated() && !var.isCompromised())
    {
      auto&& range = constraintsInfMap.equal_range(*it);
      if (range.first != range.second)
      {
        const size_t newmax = max_bound(solver, *it, range.first, range.second);
        const size_t oldmax = (*var.domain().rbegin());
        for (size_t k = newmax + 1U; (k <= oldmax) && sastified; ++k)
        {
          sastified = solver.exclude(k, *it);
        }
        if (!sastified) 
          break;
      }
    }
  }
  if (sastified)
  {
    for (auto rev = constraintsOrder.rbegin(); rev != constraintsOrder.rend(); ++rev)
    {
      const typename solver_constraint_t::variable_t var = solver.get(*rev);
      if (!var.isInstantiated() && !var.isCompromised())
      {
        auto&& range = constraintsSupMap.equal_range(*rev);
        if (range.first != range.second)
        {
          const size_t newmin = min_bound(solver, *rev, range.first, range.second);
          const size_t oldmin = (*var.domain().begin());
          for (size_t k = oldmin; (k < newmin) && sastified; ++k)
          {
            sastified = solver.exclude(k, *rev);
          }
          if (!sastified) 
            break;
        }
      }
    }
  }
  return sastified;
}

int main() 
{
  //-> Problème
  const std::string grid[] = {
    "0 0<0 0 0 4 5 7 3",
    "                 ",
    "1 0 0 0>0 6<0<0 7",
    "      ^         v",
    "0>0 0<4>0 7<0 0<0",
    "                 ",
    "0 0 0<0 0 0 0 0 0",
    "^         v      ",
    "0<0 5>0 0 0 0 0 0",
    "v       v        ",
    "0 0 0 0 0<0 0 0 0",
    "        v        ",
    "0 0 0 0 0 3<0 0 0",
    "      v v        ",
    "0 0 0<7 0 9 0<5 2",
    "        v     v  ",
    "0 0 0 0 0<0 1 0 0"
  };
  //<-

  // Init
  constexpr std::size_t GridSize = std::extent_v<decltype(grid)>;
  constexpr size_t SquareSize = (GridSize / 2U) + 1U;
  std::array<std::array<size_t,SquareSize>,SquareSize> values;
  std::vector<tda::InferiorConstraint> constraints;
  for (size_t i = 0U; i <  GridSize; ++i) 
  {
    const std::string& str = grid[i];
    for (size_t j = 0U; j <  str.size(); ++j)
    {
      const char c = str[j];
      if ('0' <= c && c <= '9') 
      {
        values[i/2U][j/2U] = c - '0';
      }
      else 
      {
        switch (c)
        {
        case '<':
          constraints.push_back(tda::InferiorConstraint({i/2U, j/2U}, tda::Direction::Right));
          break;
        case '>':
          constraints.push_back(tda::InferiorConstraint({i/2U, (j/2U)+1U}, tda::Direction::Left));
          break;
        case '^':
          constraints.push_back(tda::InferiorConstraint({i/2U, j/2U}, tda::Direction::Down));
          break;
        case 'v':
          constraints.push_back(tda::InferiorConstraint({(i/2U)+1U, j/2U}, tda::Direction::Up));
          break;
        default:
          break;
        }
      }
    }
  }

  std::multimap<tda::Coord,tda::InferiorConstraint> constraintsInfMap;
  std::multimap<tda::Coord,tda::InferiorConstraint> constraintsSupMap;
  for (const tda::InferiorConstraint& constraint : constraints) 
  {
    constraintsInfMap.insert(std::make_pair(constraint.Inf(), constraint));
    constraintsSupMap.insert(std::make_pair(constraint.Sup(), constraint));
  }
  DependencyGraph<tda::Coord> depends;
  for (const tda::InferiorConstraint& constraint : constraints) 
  {
    depends.addDependency(constraint.Inf(), constraint.Sup());
  }
  std::vector<tda::Coord> constraintsOrder = depends.topologicalSort();

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

    algoC.addConstraint([&constraintsOrder, &constraintsInfMap, &constraintsSupMap](solver_constraint_t& solver, const tda::Coord coord, const size_t value)
    {
      return inequal(solver, constraintsOrder, constraintsInfMap, constraintsSupMap);
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
    bool result = inequal(algoC, constraintsOrder, constraintsInfMap, constraintsSupMap);

    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    result = result && algoC.solve();
    std::cout << "Duration=" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms" << std::endl;
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
