#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <ctime>
#include <iomanip>
#include <limits>
#include "SteinerGraph.hpp"
#include "ReaderSteiner.hpp"

// Data e Hora atual formatada
std::string obterDataHoraAtual() {
    std::time_t agora = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buffer[80];
    // Formato-> Ano-Mês-Dia Horas:Minutos:Segundos
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&agora));
    return std::string(buffer);
}

// fnção auxiliar para verificar se o arquivo CSV já existe
bool arquivoExiste(const std::string& nomeArquivo) {
    std::ifstream f(nomeArquivo.c_str());
    return f.good();
}


int main(int argc, char* argv[]) {
   
    unsigned int seed;
    if (argc > 1) {
        try {
            seed = std::stoul(argv[1]);
            std::cout << "[REPLICAÇÃO] Usando semente fornecida por parâmetro.\n";
        } catch (...) {
            std::cerr << "Semente inválida. Gerando semente automática por tempo.\n";
            seed = std::chrono::system_clock::now().time_since_epoch().count();
        }
    } else {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
    }

    std::cout << "==================================================\n";
    // Semente impressa no início para permitir a cópia e replicação exata do lote
    std::cout << " SEMENTE MESTRE DO EXPERIMENTO: " << seed << "\n";
    std::cout << "==================================================\n\n";



    // ==================================================
    //CONFIGURAÇÃO INSTANCIAS E PARÂMETROS
    // ==================================================

    std::vector<std::string> listaGrafos = {
    "instancias/K400.9.txt",
    "instancias/K400.3.txt",
    "instancias/K100.5.txt",
    "instancias/K100.10.txt",
    "instancias/K100.2.txt",
    "instancias/K400.7.txt",
    "instancias/K400.4.txt",
    "instancias/K100.9.txt",
    "instancias/K100.8.txt",
    "instancias/K400.6.txt",
    "instancias/K100.1.txt",
    "instancias/K100.7.txt",
    "instancias/K400.1.txt",
    "instancias/K400.5.txt",
    "instancias/K100.4.txt",
    "instancias/K400.2.txt",
    "instancias/K100.3.txt",
    "instancias/K400.8.txt",
    "instancias/K100.6.txt",
    "instancias/K400.10.txt"
};
    double alphaUnico = 0.3;
    std::vector<double> listaAlfas = {0.1, 0.3, 0.5, 0.8};
    int numIteracoesInternas = 30;
    int numIteracoesInternasAdaptativo = 300; // Iterações de cada chamada do algoritmo
    int tamanhoBloco = 45;
    
    std::string nomeCsv = "resultados_experimento.csv";
    
    // Cria o cabeçalho do CSV caso o arquivo ainda não exista
    if (!arquivoExiste(nomeCsv)) {
        std::ofstream csvCriar(nomeCsv);
        if (csvCriar.is_open()) {
            csvCriar << "Arquivo_Grafo,Melhor_Tempo_ms,Tempo_Medio_ms,Nome_Algoritmo,Parametros,Semente_Mestre,Data_Hora\n";
            csvCriar.close();
        }
    }

    
    for (const std::string& arquivoGrafo : listaGrafos) {
        std::cout << "--------------------------------------------------\n";
        std::cout << "Processando Grafo: " << arquivoGrafo << "\n";
        std::cout << "--------------------------------------------------\n";

        SteinerGraph graph;
        ReaderSteiner::readFromFile(arquivoGrafo, graph);


        std::mt19937 gen(seed);

        // funçao lambda recomendada por IA pra permitir a reprodutibilidade
        auto executarRodadaDeDez = [&](const std::string& nomeAlgo, const std::string& parametrosStr, auto funcaoAlgo) {
            double melhorTempo = std::numeric_limits<double>::max();
            double tempoTotal = 0.0;

            std::cout << " -> Rodando " << nomeAlgo << " (10 vezes)... ";

            for (int i = 0; i < 10; ++i) {
                // Medição precisa de tempo em alta resolução
                auto start = std::chrono::high_resolution_clock::now();
                
                // Executa a função do algoritmo passada por parâmetro
                funcaoAlgo();
                
                auto end = std::chrono::high_resolution_clock::now();
                double duracao = std::chrono::duration<double, std::milli>(end - start).count();

                if (duracao < melhorTempo) {
                    melhorTempo = duracao;
                }
                tempoTotal += duracao;
            }
            
            double tempoMedio = tempoTotal / 10.0;
            std::cout << "Pronto! [Melhor: " << melhorTempo << "ms | Média: " << tempoMedio << "ms]\n";

            // Salva a linha correspondente a essa rodada de 10 execuções no CSV
            std::ofstream csvFile(nomeCsv, std::ios::app);
            if (csvFile.is_open()) {
                csvFile << arquivoGrafo << ","
                        << melhorTempo << ","
                        << tempoMedio << ","
                        << nomeAlgo << ",\""
                        << parametrosStr << "\","
                        << seed << ","
                        << obterDataHoraAtual() << "\n";
                csvFile.close();
            }
        };

        // Guloso 
        executarRodadaDeDez("Guloso Puro", "N/A", [&]() {
            graph.computePureGreedyPCST(gen);
        });

        // Guloso Randomizado Multi-Alpha
        for (int i = 0; i < listaAlfas.size(); ++i) {
            double alpha = listaAlfas[i];
            std::string params = "alpha=" + std::to_string(alpha) + ";iter=" + std::to_string(numIteracoesInternasAdaptativo);
            executarRodadaDeDez("Guloso Randomizado Multi-Alpha", params, [&]() {
                graph.computeRandomizedGreedyMultiAlpha(gen, listaAlfas, numIteracoesInternasAdaptativo);
            });
        }
        // Guloso Randomizado Reativo
        std::string paramsReact = "alphas_qtd=" + std::to_string(listaAlfas.size()) + ";iter=" + std::to_string(numIteracoesInternas) + ";bloco=" + std::to_string(tamanhoBloco);
        executarRodadaDeDez("Randomizado Reativo", paramsReact, [&]() {
            graph.computeReactiveGreedyPCST(gen, listaAlfas, 300, 45);
        });

        std::cout << "Grafo " << arquivoGrafo << " finalizado com sucesso.\n\n";
    }

    std::cout << "==================================================\n";
    std::cout << " EXPERIMENTO CONCLUÍDO! Dados gravados em '" << nomeCsv << "'\n";
    std::cout << "==================================================\n";

    return 0;
}