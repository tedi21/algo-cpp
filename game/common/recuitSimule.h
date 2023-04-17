
#ifndef RECUIT_SIMULE_H
#define RECUIT_SIMULE_H

#include <random>
#include <cmath>

namespace solver
{
  template<class Problem, class Value, class Index>
  class RecuitSimule {
  public:
    double INITIAL_PROBABILITY = 0.9975;
    double N1 = 2000;
    double N2 = 50;
    double GAMMA = 0.99;
    size_t SamplesCount = 100U;

    explicit RecuitSimule(const Problem& p) : Best(p) {
      computeInitialTemperature();
    }

    Problem start() {
      std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);
      Problem current = Best;
      size_t currentViolations = current.getViolationsCount();
      size_t bestViolations = currentViolations;
      for (int k = 0; k < N1; k++) {
        for (int l = 0; l < N2; l++) {
          const Problem next = neighbour(current);
          const size_t nextViolations = next.getViolationsCount();
          const int delta = nextViolations - currentViolations;
          if (delta <= 0) {
            current = next;
            currentViolations = nextViolations;
            if (currentViolations < bestViolations) {
              Best = current;
              bestViolations = currentViolations;
              if (bestViolations == 0U) {
                  return Best;
              }
            }
          } else {
            double v = uniform_dist(Generator);
            if (v <= std::exp(-(static_cast<double>(delta)) / Temperature)) {
              current = next;
              currentViolations = nextViolations;
            }
          }
        }
        Temperature = Temperature > 0.0 ? Temperature * GAMMA : Temperature;
      }
      return Best;
    }

  private:
    std::random_device RandomSeed;
    std::default_random_engine Generator{RandomSeed()};
    Problem Best;
    double Temperature;

    Index getRandomIndex(const Problem& p) {
      const std::vector<Index> selection = p.getIndexSelection();
      std::uniform_int_distribution<> uniform_dist(0U, selection.size() - 1U);
      return selection[uniform_dist(Generator)];
    }

    Problem neighbour(const Problem& p) {
      const Index a = getRandomIndex(p);
      Index b = getRandomIndex(p);
      while (a == b) {
        b = getRandomIndex(p);
      }
      const Value tmp = p.getValue(a); 
      Problem copy = p;
      copy.setValue({a, p.getValue(b)});
      copy.setValue({b, tmp});
      return copy;
    }

    void computeInitialTemperature() {
      double averageDelta = 0.0;
      for (size_t i = 0U; i < SamplesCount; ++i) {
          averageDelta += static_cast<double>(std::abs(static_cast<int>(neighbour(Best).getViolationsCount()) - static_cast<int>(Best.getViolationsCount())));
      }
      averageDelta = averageDelta / static_cast<double>(SamplesCount);
      Temperature = -averageDelta / std::log(INITIAL_PROBABILITY);
      std::cout << "Temperature " << Temperature << std::endl;
    }
  };
}

#endif // RECUIT_SIMULE_H
