#include "../comum.hpp"

int main(int argc, char **argv) {
  Execucao e = ler_argumentos(argc, argv);
  Instancia inst = carregar("../quadras.csv", raio_em_graus(e.raio));
  Avaliador av(inst, e.modelo == "fleet", e.param.pegar("bonus", 0.2),
               e.frota.usa);
  std::mt19937 gen(e.seed);

  int ils_max = e.param.pegar("ilsmax", 100);
  int nivel = e.param.pegar("nivel", 2);

  auto inicio = std::chrono::steady_clock::now();

  Solucao s = construir(av, e.frota, 1.0, gen);
  vnd(av, s);
  av.descarregar(s);

  std::uniform_int_distribution<int> dist_k(0, s.local.size() - 1);
  std::uniform_int_distribution<int> dist_j(0, inst.n - 1);

  int h = 0;
  while (h <= ils_max) {
    // perturbacao: nivel realocacoes aleatorias
    Solucao vizinho = s;
    std::vector<char> ocupado = montar_ocupado(vizinho, inst.n);
    for (int m = 0; m < nivel; m++) {
      int k = dist_k(gen);
      int j = dist_j(gen);
      while (ocupado[j]) {
        j = dist_j(gen);
      }
      ocupado[vizinho.local[k]] = 0;
      ocupado[j] = 1;
      vizinho.local[k] = j;
    }

    av.carregar(vizinho);
    vnd(av, vizinho);
    av.descarregar(vizinho);

    if (vizinho.valor > s.valor + EPS) {
      s = vizinho;
      h = 0;
    } else {
      h++;
    }
  }

  auto fim = std::chrono::steady_clock::now();
  double tempo = std::chrono::duration<double>(fim - inicio).count();

  imprimir("ils", e, av, s, tempo);
  return 0;
}
