// Efficiently find the maximal weight Eulerian path in an undirected weighted graph.
// This is a path from a node i to node j, that uses each *edge* at most once.
// 
// by Twan van Laarhoven, 2012-12-24
// License: MIT

// Inspired by the advent of code 2017 day 24

// This code needs blossom5-v2
// available from http://pub.ist.ac.at/~vnk/papers/BLOSSOM5.html

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include "blossom5-v2.05.src/PerfectMatching.h"
using namespace std;

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

const bool VERBOSE = false;

typedef int Cost;

// steps in an (acyclic/shortest) path
struct Path {
  int  prev; // previous node on shortest path
  Cost cost; // total path length
};

struct Edge {
  int  to;
  Cost cost;
  mutable bool marked;
};

// Graph
struct Node {
  vector<Edge> edges;
  
  // for algorithms:
  mutable int id;              // lookup this node in some table
  mutable map<int,Path> dists; // shortest paths from this node
  
  Edge const& find_unmarked_edge_to(int j) const {
    for (auto const& e : edges) {
      if (e.to == j && !e.marked) return e;
    }
    throw "No unmarked edge";
  }
};

// -----------------------------------------------------------------------------
// Brute force solution
// -----------------------------------------------------------------------------

void longest_paths_brute(map<int,Node> const& graph, map<int,Cost>& dist, int i, int cost) {
  if (dist[i] < cost) dist[i] = cost;
  Node const& node_i = graph.at(i);
  for (auto const& edge_j : node_i.edges) {
    if (!edge_j.marked) {
      int j = edge_j.to;
      edge_j.marked = true;
      auto const& edge_i = graph.at(j).find_unmarked_edge_to(i);
        // Note: we have to mark edge_i first, because if i==j we want to mark both endpoints, not the same endpoint twice
      edge_i.marked = true;
      longest_paths_brute(graph, dist, j, cost + edge_j.cost);
      edge_i.marked = false;
      edge_j.marked = false;
    }
  }
}

// Find longest paths to each node, starting from i0
map<int,Cost> longest_paths_brute(map<int,Node> const& graph, int i0) {
  map<int,Cost> dist;
  // we will mark edges that have been used
  for (auto& node : graph) {
    for (auto const& e : node.second.edges) {
      e.marked = false;
    }
  }
  longest_paths_brute(graph, dist, i0, 0);
  return dist;
}

// -----------------------------------------------------------------------------
// Efficient solution
// -----------------------------------------------------------------------------

// Find the shortest paths in a graph, leaving from node i0
map<int,Path> shortest_paths(map<int,Node> const& graph, int i0) {
  map<int,Path> paths;
  priority_queue<pair<Cost,pair<int,int>>> pq;
  pq.push(make_pair(0,make_pair(-1,i0)));
  while (!pq.empty()) {
    Cost d    = -pq.top().first;
    int  prev = pq.top().second.first;
    int  i    = pq.top().second.second;
    pq.pop();
    auto paths_i = paths.find(i);
    if (paths_i == paths.end() || d < paths_i->second.cost) {
      // follow edges
      paths[i] = Path{prev,d};
      for (auto const& j : graph.at(i).edges) {
        pq.push(make_pair(-(d + j.cost), make_pair(i,j.to)));
      }
    }
  }
  return paths;
}

void mark_half_edge(map<int,Node> const& graph, int i, int j) {
  Node const& node_j = graph.at(j);
  node_j.find_unmarked_edge_to(i).marked = true;
}
void mark_edge(map<int,Node> const& graph, int i, int j) {
  if (VERBOSE) printf("    mark %d - %d\n", i, j);
  mark_half_edge(graph, i, j);
  mark_half_edge(graph, j, i);
}
void mark_path(map<int,Node> const& graph, map<int,Path> dists, int j) {
  while (dists[j].prev >= 0) {
    mark_edge(graph, dists[j].prev, j);
    j = dists[j].prev;
  }
}
void print_path(map<int,Path> dists, int j) {
  while (j >= 0) {
    printf(" (%d) %d", dists[j].cost, j);
    j = dists[j].prev;
  }
}

int longest_path_to(map<int,Node> const& graph, int i0, int i1) {
  // Is there even a path from i0 to i1?
  auto const& node_i0 = graph.at(i0);
  if (node_i0.dists.empty()) {
    node_i0.dists = shortest_paths(graph, i0);
  }
  if (node_i0.dists.find(i1) == node_i0.dists.end()) {
    return -1;
  }
  
  // Find exposed nodes, and mapping to ids
  // A node is exposed if it has odd degree, counting an extra edge from i0 to i1  (if i0==i1 both end points count)
  // Each exposed node needs one if its incident edges removed.
  for (const auto& node : graph) {
    node.second.id = -1; // not exposed
  }
  struct Exposed {};
  vector<int> exposed;
  for (auto const& node : graph) {
    int i = node.first;
    int degree = (int)node.second.edges.size();
    if (i == i0) degree++;
    if (i == i1) degree++;
    if (degree % 2 == 1) {
      node.second.id = (int)exposed.size();
      exposed.push_back(i);
      if (VERBOSE) printf("exposed: %d -> [%d]  (degree: %d)\n", i, node.second.id, degree);
    }
    // calculate shortest paths
    if (node.second.dists.empty()) {
      node.second.dists = shortest_paths(graph, i);
    }
  }
  
  // set up PerfectMatching, using shortest paths between exposed nodes as weights
  PerfectMatching matching((int)exposed.size(), (int)(exposed.size()*(exposed.size()-1)));
  matching.options.verbose = false;
  for (auto i : exposed) {
    Node const& node_i = graph.at(i);
    for (auto j : exposed) {
      if (i < j) {
        auto p = node_i.dists.find(j);
        if (p != node_i.dists.end()) {
          Node const& node_j = graph.at(j);
          matching.AddEdge(node_i.id, node_j.id, p->second.cost);
          if (VERBOSE) {
            printf("  [%d] - [%d] = %d  (path: ", node_i.id, node_j.id, p->second.cost);
            print_path(node_i.dists, j);
            printf(")\n");
          }
        }
      }
    }
  }
  
  // Solve perfect matching
  matching.Solve(true);

  if (VERBOSE) {
    for (int id = 0; id < (int)exposed.size() ; ++id) {
      printf("  match: [%d] - [%d]\n", id, matching.GetMatch(id));
    }
  }
  
  // Mark all removed edges
  for (auto const& node : graph) {
    for (auto const& e : node.second.edges) {
      e.marked = false;
    }
  }
  for (int id = 0; id < (int)exposed.size() ; ++id) {
    int i = exposed[id];
    int j = exposed[matching.GetMatch(id)];
    if (j < i) continue;
    // mark the path from i to j
    auto const& node_i = graph.at(i);
    mark_path(graph, node_i.dists, j);
  }

  // Find connected component using only unmarked edges.
  // Each node will have even degree, so there will exist an Euler path that uses all remaining edges.
  // So just count weight of the remaining edges.
  Cost total_cost = 0;
  vector<int> queue;
  set<int> seen;
  queue.push_back(0);
  while (!queue.empty()) {
    int i = queue.back(); queue.pop_back();
    if (seen.count(i)) continue;
    seen.insert(i);
    Node const& node_i = graph.at(i);
    for (Edge const& e : node_i.edges) {
      if (e.marked) continue;
      total_cost += e.cost;
      queue.push_back(e.to);
      if (VERBOSE) printf("  count  %d - %d: %d\n", i, e.to, e.cost);
    }
  }

  return total_cost / 2; // we double counted all edges
}

map<int,Cost> longest_paths(map<int,Node> const& graph, int i0) {
  map<int,Cost> dist;
  for (auto const& node_to : graph) {
    dist[node_to.first] = longest_path_to(graph, i0, node_to.first);
  }
  return dist;
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

// Edge weight/cost according to AoC2017-24 problem
Cost edge_cost(int problem, int i, int j) {
  if (problem == 1) {
    return i+j;
  } else {
    return 10000000 + (i+j);
  }
}

// Main
int main(int argc, const char** argv) {
  // Usage: longest-path <brute> <input>
  // Parse arguments
  if (argc < 2) {
    fprintf(stderr, "Usage: %s {brute|fast} [PROBLEM={1|2}] [FILE]\n", argv[0]);
    return EXIT_FAILURE;
  }
  bool brute_force = argv[1][0] == 'b' || argv[1][0] == 'B' || argv[1][0] == '0';
  int problem = 1;
  if (argc >= 3) problem = string(argv[2]) == "1" ? 1 : 2;
  string input = "-";
  if (argc >= 4) input = argv[3];
  
  // Parse input
  auto f = stdin;
  if (input != "-") {
    f = fopen(input.c_str(),"rt");
  }
  map<int,Node> graph;
  while (1) {
    int i, j, cost;
    if (fscanf(f,"%d/%d\n",&i,&j) == 2) {
      if (fscanf(f,"@%d",&cost) != 1) {
        cost = edge_cost(problem,i,j);
      }
      graph[i].edges.push_back(Edge{j,cost});
      graph[j].edges.push_back(Edge{i,cost});
      if (VERBOSE) printf("%d - %d: %d\n",i,j,cost);
    } else {
      break;
    }
  }
  if (f != stdin) fclose(f);

  printf("%d nodes\n", (int)graph.size());

  // Brute force
  map<int,Cost> dists;
  if (brute_force) {
    dists = longest_paths_brute(graph, 0);
  } else {
    dists = longest_paths(graph,0);
  }
  Cost largest = 0;
  for (auto const& d : dists) {
    if (VERBOSE) {
      printf("%d -> %d: %d\n", 0, d.first, d.second);
    }
    largest = max(largest, d.second);
  }
  printf("longest path length: %d\n", largest);
}

