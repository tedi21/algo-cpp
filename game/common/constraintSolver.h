
#ifndef CONSTRAINT_ALGO_H
#define CONSTRAINT_ALGO_H

#include <concepts>
#include <type_traits>
#include <functional>
#include <set>
#include <list>
#include <map>
#include <vector>
#include <algorithm>

namespace solver
{

  // Une variable est définie par deux types :
  // - un indice : L'indice doit être unique et ordonné (trie)
  // - un domaine : Le domaine est constitué de valeur unique et ordonnée (trie)

  // Utilisation du type std::set car le nombre de valeurs constituant le domaine est supposé inférieur à 100.
  // Utilisation du concept std::totally_ordered en adéquation avec l'utilisation std::set

  template <class T>
  concept ordered_value = std::totally_ordered<T> and (std::is_integral_v<T> or std::is_enum_v<T>);

  template <ordered_value ValueT, std::totally_ordered IndiceT>
  class Variable
  {
  public:
    using indice_t = IndiceT;
    using value_t = ValueT;
    using domain_t = std::set<ValueT>;

    Variable(const indice_t indice, const domain_t &domain)
        : Indice(indice), Domain(domain)
    {
    }

    bool isInstantiated() const
    {
      return (Domain.size() == 1U);
    }

    bool isCompromised() const
    {
      return (Domain.size() == 0U);
    }

    size_t domainSize() const
    {
      return Domain.size();
    }

    const domain_t& domain() const
    {
      return Domain;
    }

    void exclude(const value_t value)
    {
      Domain.erase(value);
    }

    void set(const value_t value)
    {
      Domain.clear();
      Domain.insert(value);
    }

    bool canBe(const value_t value) const
    {
      return Domain.find(value) != Domain.cend();
    }

    value_t value() const
    {
      if (Domain.empty()) 
      {
        throw std::out_of_range("Variable is compromised.");
      }
      return *(Domain.begin());
    }

    indice_t indice() const
    {
      return Indice;
    }

  private:
    indice_t Indice;
    domain_t Domain;
  };

  template <ordered_value ValueT, std::totally_ordered IndiceT>
  class ConstraintSolver
  {
  public:
    using value_t = ValueT;
    using indice_t = IndiceT;
    using solver_t = ConstraintSolver<ValueT, IndiceT>;
    using variable_t = Variable<value_t, indice_t>;
    using domain_t = variable_t::domain_t;
    using solution_t = std::vector<variable_t>;
    using solution_list_t = std::vector<solution_t>;

    ConstraintSolver()
    {
      IsSolveInProgress = false;
    }

    template <typename FctT>
    void
    setComparator(FctT &&fct)
    {
      Comparator = std::forward<FctT>(fct);
    }

    template <typename FctT>
    void
    setSelector(FctT &&fct)
    {
      Selector = std::forward<FctT>(fct);
    }

    template <typename FctT>
    void
    addConstraint(FctT &&fct)
    {
      Constraints.push_back(
          std::forward<FctT>(fct));
    }

    template<class It>
    bool
    addVariable(It first, It last, const indice_t &indice)
    {
      const bool success = Variables.insert(std::make_pair(indice, variable_t{indice, std::set<value_t>{first, last}})).second;
      if (success && IsSolveInProgress)
      {
        AdditionalIndices.push_back(indice);
      }
      return success;
    }

    bool
    addVariable(const std::initializer_list<value_t> domain, const indice_t &indice)
    {
      return addVariable(domain.begin(), domain.end(), indice);
    }

    bool
    exclude(const value_t value, const indice_t &indice)
    {
      bool success = false;
      if (auto &&it = Variables.find(indice); it != Variables.end())
      {
        variable_t &var = it->second;
        if (!var.isInstantiated() && !var.isCompromised())
        {
          var.exclude(value);
          success = true;
        }
        else
        {
          success = !(var.canBe(value));
        }
      }
      return success;
    }

    bool
    hasBeenRejected(const indice_t &indice) const
    {
      auto &&it = RejectedIndices.find(indice);
      return (it != RejectedIndices.end());
    }

    bool
    exists(const indice_t &indice) const
    {
      auto &&it = Variables.find(indice);
      return (it != Variables.end());
    }

    variable_t
    get(const indice_t &indice) const
    {
      variable_t var(indice, {});
      if (auto &&it = Variables.find(indice); it != Variables.cend())
      {
        var = it->second;
      }
      return var;
    }

    size_t solutionsSize() const
    {
      return SolutionList.size();
    }

    const solution_list_t& solutions() const
    {
      return SolutionList;
    }

    bool solve()
    {
      return internSolve(false);
    }

    void solveAll()
    {
      internSolve(true);
    }

  private:
    using constraint_t = std::function<bool(solver_t&, const indice_t, const value_t)>;
    using comparator_t = std::function<bool(const variable_t&, const variable_t&)>;
    using selector_t = std::function<value_t(const variable_t&)>;
    using map_variable_t = std::map<indice_t,variable_t>;
    using list_cref_variable_t = std::vector<std::reference_wrapper<variable_t>>;
    using vector_variable_t = std::vector<variable_t>;
    using list_indice_t = std::vector<indice_t>;
    using set_indice_t = std::set<indice_t>;
    struct record_t
    {
      list_indice_t Register;
      vector_variable_t RemainingVariables;
      indice_t ChoiceIndice;
      value_t ChoiceValue;
    };
    using record_list_t = std::list<record_t>;
    struct parameters_t
    {
      list_cref_variable_t Processing;
      list_indice_t Register;
      record_list_t Backup;
    };

    bool
    internSolve(const bool all)
    {
      // Démarrage de la recherche de solution
      IsSolveInProgress = true;

      // Initialisation des données
      bool nextSolution = true;
      parameters_t parameters;

      // Initialisation des variables utilisées pour la recherche de solution
      parameters.Processing.reserve(Variables.size());
      parameters.Register.reserve(Variables.size());
      for(auto &&var: Variables)
      {
        parameters.Processing.push_back(var.second); // Variables en cours de traitement
        parameters.Register.push_back(var.first);// Indice des variables dans l'ordre de traitement
      }

      // TANT QUE (des solutions existent) FAIRE 
      //   rechercher une solution
      //   si une solution existe, mémoriser la solution est passée à la suivante
      //   sinon arrêter
      while (nextSolution)
      {
        // Recherche une solution
        const bool isSolution = SearchSolution(parameters);
        // Mémorise la solution
        if (isSolution)
        {
          // Recopie des variables en cours de calcul dans l'ordre de leurs traitements
          solution_t solution;
          solution.reserve(parameters.Register.size());
          for(auto &&indice : parameters.Register)
          {
            if(auto &&it = Variables.find(indice); it != Variables.cend())
            {
              solution.push_back(it->second);
            }
          }
          SolutionList.emplace_back(std::move(solution));
        }
        // Recherche les autres solutions
        nextSolution = all && !parameters.Backup.empty() && isSolution;
      } 
      IsSolveInProgress = false;
      return !SolutionList.empty();
    }

    bool 
    SearchSolution(parameters_t& parameters)
    {
      bool isSolution = false;
      bool sastified = true;
      // TANT QUE (la solution n'est pas trouvé et que toutes les contraintes sont sastifaites) FAIRE 
      //   réduire le nombre de variable constituant le problème et vérifier la cohérence du problème
      //   si une solution partielle est trouvée, déterminer un point de choix
      //   sinon si une solution est trouvée, arrêter
      //   sinon si un point de choix existe, revenir en arrière (les contraintes du problèmes ne sont pas sastifaites)
      //   sinon arrêter (les contraintes du problèmes ne sont pas sastifaites)
      while (!isSolution && sastified)
      {
        bool isPartialSolution = true;
        if (!parameters.Processing.empty())
        {
          // Réduit le problème en appliquant les contraintes
          std::tie(sastified, isPartialSolution) = partitionAndConstrain(parameters);

          // Fait un choix
          if (isPartialSolution)
          {
            // Mémorise le contexte et le choix pour le backtracking
            selectVariableAndValue(parameters);
          }
        }
        else 
        {
          // Aucune variable 
          sastified = false;
        }
        isSolution = sastified && !isPartialSolution;

        // Mauvais choix ou recherche d'une autre solution, réinitialise les variables
        if ((!sastified) && (!parameters.Backup.empty()))
        {
          backtracking(parameters);
          sastified = true;
        }
      }
      return isSolution;
    }

    std::tuple<bool,bool>
    partitionAndConstrain(parameters_t& parameters)
    {
      // TANT QUE (Existe nouvelles variables instanciées) FAIRE 
      //   partitionner toutes les nouvelles variables qui sont instanciées parmi les variables du problème
      //   POUR CHAQUE (Nouvelles variables instanciées)
      //     POUR CHAQUE (Contraintes)
      //       Appliquer les contraintes pour réduire le domaine des variables constituant le problème
      bool sastified = true;
      bool hasNoInstantiatedVars = true;
      bool hasInstantiatedVars = true;
      bool hasAdditionnalVars = false;
      while (sastified && ((hasInstantiatedVars && hasNoInstantiatedVars) || hasAdditionnalVars))
      {
        // Ajout des nouvelles variables créées pendant la recherche
        for (const indice_t &newIndice : AdditionalIndices)
        {
          if (auto &&it = Variables.find(newIndice); it != Variables.cend())
          {
            parameters.Processing.push_back(it->second);
          }
        }
        parameters.Register.insert(parameters.Register.end(), AdditionalIndices.begin(), AdditionalIndices.end());
        // Partition des variables
        list_cref_variable_t instantiatedVars;
        list_cref_variable_t noInstantiatedVars;
        std::partition_copy(parameters.Processing.begin(), parameters.Processing.end(),
                            std::back_inserter(instantiatedVars),
                            std::back_inserter(noInstantiatedVars),
                            [](const variable_t &var)
                            { return var.isInstantiated(); });
        hasInstantiatedVars = (!instantiatedVars.empty());
        hasNoInstantiatedVars = (!noInstantiatedVars.empty());
        // Applique les contraintes
        for (const variable_t &var : instantiatedVars)
        {
          for (const constraint_t &constraint : Constraints)
          {
            sastified = constraint(*this, var.indice(), var.value());
            if (!sastified)
              break;
          }
          if (!sastified)
            break;
        }
        hasAdditionnalVars = (!AdditionalIndices.empty());
        // Recommencer tant qu'il y a des nouvelles variables instanciées parmi celles non instanciées
        parameters.Processing = noInstantiatedVars;
      }
      return std::make_tuple(sastified, sastified && hasNoInstantiatedVars);
    }

    void
    selectVariableAndValue(parameters_t& parameters) const
    {
      // Trie des variables et sélection de la première variable
      if (Comparator)
      {
        std::sort(parameters.Processing.begin(), parameters.Processing.end(), Comparator);
      }
      variable_t& var = parameters.Processing.front();
      // Sélectionne la valeur
      value_t val = var.value();
      if (Selector)
      {
        val = Selector(var);
      }
      // Copie du contexte
      record_t save;
      save.Register = parameters.Register;
      save.RemainingVariables.reserve(parameters.Processing.size());
      save.RemainingVariables.assign(parameters.Processing.begin(), parameters.Processing.end());
      save.ChoiceIndice = var.indice();
      save.ChoiceValue = val;
      // Définit le choix de la valeur
      var.set(val);
      //std::cout << "try (" << copyIndice.X << ", " << copyIndice.Y << ") = " << copyVal << std::endl;
      parameters.Backup.emplace_back(std::move(save));
    }

    void 
    backtracking(parameters_t& parameters)
    {
      record_t save = parameters.Backup.back();
      parameters.Backup.pop_back();
      // Copier les variables non viables
      auto &&it = parameters.Register.begin();    
      std::advance(it, save.Register.size());
      for (; it != parameters.Register.end(); ++it)
      {
        RejectedIndices.insert(*it);
        Variables.erase(*it);
      }
      for (it = AdditionalIndices.begin(); it != AdditionalIndices.end(); ++it)
      {
        RejectedIndices.insert(*it);
        Variables.erase(*it);
      }
      // Restaure les variables
      parameters.Processing.clear();
      parameters.Processing.reserve(save.RemainingVariables.size());
      for (auto&& var : save.RemainingVariables)
      {
        if (auto &&it = Variables.find(var.indice()); it != Variables.end())
        {
          it->second = var;
          parameters.Processing.push_back(it->second);
        }
      }
      parameters.Register = std::move(save.Register);
      AdditionalIndices.clear();
      // Elimine la valeur déjà analysée de la variable
      if (auto &&it = Variables.find(save.ChoiceIndice); it != Variables.end())
      {
        it->second.exclude(save.ChoiceValue);
      }
      //std::cout << "backtracking (" << indice.X << ", " << indice.Y << ") = " << val << std::endl;
    }

    bool IsSolveInProgress;
    comparator_t Comparator;
    selector_t Selector;
    std::list<constraint_t> Constraints;
    map_variable_t Variables;
    list_indice_t AdditionalIndices;
    set_indice_t RejectedIndices;
    solution_list_t SolutionList;
  };
}

#endif
