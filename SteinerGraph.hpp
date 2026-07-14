#ifndef STEINER_GRAPH_HPP
#define STEINER_GRAPH_HPP
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>

class Node {
public:
    int name;
    int value;
    Node() : name(0), value(0) {}
    Node(int name, int value) {
        this->name = name;
        this->value = value;
    }
    ~Node() {}
};

// NOVO STRUCT: Retorna as estatísticas completas de uma rodada de execuções
struct ResultadoExecucao {
    long long soma_net_profit;   // Soma do net_profit de todas as iterações
    int total_iteracoes;         // Quantidade total de iterações realizadas
    int melhor_net_profit;       // O maior net_profit encontrado entre as iterações
    double alpha_melhor;         // O alpha que gerou o melhor net_profit (-1.0 se não aplicável)
};

class SteinerGraph {
private:
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, std::unordered_map<int, int>> adj;

public:
    SteinerGraph() {}
    ~SteinerGraph() {}

    void addNode(int name, int value) { nodes[name] = Node(name, value); }
    
    void addEdge(int u, int v, int weight) {
        if (nodes.count(u) && nodes.count(v)) {
            adj[u][v] = weight;
            adj[v][u] = weight;
        }
    }

    int getTotalPrize() const {
        int total = 0;
        for (const auto& pair : nodes) {
            total += pair.second.value;
        }
        return total;
    }

    std::pair<SteinerGraph, int> buildTreeRandomized(std::mt19937& gen, double alpha) {
        SteinerGraph tree;
        if (nodes.empty()) return {tree, 0};

        std::vector<int> allNodes;
        for (auto& pair : nodes) allNodes.push_back(pair.first);
        std::uniform_int_distribution<> dis(0, allNodes.size() - 1);
        int startNode = allNodes[dis(gen)];

        std::unordered_set<int> visited;
        tree.addNode(startNode, nodes[startNode].value);
        visited.insert(startNode);
        int lucro_total = nodes[startNode].value;

        while (true) {
            struct Candidate { int u; int v; int weight; int profit; };
            std::vector<Candidate> candidates;

            for (int u : visited) {
                if (adj.count(u)) {
                    for (auto& edge : adj[u]) {
                        int v = edge.first;
                        int weight = edge.second;
                        if (visited.find(v) == visited.end()) {
                            int profit = nodes[v].value - weight;
                            candidates.push_back({u, v, weight, profit});
                        }
                    }
                }
            }

            if (candidates.empty()) break;

            int maxProfit = -999999;
            int minProfit = 999999;
            for (const auto& c : candidates) {
                if (c.profit > maxProfit) maxProfit = c.profit;
                if (c.profit < minProfit) minProfit = c.profit;
            }

            if (maxProfit <= 0) break;

            double threshold = maxProfit - alpha * (maxProfit - minProfit);
            std::vector<Candidate> rcl;
            for (const auto& c : candidates) {
                if (c.profit >= threshold && c.profit > 0) rcl.push_back(c);
            }

            if (rcl.empty()) break;

            std::uniform_int_distribution<> rclDis(0, rcl.size() - 1);
            Candidate chosen = rcl[rclDis(gen)];

            tree.addNode(chosen.v, nodes[chosen.v].value);
            tree.addEdge(chosen.u, chosen.v, chosen.weight);
            visited.insert(chosen.v);
            lucro_total += chosen.profit;
        }
        return {tree, lucro_total};
    }

    // 1 Iteração
    ResultadoExecucao computePureGreedyPCST(std::mt19937& gen) {
        ResultadoExecucao res;
        auto [tree, profit] = buildTreeRandomized(gen, 0.0);
        res.soma_net_profit = profit;
        res.total_iteracoes = 1;
        res.melhor_net_profit = profit;
        res.alpha_melhor = -1.0; // Não usa alpha
        return res;
    }

    // N Iterações (Alpha Fixo)
    ResultadoExecucao computeRandomizedGreedyMultiAlpha(std::mt19937& gen, double alpha, int iterations) {
        ResultadoExecucao res;
        res.soma_net_profit = 0;
        res.total_iteracoes = iterations;
        res.melhor_net_profit = std::numeric_limits<int>::min();
        res.alpha_melhor = alpha;

        std::vector<uint32_t> subSeeds(iterations);
        for (int i = 0; i < iterations; ++i) subSeeds[i] = gen();

        for (int i = 0; i < iterations; ++i) {
            std::mt19937 subGen(subSeeds[i]);
            auto [currentTree, currentProfit] = buildTreeRandomized(subGen, alpha);
            res.soma_net_profit += currentProfit;
            if (currentProfit > res.melhor_net_profit) {
                res.melhor_net_profit = currentProfit;
            }
        }
        return res;
    }

    // N Iterações (Reativo)
    ResultadoExecucao computeReactiveGreedyPCST(std::mt19937& gen, const std::vector<double>& alphas, int iterations, int blockSize) {
        ResultadoExecucao res;
        res.soma_net_profit = 0;
        res.total_iteracoes = iterations;
        res.melhor_net_profit = std::numeric_limits<int>::min();
        res.alpha_melhor = -1.0;

        size_t m = alphas.size();
        std::vector<double> probabilities(m, 1.0 / m);
        std::vector<std::vector<int>> blockHistory(m);

        for (int it = 1; it <= iterations; ++it) {
            std::discrete_distribution<> dist(probabilities.begin(), probabilities.end());
            int idx_alpha = dist(gen);
            double alpha = alphas[idx_alpha];

            auto [currentTree, currentProfit] = buildTreeRandomized(gen, alpha);
            res.soma_net_profit += currentProfit;
            blockHistory[idx_alpha].push_back(currentProfit);

            if (currentProfit > res.melhor_net_profit) {
                res.melhor_net_profit = currentProfit;
                res.alpha_melhor = alpha; // Rastreia o alpha da melhor solução global
            }

            if (it % blockSize == 0) {
                std::vector<double> qualities(m, 0.0);
                double sumQualities = 0.0;
                for (size_t i = 0; i < m; ++i) {
                    if (!blockHistory[i].empty()) {
                        double sumProfits = 0;
                        for (int p : blockHistory[i]) sumProfits += p;
                        double avgProfit = sumProfits / blockHistory[i].size();
                        qualities[i] = std::max(0.0, avgProfit);
                        sumQualities += qualities[i];
                    }
                }
                for (size_t i = 0; i < m; ++i) {
                    if (sumQualities > 0.0) probabilities[i] = qualities[i] / sumQualities;
                    else probabilities[i] = 1.0 / m;
                    blockHistory[i].clear();
                }
            }
        }
        return res;
    }
};
#endif