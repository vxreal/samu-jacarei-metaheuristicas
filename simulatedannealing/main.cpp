#include "../comum.hpp"

int main(int argc, char **argv) {
  Execucao e = ler_argumentos(argc, argv);
  Instancia inst = carregar("../quadras.csv", raio_em_graus(e.raio));
  Avaliador av(inst, e.modelo == "fleet", e.param.pegar("bonus", 0.2),
               e.frota.usa);
  std::mt19937 gen(e.seed);

  double temperatura = e.param.pegar("t0", 50);
  double mu = e.param.pegar("mu", 0.9);
  int n_max = e.param.pegar("n", 100);
  double tolerancia = e.param.pegar("tol", 0.001);

  auto inicio = std::chrono::steady_clock::now();

  Solucao s = solucao_aleatoria(inst, e.frota, gen);
  av.carregar(s);
  s.valor = av.valor;
  std::vector<char> ocupado = montar_ocupado(s, inst.n);
  Solucao melhor = s;

  std::uniform_int_distribution<int> dist_k(0, s.local.size() - 1);
  std::uniform_int_distribution<int> dist_j(0, inst.n - 1);
  std::uniform_real_distribution<double> dist_r(0, 1);

  int count = 0;
  while (temperatura >= tolerancia) {
    int k = dist_k(gen);
    int j = dist_j(gen);
    while (ocupado[j]) {
      j = dist_j(gen);
    }

    double ganho = ganho_mover(av, s, k, j);

    if (ganho > EPS) {
      mover(av, s, ocupado, k, j);
      if (s.valor > melhor.valor + EPS) {
        melhor = s;
      }
    } else if (dist_r(gen) < std::exp(ganho / temperatura)) {
      mover(av, s, ocupado, k, j);
    }

    // adaptado de murray e church: resfria a cada n tentativas mesmo com
    // melhora
    count++;
    if (count >= n_max) {
      temperatura *= mu;
      count = 0;
    }
  }

  av.descarregar(s);

  auto fim = std::chrono::steady_clock::now();
  double tempo = std::chrono::duration<double>(fim - inicio).count();

  imprimir("sa", e, av, melhor, tempo);
  return 0;
}
