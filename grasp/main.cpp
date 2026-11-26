#include "../comum.hpp"

int main(int argc, char **argv) {
  Execucao e = ler_argumentos(argc, argv);
  Instancia inst = carregar("../quadras.csv", raio_em_graus(e.raio));
  Avaliador av(inst, e.modelo == "fleet", e.param.pegar("bonus", 0.2),
               e.frota.usa);
  std::mt19937 gen(e.seed);

  double alfa_inicial = e.param.pegar("alfaini", 0.75);
  double alfa_final = e.param.pegar("alfafim", 0.95);
  int max_sem_melhora = e.param.pegar("semmelhora", 50);

  auto inicio = std::chrono::steady_clock::now();

  // alfa sobe 0,05 a cada 5 iteracoes ate alfafim e volta ao inicio
  // (diaz et al.). o 1e-9 corrige o arredondamento do floor
  int niveis = std::floor((alfa_final - alfa_inicial) / 0.05 + 1e-9) + 1;

  Solucao melhor;
  melhor.valor = -1;
  int iter = 0;
  int sem_melhora = 0;

  while (sem_melhora < max_sem_melhora) {
    double alfa = alfa_inicial + 0.05 * ((iter / 5) % niveis);

    Solucao s = construir(av, e.frota, alfa, gen);
    vnd(av, s);
    av.descarregar(s);

    if (s.valor > melhor.valor + EPS) {
      melhor = s;
      sem_melhora = 0;
    } else {
      sem_melhora++;
    }
    iter++;
  }

  auto fim = std::chrono::steady_clock::now();
  double tempo = std::chrono::duration<double>(fim - inicio).count();

  imprimir("grasp", e, av, melhor, tempo);
  return 0;
}
