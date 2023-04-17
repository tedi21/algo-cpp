
#ifndef DEPENDENCY_GRAPH_H
#define DEPENDENCY_GRAPH_H

#include <stack>
#include <map>
#include <vector>
#include <set>
#include <concepts>
 
template<std::totally_ordered T>
class DependencyGraph {
private:
  std::map<T, std::vector<T> > graph; 
  std::set<T> vertices;
public:
  DependencyGraph()
  {}

  void addDependency(const T& u, const T& v) 
  { 
    vertices.insert(u);
    vertices.insert(v);
    graph[u].push_back(v); 
  }
 
  std::vector<T> topologicalSort()
  {
    std::set<T> visited; 
    std::vector<T> sorted;
    sorted.reserve(vertices.size());

    for (auto&& vertice: vertices)
    {
      if (!visited.contains(vertice))
      {
        parseDependencies(vertice, visited, sorted);
      }
    }
    return sorted;
  }
 
private:
  void parseDependencies(const T& vertice, std::set<T>& visited, std::vector<T>& stack)
  {
    visited.insert(vertice);
    for (auto&& depends: graph[vertice])
    {
      if (!visited.contains(depends))
      {
        parseDependencies(depends, visited, stack);
      }
    }
    stack.push_back(vertice);
  }
};

#endif