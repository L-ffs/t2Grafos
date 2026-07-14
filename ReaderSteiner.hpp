#ifndef READER_STEINER_HPP
#define READER_STEINER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include "SteinerGraph.hpp"

class ReaderSteiner {
public:
    static void readFromFile(const std::string& filename, SteinerGraph& graph) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
            exit(1);
        }

        std::string line;
        int lineNum = 0;
        while (std::getline(file, line)) {
            lineNum++;
            
            // 1. Remover espaços em branco no início e no fim (resolve o bug do espaço inicial e do \r do Windows)
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue; // Pula linhas vazias
            size_t end = line.find_last_not_of(" \t\r\n");
            std::string trimmedLine = line.substr(start, end - start + 1);

            // Ignorar linhas de comentário (se houver alguma no arquivo)
            if (trimmedLine[0] == '#' || trimmedLine[0] == 'c' || trimmedLine[0] == '%') {
                continue;
            }

            // 2. Substituir ':' por ' ' para facilitar a leitura dos números
            std::replace(trimmedLine.begin(), trimmedLine.end(), ':', ' ');

            // 3. Ler os números inteiros da linha de forma segura
            std::stringstream ss(trimmedLine);
            std::vector<int> tokens;
            int val;
            while (ss >> val) {
                tokens.push_back(val);
            }

            // 4. Processar de acordo com a quantidade de números lidos
            if (tokens.size() == 5) {
                // Formato: u:val_u v:val_v peso -> vira 5 inteiros (u, val_u, v, val_v, peso)
                int u = tokens[0];
                int val_u = tokens[1];
                int v = tokens[2];
                int val_v = tokens[3];
                int weight = tokens[4];

                graph.addNode(u, val_u);
                graph.addNode(v, val_v);
                graph.addEdge(u, v, weight);
            } 
            else if (tokens.size() == 2) {
                // Formato: u:val_u (nó isolado) -> vira 2 inteiros (u, val_u)
                int u = tokens[0];
                int val_u = tokens[1];
                graph.addNode(u, val_u);
            } 
            else {
                std::cerr << "Erro de formato fatal na linha " << lineNum << ": \"" << line << "\"\n";
                std::cerr << "Tokens lidos: " << tokens.size() << " (esperado 5 para arestas ou 2 para nós isolados).\n";
                exit(1);
            }
        }
        file.close();
    }
};

#endif