#ifndef STEINER_GRAPH_HPP
#define STEINER_GRAPH_HPP

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>

class Node
{
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

class SteinerGraph
{
private:
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, std::unordered_map<int, int>> adj;

public:
    SteinerGraph() {}
    ~SteinerGraph() {}

    void addNode(int name, int value) {
        nodes[name] = Node(name, value);
    }

    void removeNode(int name) {
        nodes.erase(name);
        if (adj.count(name)) {
            for (auto& pair : adj[name]) {
                int neighbor = pair.first;
                adj[neighbor].erase(name);
            }
            adj.erase(name);
        }
    }

    void modifyNode(int name, int newValue) {
        if (nodes.count(name)) {
            nodes[name].value = newValue;
        }
    }

    void addEdge(int u, int v, int weight) {
        if (nodes.count(u) && nodes.count(v)) {
            adj[u][v] = weight;
            adj[v][u] = weight;
        }
    }

    void removeEdge(int u, int v) {
        if (adj.count(u)) adj[u].erase(v);
        if (adj.count(v)) adj[v].erase(u);
    }

    void modifyEdge(int u, int v, int newWeight) {
        if (adj.count(u) && adj[u].count(v)) {
            adj[u][v] = newWeight;
            adj[v][u] = newWeight;
        }
    }

    // --- IMPRESSÃO ATUALIZADA (Suporta Nós Isolados) ---
    void printGraph() {
        // 1. Imprime todas as arestas existentes
        for (auto& pairU : adj) {
            int u = pairU.first;
            for (auto& pairV : pairU.second) {
                int v = pairV.first;
                if (u < v) { 
                    std::cout << nodes[u].name << ":" << nodes[u].value << " "
                              << nodes[v].name << ":" << nodes[v].value << " "
                              << pairV.second << "\n";
                }
            }
        }
        
        // 2. Imprime nós que não possuem nenhuma aresta vinculada
        for (auto& pair : nodes) {
            int u = pair.first;
            if (adj.count(u) == 0 || adj[u].empty()) {
                std::cout << nodes[u].name << ":" << nodes[u].value << "\n";
            }
        }
    }

    // --- ALGORITMO GULOSO E HEURÍSTICO PARA PCST ---
    SteinerGraph computeGreedyPCST() {
        SteinerGraph tree;
        if (nodes.empty()) return tree;

        // Sorteia um nó inicial a partir da lista de nós do grafo
        std::vector<int> allNodes;
        for (auto& pair : nodes) {
            allNodes.push_back(pair.first);
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, allNodes.size() - 1);
        int startNode = allNodes[dis(gen)];

        // Estrutura para controlar quem já entrou na nossa árvore
        std::unordered_set<int> visited;
        
        // Inicializa a árvore com o nó sorteado
        tree.addNode(startNode, nodes[startNode].value);
        visited.insert(startNode);

        std::cout << "[HEURISTICA] No inicial sorteado: " << startNode << "\n";

        while (true) {
            int bestU = -1;
            int bestV = -1;
            int bestWeight = -1;
            int maxProfit = -999999; // Equivalente a menos infinito inicial

            // Busca em todas as fronteiras da árvore atual por uma expansão lucrativa
            for (int u : visited) {
                if (adj.count(u)) {
                    for (auto& edge : adj[u]) {
                        int v = edge.first;
                        int weight = edge.second;

                        // Só avalia nós vizinhos que ainda não estão na árvore
                        if (visited.find(v) == visited.end()) {
                            // Métrica: Lucro = Prêmio do Vizinho - Custo da Aresta
                            int profit = nodes[v].value - weight;
                            if (profit > maxProfit) {
                                maxProfit = profit;
                                bestU = u;
                                bestV = v;
                                bestWeight = weight;
                            }
                        }
                    }
                }
            }

            // CRITÉRIO DE PARADA: Se não há mais vizinhos alcançáveis 
            // ou se a melhor opção disponível der prejuízo (<= 0), encerra a árvore.
            if (bestV == -1 || maxProfit <= 0) {
                break;
            }

            // Adiciona o nó e a aresta vencedora na subárvore de Steiner
            tree.addNode(bestV, nodes[bestV].value);
            tree.addEdge(bestU, bestV, bestWeight);
            visited.insert(bestV);
        }

        return tree;
    }
};

#endif