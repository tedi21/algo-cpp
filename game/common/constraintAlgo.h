
#ifndef CONSTRAINT_ALGO_H
#define CONSTRAINT_ALGO_H

#include <concepts>
#include <type_traits>
#include <memory>
#include <set>
#include <list>
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>

namespace solver
{

  // Une variable est définie par deux types :
  // - un indice : L'indice doit être unique et ordonné (trie)
  // - un domaine : Le domaine est constitué de valeur unique et ordonnée (trie)

  // Utilisation du type std::set car le nombre de valeurs constituant le domaine est supposé inférieur à 100.
  // Utilisation du concept std::totally_ordered en adéquation avec l'utilisation std::set

  template <class T>
  concept finite_domain = std::totally_ordered<T> and (std::is_integral_v<T> or std::is_enum_v<T>);

  template <finite_domain ValueT, std::totally_ordered IndiceT>
  class Variable
  {
  public:
    using indice_t = IndiceT;
    using value_t = ValueT;
    using domain_t = std::set<ValueT>;
    using domain_const_iterator_t = typename domain_t::const_iterator;
    using domain_const_reverse_iterator_t = typename domain_t::const_reverse_iterator;

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

    domain_const_iterator_t domainBegin() const
    {
      return Domain.cbegin();
    }

    domain_const_iterator_t domainEnd() const
    {
      return Domain.cend();
    }

    domain_const_reverse_iterator_t domainRBegin() const
    {
      return Domain.crbegin();
    }

    domain_const_reverse_iterator_t domainREnd() const
    {
      return Domain.crend();
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

    value_t valueOr(const value_t no_value, const value_t no_instantiated) const
    {
      value_t val = no_instantiated;
      if (isInstantiated())
      {
        val = *(domainBegin());
      }
      else if (isCompromised())
      {
        val = no_value;
      }
      else
      {
        val = no_instantiated;
      }
      return val;
    }

    value_t value() const
    {
      return *(domainBegin());
    }

    indice_t indice() const
    {
      return Indice;
    }

  private:
    indice_t Indice;
    domain_t Domain;
  };

  template <finite_domain ValueT, std::totally_ordered IndiceT, class SolverT>
  class ConstraintBase
  {
  public:
    using indice_t = IndiceT;
    using value_t = ValueT;
    using solver_t = SolverT;

    ConstraintBase() = default;
    virtual ~ConstraintBase() = default;

    ConstraintBase(const ConstraintBase &) = delete;
    ConstraintBase(ConstraintBase &&) = delete;

    ConstraintBase &operator=(const ConstraintBase &) = delete;
    ConstraintBase &operator=(ConstraintBase &&) = delete;

    bool apply(solver_t &solver, const indice_t indice, const value_t value) const
    {
      return applyImpl(solver, indice, value);
    }

  private:
    virtual bool applyImpl(solver_t &solver, const indice_t indice, const value_t value) const = 0;
  };

  template <class FctT, finite_domain ValueT, std::totally_ordered IndiceT, class SolverT>
  class Constraint : public ConstraintBase<ValueT, IndiceT, SolverT>
  {
  public:
    Constraint(FctT &&function)
        : Function(std::forward<FctT>(function))
    {
    }

  private:
    using base_t = ConstraintBase<ValueT, IndiceT, SolverT>;
    FctT Function;

    bool applyImpl(typename base_t::solver_t &solver, const typename base_t::indice_t indice, const typename base_t::value_t value) const override
    {
      return Function(solver, indice, value);
    }
  };

  template <class VariableT>
  class IteratorBase
  {
  public:
    using variable_t = VariableT;
    using variable_ptr_t = std::shared_ptr<variable_t>;
    using variable_list_t = std::list<variable_ptr_t>;

    IteratorBase() = default;
    virtual ~IteratorBase() = default;

    IteratorBase(const IteratorBase &) = delete;
    IteratorBase(IteratorBase &&) = delete;

    IteratorBase &operator=(const IteratorBase &) = delete;
    IteratorBase &operator=(IteratorBase &&) = delete;

    void sort(variable_list_t &variables) const
    {
      sortImpl(variables);
    }

  private:
    virtual void sortImpl(variable_list_t &variables) const = 0;
  };

  template <class FctT, class VariableT>
  class Iterator : public IteratorBase<VariableT>
  {
  public:
    Iterator(FctT &&function)
        : Function(std::forward<FctT>(function))
    {
    }

  private:
    using base_t = IteratorBase<VariableT>;
    FctT Function;

    void sortImpl(typename base_t::variable_list_t &variables) const override
    {
      variables.sort([this](const typename base_t::variable_ptr_t &varA, typename base_t::variable_ptr_t &varB)
                     {
        bool is_less = false;
        if ((!varA->isCompromised())  && (!varB->isCompromised()) &&
            (!varA->isInstantiated()) && (!varB->isInstantiated()))
        {
          is_less = Function(*varA, *varB);
        }
        else if ((!varA->isCompromised()) && !(varA->isInstantiated()))
        {
          is_less = true;
        }
        return is_less; });
    }
  };

  template <class VariableT>
  class SelectorBase
  {
  public:
    using variable_t = VariableT;
    using variable_ptr_t = std::shared_ptr<variable_t>;

    SelectorBase() = default;
    virtual ~SelectorBase() = default;

    SelectorBase(const SelectorBase &) = delete;
    SelectorBase(SelectorBase &&) = delete;

    SelectorBase &operator=(const SelectorBase &) = delete;
    SelectorBase &operator=(SelectorBase &&) = delete;

    typename variable_t::value_t select(const variable_ptr_t &variable) const
    {
      return selectImpl(variable);
    }

  private:
    virtual typename variable_t::value_t selectImpl(const variable_ptr_t &variable) const = 0;
  };

  template <class FctT, class VariableT>
  class Selector : public SelectorBase<VariableT>
  {
  public:
    Selector(FctT &&function)
        : Function(std::forward<FctT>(function))
    {
    }

  private:
    using base_t = SelectorBase<VariableT>;
    FctT Function;

    typename base_t::variable_t::value_t selectImpl(const typename base_t::variable_ptr_t &variable) const override
    {
      return Function(*variable);
    }
  };

  template <finite_domain ValueT, std::totally_ordered IndiceT>
  class ConstraintAlgo
  {
  public:
    using value_t = ValueT;
    using indice_t = IndiceT;
    using variable_t = Variable<value_t, indice_t>;
    using variable_ptr_t = std::shared_ptr<variable_t>;
    using variable_list_t = std::list<variable_ptr_t>;
    using iterator_t = typename variable_list_t::const_iterator;
    using reverse_iterator_t = typename variable_list_t::const_reverse_iterator;

    static constexpr size_t INTERMEDIATE_SOLUTION = std::numeric_limits<size_t>::max();

    ConstraintAlgo()
    {
      Id = 0;
      IsSolveInProgress = false;
    }

    template <typename FctT>
    void
    setIterator(FctT &&fct)
    {
      Iterator = std::make_unique<solver::Iterator<typename std::decay<FctT>::type, variable_t>>(std::forward<FctT>(fct));
    }

    template <typename FctT>
    void
    setSelector(FctT &&fct)
    {
      Selector = std::make_unique<solver::Selector<typename std::decay<FctT>::type, variable_t>>(std::forward<FctT>(fct));
    }

    template <typename FctT>
    void
    addConstraint(FctT &&fct)
    {
      Constraints.push_back(
          std::make_unique<Constraint<typename std::decay<FctT>::type, value_t, indice_t, ConstraintAlgo>>(std::forward<FctT>(fct)));
    }

    template<class It>
    bool
    addVariable(It first, It last, const indice_t &indice)
    {
      const bool success = Variables.insert(std::make_pair(indice, std::make_shared<variable_t>(indice, std::set<value_t>{first, last}))).second;
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
      auto &&it = Variables.find(indice);
      if (it != Variables.end())
      {
        variable_ptr_t &var = it->second;
        if (!var->isInstantiated() && !var->isCompromised())
        {
          var->exclude(value);
          success = true;
        }
        else
        {
          success = !(var->canBe(value));
        }
      }
      return success;
    }

    const std::pair<variable_ptr_t, bool>
    rejectedVariable(const indice_t &indice) const
    {
      bool found = false;
      variable_ptr_t var;
      auto &&it = RejectedVariables.find(indice);
      if (it != RejectedVariables.end())
      {
        var = it->second;
        found = true;
      }
      return std::make_pair(var, found);
    }

    const std::pair<variable_t, bool>
    exists(const indice_t &indice) const
    {
      bool found = false;
      variable_t var(indice, {});
      auto &&it = Variables.find(indice);
      if (it != Variables.end())
      {
        var = *(it->second);
        found = true;
      }
      return std::make_pair(var, found);
    }

    const variable_t
    get(const indice_t &indice) const
    {
      variable_t var(indice, {});
      auto &&it = CurrentSolutionMap.find(indice);
      if (it != CurrentSolutionMap.cend())
      {
        var = *(it->second);
      }
      return var;
    }

    iterator_t
    cbegin() const
    {
      return CurrentSolutionList.cbegin();
    }

    iterator_t
    cend() const
    {
      return CurrentSolutionList.cend();
    }

    reverse_iterator_t
    crbegin() const
    {
      return CurrentSolutionList.crbegin();
    }

    reverse_iterator_t
    crend() const
    {
      return CurrentSolutionList.crend();
    }

    size_t
    size() const
    {
      return CurrentSolutionList.size();
    }

    size_t
    solutionsCount() const
    {
      return Solutions.size();
    }

    void
    setSolution(const size_t i)
    {
      // Définie les variables constituant une solution (parmi celles en cours de recherche ou déjà trouvées, sauf celle déjà recopiée)
      if ((i == INTERMEDIATE_SOLUTION) || (i != Id))
      {
        if (i == INTERMEDIATE_SOLUTION)
        {
          // Recopie des variables en cours de calcul dans l'ordre de leurs traitements
          CurrentSolutionList.resize(ProcessedIndices.size());
          std::transform(ProcessedIndices.begin(), ProcessedIndices.end(), CurrentSolutionList.begin(),
                         [this](const indice_t &indice)
                         { return Variables[indice]; });
        }
        else if (i < Solutions.size())
        {
          // Recopie des variables calculées dans une précédente solution dans l'ordre de leurs traitements
          CurrentSolutionList = Solutions[i];
        }
        else
        {
          // Pas de solution trouvée
          CurrentSolutionList.clear();
        }
        CurrentSolutionMap.clear();
        // Recopie les variables pour un accès à partir de leurs indices
        for (const variable_ptr_t &var : CurrentSolutionList)
        {
          CurrentSolutionMap.insert(std::make_pair(var->indice(), var));
        }
        // Identifie la solution recopiée
        Id = i;
      }
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
    using constraint_ptr_t = std::unique_ptr< ConstraintBase<value_t,indice_t,ConstraintAlgo> >;
    using map_variable_ptr_t = std::map<indice_t,variable_ptr_t>;
    using list_variable_ptr_t = std::list<variable_ptr_t>;
    using list_indice_t = std::list<indice_t>;
    using record_t = std::tuple<list_indice_t,list_variable_ptr_t,indice_t,value_t>;
    using record_list_t = std::list<record_t>;

    bool
    internSolve(const bool all)
    {
      // Démarrage de la recherche de solution
      IsSolveInProgress = true;

      // Initialisation des données
      bool nextSolution = true;
      list_variable_ptr_t processingVariables;
      record_list_t backup;

      // Initialisation des variables utilisées pour la recherche de solution
      processingVariables.resize(Variables.size());
      std::transform(Variables.cbegin(), Variables.cend(), processingVariables.begin(),
                     [](const typename map_variable_ptr_t::value_type& var)
                     { return var.second; });

      // Initialisation des indices des variables utilisées pour la recherche de solution
      ProcessedIndices.clear();
      ProcessedIndices.resize(Variables.size());
      std::transform(Variables.cbegin(), Variables.cend(), ProcessedIndices.begin(),
                     [](const typename map_variable_ptr_t::value_type& var)
                     { return var.first; });

      // TANT QUE (des solutions existent) FAIRE 
      //   rechercher une solution
      //   si une solution existe, mémoriser la solution est passée à la suivante
      //   sinon arrêter
      while (nextSolution)
      {
        // Recherche une solution
        const bool isSolution = SearchSolution(processingVariables, backup);
        // Mémorise la solution
        if (isSolution)
        {
          setSolution(INTERMEDIATE_SOLUTION);
          Solutions.push_back(CurrentSolutionList);
          Id = Solutions.size() - 1U;
        }
        // Recherche les autres solutions
        nextSolution = all && !backup.empty() && isSolution;
      } 
      IsSolveInProgress = false;
      return !Solutions.empty();
    }

    bool 
    SearchSolution(list_variable_ptr_t& processingVariables, record_list_t& backup)
    {
      bool isSolution = false;
      bool sastified = true;
      // TANT QUE (la solution n'est pas trouvé et le problème est toujours résoluble) FAIRE 
      //   réduire le nombre de variable constituant le problème et vérifier la cohérence du problème
      //   si une solution partielle est trouvée, déterminer un point de choix
      //   sinon si une solution est trouvée, arrêter
      //   sinon si un point de choix existe, revenir en arrière (la résolution du problème est incohérente)
      //   sinon arrêter (le problème est insoluble)
      while (!isSolution && sastified)
      {
        bool isPartialSolution = true;
        if (!processingVariables.empty())
        {
          // Réduit le problème en appliquant les contraintes
          std::tie(sastified, isPartialSolution) = partitionAndConstrain(processingVariables);

          // Fait un choix
          if (isPartialSolution)
          {
            // Mémorise le contexte et le choix pour le backtracking
            backup.emplace_back(selectVariableAndValue(processingVariables));
          }
        }
        else 
        {
          sastified = false;
        }
        isSolution = sastified && !isPartialSolution;

        // Mauvais choix ou recherche d'une autre solution, réinitialise les variables
        if ((!sastified) && (!backup.empty()))
        {
          processingVariables = backtracking(backup);
          sastified = true;
        }
      }
      return isSolution;
    }

    std::tuple<bool,bool>
    partitionAndConstrain(list_variable_ptr_t& processingVariables)
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
          processingVariables.push_back(Variables[newIndice]);
        }
        ProcessedIndices.splice(ProcessedIndices.end(), AdditionalIndices);
        // Partition des variables
        std::list<variable_ptr_t> instantiatedVars;
        std::list<variable_ptr_t> noInstantiatedVars;
        std::partition_copy(processingVariables.begin(), processingVariables.end(),
                            std::back_inserter(instantiatedVars),
                            std::back_inserter(noInstantiatedVars),
                            [](const variable_ptr_t &var)
                            { return var->isInstantiated(); });
        hasInstantiatedVars = (!instantiatedVars.empty());
        hasNoInstantiatedVars = (!noInstantiatedVars.empty());
        // Applique les contraintes
        for (const variable_ptr_t &var : instantiatedVars)
        {
          for (const constraint_ptr_t &constraint : Constraints)
          {
            sastified = constraint->apply(*this, var->indice(), var->value());
            if (!sastified)
              break;
          }
          if (!sastified)
            break;
        }
        hasAdditionnalVars = (!AdditionalIndices.empty());
        // Recommencer tant qu'il y a des nouvelles variables instanciées parmi celles non instanciées
        processingVariables = noInstantiatedVars;
      }
      return std::make_tuple(sastified, sastified && hasNoInstantiatedVars);
    }

    record_t
    selectVariableAndValue(list_variable_ptr_t& processingVariables) const
    {
      // Trie des variables et sélection de la première variable
      if (Iterator != nullptr)
      {
        Iterator->sort(processingVariables);
      }
      const variable_ptr_t var = processingVariables.front();
      // Sélectionne la valeur
      value_t val = var->value();
      if (Selector != nullptr)
      {
        val = Selector->select(var);
      }
      // Copie du contexte
      list_indice_t copyProcessedIndices;
      std::copy(ProcessedIndices.begin(), ProcessedIndices.end(), std::back_inserter(copyProcessedIndices));
      variable_list_t copyProcessingVariables;
      copyProcessingVariables.resize(processingVariables.size());
      std::transform(processingVariables.begin(), processingVariables.end(), copyProcessingVariables.begin(),
                      [](const variable_ptr_t &v)
                      { return std::make_shared<variable_t>(*v); });
      const indice_t copyIndice = var->indice();
      const value_t copyVal = val;
      // Définit le choix de la valeur
      var->set(val);
      //std::cout << "try (" << copyIndice.X << ", " << copyIndice.Y << ") = " << copyVal << std::endl;
      return std::make_tuple(copyProcessedIndices, copyProcessingVariables, copyIndice, copyVal);
    }

    list_variable_ptr_t 
    backtracking(record_list_t& backup)
    {
      list_indice_t copyProcessedIndices = std::move(ProcessedIndices);
      typename list_indice_t::iterator it = copyProcessedIndices.begin();     
      // Restaure les variables
      const auto [tmpProcessedIndices, processingVariables, indice, val] = backup.back();
      ProcessedIndices = std::move(tmpProcessedIndices);
      backup.pop_back();
      for (const variable_ptr_t &var : processingVariables)
      {
        Variables[var->indice()] = var;
      }
      // Copier les variables non viables
      std::advance(it, ProcessedIndices.size());
      for (; it != copyProcessedIndices.end(); ++it)
      {
        RejectedVariables.insert(std::make_pair(*it, Variables[*it]));
        Variables.erase(*it);
      }
      for (it = AdditionalIndices.begin(); it != AdditionalIndices.end(); ++it)
      {
        RejectedVariables.insert(std::make_pair(*it, Variables[*it]));
        Variables.erase(*it);
      }
      AdditionalIndices.clear();
      // Elimine la valeur déjà analysée de la variable
      Variables[indice]->exclude(val);
      //std::cout << "backtracking (" << indice.X << ", " << indice.Y << ") = " << val << std::endl;
      return processingVariables;
    }

    bool IsSolveInProgress;
    std::unique_ptr<IteratorBase<variable_t>> Iterator;
    std::unique_ptr<SelectorBase<variable_t>> Selector;
    std::list<constraint_ptr_t> Constraints;
    map_variable_ptr_t Variables;
    list_indice_t AdditionalIndices;
    map_variable_ptr_t RejectedVariables;
    list_indice_t ProcessedIndices;
    map_variable_ptr_t CurrentSolutionMap;
    list_variable_ptr_t CurrentSolutionList;
    size_t Id;
    std::vector<list_variable_ptr_t> Solutions;
  };
}

#endif
