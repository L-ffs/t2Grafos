#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <ctime>
#include <iomanip>
#include <limits>
#include <unordered_map>
#include "SteinerGraph.hpp"
#include "ReaderSteiner.hpp"
#include <functional>

std::string obterDataHoraAtual() {
    std::time_t agora = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&agora));
    return std::string(buffer);
}

bool arquivoExiste(const std::string& nomeArquivo) {
    std::ifstream f(nomeArquivo.c_str());
    return f.good();
}

int main(int argc, char* argv[]) {
    unsigned int seed;
    if (argc > 1) {
        try { seed = std::stoul(argv[1]); }
        catch (...) { seed = std::chrono::system_clock::now().time_since_epoch().count(); }
    } else {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
    }
    std::cout << "SEMENTE MESTRE: " << seed << "\n";

    std::unordered_map<std::string, int> paperLB = {
        {"K100", 135511}, {"K100.1", 124108}, {"K100.2", 200262}, {"K100.3", 115953},
        {"K100.4", 87498}, {"K100.5", 119078}, {"K100.6", 132886}, {"K100.7", 172457},
        {"K100.8", 210869}, {"K100.9", 122917}, {"K100.10", 133567},
        {"K400", 350093}, {"K400.1", 490771}, {"K400.2", 477073}, {"K400.3", 401881},
        {"K400.4", 389451}, {"K400.5", 519526}, {"K400.6", 374849}, {"K400.7", 474466},
        {"K400.8", 418614}, {"K400.9", 383105}, {"K400.10", 394191}
    };

    std::vector<std::string> listaGrafos = {
        "instancias/K400.9.txt", "instancias/K400.3.txt", "instancias/K100.5.txt",
        "instancias/K100.10.txt", "instancias/K100.2.txt", "instancias/K400.7.txt",
        "instancias/K400.4.txt", "instancias/K100.9.txt", "instancias/K100.8.txt",
        "instancias/K400.6.txt", "instancias/K100.1.txt", "instancias/K100.7.txt",
        "instancias/K400.1.txt", "instancias/K400.5.txt", "instancias/K100.4.txt",
        "instancias/K400.2.txt", "instancias/K100.3.txt", "instancias/K400.8.txt",
        "instancias/K100.6.txt", "instancias/K400.10.txt"
    };

    std::vector<double> listaAlfas = {0.1, 0.3, 0.5, 0.8};
    std::string nomeCsv = "resultados_experimento_2.csv";

    // Cabeçalho com EXATAMENTE as 13 colunas solicitadas
    if (!arquivoExiste(nomeCsv)) {
        std::ofstream csvCriar(nomeCsv);
        if (csvCriar.is_open()) {
            csvCriar << "Data_Hora,Instancia,Algoritmo,Parametros_Algoritmo,Semente_Mestra,"
                     << "Tempo_Medio_ms,Media_NetProfit,Media_Custo_Equivalente,LB_Cost_Paper,"
                     << "Gap(%),Melhor_NetProfit,Melhor_Custo_Equivalente,Alpha_Melhor_Custo\n";
            csvCriar.close();
        }
    }

    for (const std::string& arquivoGrafo : listaGrafos) {
        std::string nomeInstancia = arquivoGrafo;
        size_t start = nomeInstancia.find('/') + 1;
        size_t end = nomeInstancia.find(".txt");
        nomeInstancia = nomeInstancia.substr(start, end - start);

        SteinerGraph graph;
        ReaderSteiner::readFromFile(arquivoGrafo, graph);

        int P_total = graph.getTotalPrize();
        int lbPaper = paperLB.count(nomeInstancia) ? paperLB[nomeInstancia] : 0;
        std::mt19937 gen(seed);

        // Lambda ajustada para lidar com o novo struct ResultadoExecucao
        auto executarRodadaDeDez = [&](const std::string& nomeAlgo, const std::string& parametrosStr, auto funcaoAlgo) {
            long long somaTotalNetProfit = 0;
            int totalIteracoes = 0;
            int melhorNetProfitGlobal = std::numeric_limits<int>::min();
            double alphaDoMelhorGlobal = -1.0;
            double tempoTotal = 0.0;
            std::string alphaDoMelhorGlobalStr = "N/A";

            std::cout << " -> Rodando " << nomeAlgo << " (10 execuções)... ";
            
            for (int i = 0; i < 10; ++i) {
                size_t hashAlgo = std::hash<std::string>{}(nomeAlgo + "|" + parametrosStr);
                unsigned int seedChamada = seed ^ (hashAlgo + 0x9e3779b9 + (i << 6) + (i >> 2));
                std::mt19937 genChamada(seedChamada);
                
                auto start = std::chrono::high_resolution_clock::now();
                ResultadoExecucao resultado = funcaoAlgo(genChamada);
                auto end = std::chrono::high_resolution_clock::now();
                
                double duracao = std::chrono::duration<double, std::milli>(end - start).count();
                tempoTotal += duracao;
                
                // Acumula as estatísticas de todas as iterações internas
                somaTotalNetProfit += resultado.soma_net_profit;
                totalIteracoes += resultado.total_iteracoes;
                
                // Busca o melhor absoluto entre todas as execuções e iterações
                if (resultado.melhor_net_profit > melhorNetProfitGlobal) {
                    melhorNetProfitGlobal = resultado.melhor_net_profit;
                    alphaDoMelhorGlobal = resultado.alpha_melhor;
                }
            }
            
            // CÁLCULOS FINAIS
            double tempoMedio = tempoTotal / 10.0; // Tempo médio de 1 execução completa (com n iterações)
            
            // Média real de todas as iterações (10 * n)
            double mediaNetProfit = (double)somaTotalNetProfit / totalIteracoes;
            double mediaCustoEquivalente = P_total - mediaNetProfit;
            
            // Melhores absolutos
            int melhorCustoEquivalente = P_total - melhorNetProfitGlobal;
            double gap = (lbPaper > 0) ? (100.0 * (melhorCustoEquivalente - lbPaper) / lbPaper) : 0.0;
            
            std::string alphaStr = (alphaDoMelhorGlobal >= 0.0) ? std::to_string(alphaDoMelhorGlobal) : "N/A";

            std::cout << "Pronto! [Melhor Custo: " << melhorCustoEquivalente << " | LB: " << lbPaper << " | Gap: " << std::fixed << std::setprecision(2) << gap << "%]\n";

            std::ofstream csvFile(nomeCsv, std::ios::app);
            if (csvFile.is_open()) {
                csvFile << obterDataHoraAtual() << ","
                        << nomeInstancia << ","
                        << nomeAlgo << ","
                        << "\"" << parametrosStr << "\","
                        << seed << ","
                        << std::fixed << std::setprecision(5) << tempoMedio << ","
                        << std::fixed << std::setprecision(2) << mediaNetProfit << ","
                        << std::fixed << std::setprecision(2) << mediaCustoEquivalente << ","
                        << lbPaper << ","
                        << std::fixed << std::setprecision(3) << gap << ","
                        << melhorNetProfitGlobal << ","
                        << melhorCustoEquivalente << ","
                        << alphaStr << "\n";
                csvFile.close();
            }
        };

        // 1. Guloso Puro (1 iteração por execução)
        executarRodadaDeDez("Guloso Puro", "N/A", [&](std::mt19937& g) {
            return graph.computePureGreedyPCST(g);
        });

        // 2. Guloso Randomizado Alpha Fixo (30 iterações por execução)
        for (size_t i = 0; i < listaAlfas.size(); ++i) {
            double alpha = listaAlfas[i];
            std::string params = "alpha=" + std::to_string(alpha) + "; iter=30";
            executarRodadaDeDez("Guloso Randomizado Alpha Fixo", params, [&](std::mt19937& g) {
                return graph.computeRandomizedGreedyMultiAlpha(g, alpha, 30);
            });
        }

        // 3. Guloso Randomizado Reativo (300 iterações por execução)
        std::string paramsReact = "alphas_qtd=" + std::to_string(listaAlfas.size()) + ";iter=300;bloco=45";
        executarRodadaDeDez("Randomizado Reativo", paramsReact, [&](std::mt19937& g) {
            return graph.computeReactiveGreedyPCST(g, listaAlfas, 300, 45);
        });
    }

    std::cout << "==================================================\n";
    std::cout << " EXPERIMENTO CONCLUÍDO! Dados gravados em '" << nomeCsv << "'\n";
    std::cout << "==================================================\n";

    return 0;
}