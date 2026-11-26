#include "../comum.hpp"

int main(int argc, char **argv) {
  Execucao e = ler_argumentos(argc, argv);
  Instancia inst = carregar("../quadras.csv", raio_em_graus(e.raio));
  Avaliador av(inst, e.modelo == "fleet", e.param.pegar("bonus", 0.2),
               e.frota.usa);
  std::mt19937 gen(e.seed);

  int tenure = e.param.pegar("tenure", 7);
  int max_sem_melhora = e.param.pegar("semmelhora", 50);

  auto inicio = std::chrono::steady_clock::now();

  Solucao s = construir(av, e.frota, 1.0, gen);
  Solucao melhor = busca_tabu(av, s, tenure, max_sem_melhora);
  av.descarregar(melhor);

  auto fim = std::chrono::steady_clock::now();
  double tempo = std::chrono::duration<double>(fim - inicio).count();

  imprimir("tabu", e, av, melhor, tempo);
  return 0;
}
