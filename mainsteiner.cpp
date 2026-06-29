#include <iostream>
#include <fstream>
#include "SteinerGraph.hpp"
#include "ReaderSteiner.hpp"

void createDummyFile(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        // Arestas normais
        file << "1:10 2:20 5\n";
        file << "2:20 3:100 10\n"; // Note que o nó 3 tem um prêmio gigante (100)
        file << "3:100 4:5 50\n";  // Ir para o nó 4 dá prejuízo (prêmio 5, custo 50)
        
        // Nós completamente isolados (sem arestas)
        file << "5:50\n";
        file << "6:60\n";
        file.close();
        std::cout << "[INFO] Arquivo '" << filename << "' gerado com nos isolados (5 e 6).\n\n";
    }
}

int main() {
    std::string filename = "grafo_teste.txt";
    createDummyFile(filename);

    SteinerGraph completo;

    std::cout << "========== 1. CARREGANDO GRAFO ORIGINAL ==========\n";
    ReaderSteiner::readFromFile(filename, completo);
    completo.printGraph();
    std::cout << "==================================================\n\n";

    std::cout << "========== 2. EXECUTANDO HEURISTICA PCST ==========\n";
    // Roda a heurística gulosa a partir de um nó aleatório
    SteinerGraph arvoreResultado = completo.computeGreedyPCST();
    
    std::cout << "\nSubarvore de Steiner gerada pela heuristica:\n";
    arvoreResultado.printGraph();
    std::cout << "==================================================\n";

    return 0;
}