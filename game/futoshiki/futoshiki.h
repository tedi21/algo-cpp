
#ifndef FUTOSHIKI_H
#define FUTOSHIKI_H

#include <algorithm>
#include <optional>
#include <vector>
#include "carre.h"

namespace tda {
  enum class Direction {
    Up,
    Right,
    Left,
    Down
  };

  class InferiorConstraint {
  public:
    InferiorConstraint(Coord inf, Direction side)
    : CoordInf(inf), Side(side)
    {
      switch (Side)
      {
      case tda::Direction::Down:
        CoordSup = {(CoordInf.X + 1U), CoordInf.Y};
        break;
      case tda::Direction::Up:
        CoordSup = {(CoordInf.X - 1U), CoordInf.Y};
        break;
      case tda::Direction::Left:
        CoordSup = {CoordInf.X, (CoordInf.Y - 1U)};
        break;
      case tda::Direction::Right:
        CoordSup = {CoordInf.X, (CoordInf.Y + 1U)};
        break;
      default:
        break;
      }
    }
    const Coord& Inf() const 
    {
      return CoordInf;
    }
    const Coord& Sup() const 
    {
      return CoordSup;
    }
  private:
    Coord CoordInf;
    Coord CoordSup;
    Direction Side = Direction::Up;
  };

  struct Assertion {
    Coord Pos;
    size_t Value;
  };

  template<size_t N>
  struct PotentialValues {
    std::vector<size_t> Values{ValueEnum<N>.begin(), ValueEnum<N>.end()};

    std::optional<size_t> Selected;

    bool isSelected() const noexcept {
      return Selected.has_value();
    }

    bool isSet() const noexcept {
      return Values.size() == 1U;
    }

    size_t getValue() const noexcept {
      size_t v = 0U;
      if (isSet()) {
        v = Values[0];
      }
      else if (isSelected()) {
        v = Selected.value();
      }
      return v;
    }
  };

  template<size_t N>
  std::ostream& operator<<(std::ostream& os, const PotentialValues<N>& pv) {
    if (!pv.isSelected() && !pv.isSet()) {
      os << '(';
      for (bool first = true; auto v: pv.Values) {
        if (!first) {
          os << ' ';
        }
        os << v;
        first = false;
      }
      os << ')';
    }
    else {
      os << pv.getValue();
    }
    return os;
  }

  template<size_t N>
  class Futoshiki {
  public:
    Futoshiki(const std::vector<InferiorConstraint>& constaints,
              const std::vector<Assertion>& inits) 
    : Constraints(constaints) {

      std::array<size_t,N> range;
      std::iota(range.begin(), range.end(), 1U);
      std::array<std::array<size_t,N>,N> values;
      values.fill(range);

      for(auto i: inits) {
        Grid[i.Pos.X][i.Pos.Y].Values = {i.Value};
        Grid[i.Pos.X][i.Pos.Y].Selected = i.Value;
        values[i.Pos.X][i.Value - 1U] = 0U;
      }

      //  first solution
      for(size_t i = 0U; i < N; ++i) {
        size_t k = 0U;
        std::remove(values[i].begin(), values[i].end(), 0U);
        for(size_t j = 0U; j < N; ++j) {
          if (!Grid[i][j].isSet()) {
            Grid[i][j].Selected = values[i][k++];
            Index.push_back({i, j});
          }
        }
      }
    }

    void setValue(const Assertion assert) {
      Grid[assert.Pos.X][assert.Pos.Y].Selected = assert.Value;
    }

    const std::vector<size_t>& getValueSelection(const Coord c) const {
      return Grid[c.X][c.Y].Values;
    }

    const std::vector<Coord>& getIndexSelection() const {
      return Index;
    }

    size_t getValue(const Coord c) const {
      return *(Grid[c.X][c.Y].Selected);
    }

    size_t getViolationsCount() const {
      size_t violations = 0U;
      std::array<std::array<bool, N>, N> lines;
      std::array<bool, N> occurences;
      occurences.fill(false);
      lines.fill(occurences);
      std::array<std::array<bool, N>, N> columns(lines);
      
      for(size_t i = 0U; i < N; ++i) {
        for(size_t j = 0U; j < N; ++j) {
          const size_t value = *(Grid[i][j].Selected) - 1U;
          if (lines[i][value]) {
            ++violations;
          }
          else {
            lines[i][value] = true;
          }
          if (columns[j][value]) {
            ++violations;
          }
          else {
            columns[j][value] = true;
          }
        }  
      }
      for (const InferiorConstraint& c : Constraints) {
        if (Grid[c.Inf().X][c.Inf().Y].Selected >= Grid[c.Sup().X][c.Sup().Y].Selected) {
          ++violations;
        }
      }
      return violations;
    }

    constexpr static size_t GetSize() noexcept {
      return N * N;
    }

  private:
    std::vector<Coord> Index;
    std::vector<InferiorConstraint> Constraints;
    std::array<std::array<PotentialValues<N>, N>, N> Grid;

    template<size_t X>
    friend std::ostream& operator<<(std::ostream& os, const Futoshiki<X>& f);
  };

  template<size_t N>
  std::ostream& operator<<(std::ostream& os, const Futoshiki<N>& f) {
    for(size_t i = 0U; i < N; ++i) {
      for(size_t j = 0U; j < N; ++j) {
        os << f.Grid[i][j] << ' '; 
      }
      os << '\n'; 
    }
    return os;
  }
}

#endif