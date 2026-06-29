#ifndef READER_STEINER_HPP
#define READER_STEINER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>
#include "SteinerGraph.hpp"

class ReaderSteiner 
{
public:
    static void readFromFile(const std::string& filename, SteinerGraph& graph) {
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Erro fatal: Nao foi possivel abrir o arquivo '" << filename << "'." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        std::string line;
        
        // Regex para linhas com arestas: u:val v:val peso
        std::regex edgeRegex(R"(^(\d+):(-?\d+) (\d+):(-?\d+) (\d+)$)");
        // Regex para linhas com nós isolados: u:val
        std::regex isolatedNodeRegex(R"(^(\d+):(-?\d+)$)");
        
        std::smatch match;
        int lineCounter = 1;

        while (std::getline(file, line)) {
            if (line.empty()) {
                std::cerr << "Erro de formato na linha " << lineCounter << ": Linha vazia detectada." << std::endl;
                std::exit(EXIT_FAILURE);
            }

            // Tenta casar com o formato de aresta
            if (std::regex_match(line, match, edgeRegex)) {
                int u_name = std::stoi(match[1].str());
                int u_val  = std::stoi(match[2].str());
                int v_name = std::stoi(match[3].str());
                int v_val  = std::stoi(match[4].str());
                int weight = std::stoi(match[5].str());

                graph.addNode(u_name, u_val);
                graph.addNode(v_name, v_val);
                graph.addEdge(u_name, v_name, weight);
            } 
            // Se falhar, tenta casar com o formato de nó isolado
            else if (std::regex_match(line, match, isolatedNodeRegex)) {
                int name = std::stoi(match[1].str());
                int val  = std::stoi(match[2].str());
                
                graph.addNode(name, val);
            } 
            // Se não casar com nenhum dos dois, o arquivo está corrompido/fora do padrão
            else {
                std::cerr << "Erro de formato fatal na linha " << lineCounter << ": \" " << line << " \"" << std::endl;
                std::cerr << "O programa aceita apenas 'u:val v:val peso' ou 'u:val' para nos isolados." << std::endl;
                std::exit(EXIT_FAILURE);
            }
            lineCounter++;
        }

        file.close();
    }
};

#endif