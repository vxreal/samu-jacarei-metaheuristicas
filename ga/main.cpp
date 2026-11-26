#include "../comum.hpp"

bool menor_valor(const Solucao &a, const Solucao &b) {
  return a.valor < b.valor;
}

bool contem(const std::vector<int> &v, int x) {
  for (int y : v) {
    if (y == x) {
      return true;
    }
  }
  return false;
}

// populacao ordenada do pior para o melhor e rank = posicao + 1
int roleta_rank(int tamanho, std::mt19937 &gen) {
  double soma = tamanho * (tamanho + 1) / 2.0;
  std::uniform_real_distribution<double> dist(0, soma);
  double r = dist(gen);

  double acumulado = 0;
  for (int i = 0; i < tamanho; i++) {
    acumulado += i + 1;
    if (r < acumulado) {
      return i;
    }
  }
  return tamanho - 1;
}

void ajustar_tamanho(std::vector<int> &v, int alvo, std::vector<int> &usados,
                     int n, std::mt19937 &gen) {
  while ((int)v.size() > alvo) {
    std::uniform_int_distribution<int> dist(0, v.size() - 1);
    int pos = dist(gen);
    v.erase(v.begin() + pos);
  }

  std::uniform_int_distribution<int> dist_j(0, n - 1);
  while ((int)v.size() < alvo) {
    int j = dist_j(gen);
    if (!contem(usados, j) && !contem(v, j)) {
      v.push_back(j);
    }
  }
}

// cruzamento de silva et al.: o filho herda os locais da base e cada local
// so da guia com probabilidade 0,5
Solucao cruzar(const Solucao &base, const Solucao &guia, Frota frota, int n,
               std::mt19937 &gen) {
  std::uniform_int_distribution<int> moeda(0, 1);
  std::vector<int> filho_usa;
  std::vector<int> filho_usb;

  for (int k = 0; k < (int)base.local.size(); k++) {
    if (k < frota.usa) {
      filho_usa.push_back(base.local[k]);
    } else {
      filho_usb.push_back(base.local[k]);
    }
  }

  for (int k = 0; k < (int)guia.local.size(); k++) {
    int j = guia.local[k];
    if (k < frota.usa) {
      if (!contem(filho_usa, j) && moeda(gen) == 1) {
        filho_usa.push_back(j);
      }
    } else {
      if (!contem(filho_usb, j) && moeda(gen) == 1) {
        filho_usb.push_back(j);
      }
    }
  }

  // reparo: tira conflito de local e ajusta o numero de viaturas da frota
  std::vector<int> sem_conflito;
  for (int j : filho_usb) {
    if (!contem(filho_usa, j)) {
      sem_conflito.push_back(j);
    }
  }
  filho_usb = sem_conflito;

  std::vector<int> vazio;
  ajustar_tamanho(filho_usa, frota.usa, vazio, n, gen);
  ajustar_tamanho(filho_usb, frota.usb, filho_usa, n, gen);

  Solucao filho;
  for (int j : filho_usa) {
    filho.local.push_back(j);
  }
  for (int j : filho_usb) {
    filho.local.push_back(j);
  }
  return filho;
}

int main(int argc, char **argv) {
  Execucao e = ler_argumentos(argc, argv);
  Instancia inst = carregar("../quadras.csv", raio_em_graus(e.raio));
  Avaliador av(inst, e.modelo == "fleet", e.param.pegar("bonus", 0.2),
               e.frota.usa);
  std::mt19937 gen(e.seed);

  int tamanho = e.param.pegar("pop", 200);
  int geracoes = e.param.pegar("geracoes", 2000);
  double elite = e.param.pegar("elite", 0.3);
  double sorteados = e.param.pegar("sorteados", 0.2);
  int desastre_cada = e.param.pegar("desastre", 30);
  double mortes = e.param.pegar("mortes", 0.5);

  auto inicio = std::chrono::steady_clock::now();

  std::vector<Solucao> populacao;
  for (int i = 0; i < tamanho; i++) {
    Solucao s = solucao_aleatoria(inst, e.frota, gen);
    s.valor = av.avaliar(s);
    populacao.push_back(s);
  }

  std::sort(populacao.begin(), populacao.end(), menor_valor);
  Solucao melhor = populacao[tamanho - 1];

  int qtd_elite = elite * tamanho;
  int qtd_sorteados = sorteados * tamanho;

  for (int g = 1; g <= geracoes; g++) {
    std::vector<Solucao> nova;

    // elite e sorteados do codigo de silva et al.
    for (int i = 0; i < qtd_elite; i++) {
      nova.push_back(populacao[tamanho - 1 - i]);
    }

    // sorteados sem repeticao fora da elite
    std::vector<int> resto(tamanho - qtd_elite);
    for (int i = 0; i < (int)resto.size(); i++) {
      resto[i] = i;
    }
    std::shuffle(resto.begin(), resto.end(), gen);
    for (int i = 0; i < qtd_sorteados && i < (int)resto.size(); i++) {
      nova.push_back(populacao[resto[i]]);
    }

    while ((int)nova.size() < tamanho) {
      const Solucao &pai1 = populacao[roleta_rank(tamanho, gen)];
      const Solucao &pai2 = populacao[roleta_rank(tamanho, gen)];

      Solucao filho;
      if (pai1.valor >= pai2.valor) {
        filho = cruzar(pai1, pai2, e.frota, inst.n, gen);
      } else {
        filho = cruzar(pai2, pai1, e.frota, inst.n, gen);
      }
      filho.valor = av.avaliar(filho);
      nova.push_back(filho);
    }

    populacao = nova;
    std::sort(populacao.begin(), populacao.end(), menor_valor);

    // desastre do codigo de silva et al.: troca parte da populacao por
    // solucoes aleatorias sem repeticao e preserva a melhor (a ultima)
    if (g % desastre_cada == 0) {
      int qtd_mortes = mortes * tamanho;
      std::vector<int> posicoes(tamanho - 1);
      for (int i = 0; i < (int)posicoes.size(); i++) {
        posicoes[i] = i;
      }
      std::shuffle(posicoes.begin(), posicoes.end(), gen);
      for (int i = 0; i < qtd_mortes && i < (int)posicoes.size(); i++) {
        Solucao s = solucao_aleatoria(inst, e.frota, gen);
        s.valor = av.avaliar(s);
        populacao[posicoes[i]] = s;
      }
      std::sort(populacao.begin(), populacao.end(), menor_valor);
    }

    if (populacao[tamanho - 1].valor > melhor.valor + EPS) {
      melhor = populacao[tamanho - 1];
    }
  }

  auto fim = std::chrono::steady_clock::now();
  double tempo = std::chrono::duration<double>(fim - inicio).count();

  imprimir("ga", e, av, melhor, tempo);
  return 0;
}
