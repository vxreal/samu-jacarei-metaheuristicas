#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

const double EPS = 1e-6;

struct Quadra {
  double x;
  double y;
  double demanda;
};

struct Instancia {
  int n = 0;
  double demanda_total = 0;
  std::vector<Quadra> quadras;
  std::vector<std::vector<int>> cobre;
};

struct Frota {
  int usa;
  int usb;
};

// local[k] e a quadra da viatura k e as primeiras frota.usa sao USA
struct Solucao {
  std::vector<int> local;
  double valor = 0;
};

std::vector<std::string> separar(const std::string &linha) {
  std::vector<std::string> campos;
  std::stringstream ss(linha);
  std::string campo;

  while (std::getline(ss, campo, ',')) {
    campos.push_back(campo);
  }

  return campos;
}

double raio_em_graus(int minutos) {
  if (minutos == 10) {
    return 0.0056;
  }
  if (minutos == 15) {
    return 0.0084;
  }
  if (minutos == 30) {
    return 0.0168;
  }

  std::cerr << "raio invalido, use 10, 15 ou 30\n";
  std::exit(1);
}

Frota ler_frota(const std::string &nome) {
  if (nome == "atual") {
    return {1, 3};
  }
  if (nome == "5a") {
    return {1, 4};
  }
  if (nome == "5b") {
    return {2, 3};
  }
  // usada so na calibracao
  if (nome == "6") {
    return {2, 4};
  }

  std::cerr << "frota invalida, use atual, 5a, 5b ou 6\n";
  std::exit(1);
}

Instancia carregar(const std::string &arquivo, double raio) {
  Instancia inst;
  std::ifstream f(arquivo);

  if (!f) {
    std::cerr << "nao abriu " << arquivo << '\n';
    std::exit(1);
  }

  std::string linha;
  std::getline(f, linha);

  while (std::getline(f, linha)) {
    if (linha.empty()) {
      continue;
    }

    std::vector<std::string> c = separar(linha);
    Quadra q;
    q.x = std::stod(c[1]);
    q.y = std::stod(c[2]);
    q.demanda = std::stod(c[3]);

    inst.quadras.push_back(q);
    inst.demanda_total += q.demanda;
  }

  inst.n = inst.quadras.size();
  inst.cobre.assign(inst.n, {});

  // compara o quadrado da distancia
  double raio2 = raio * raio;
  for (int j = 0; j < inst.n; j++) {
    for (int i = 0; i < inst.n; i++) {
      double dx = inst.quadras[i].x - inst.quadras[j].x;
      double dy = inst.quadras[i].y - inst.quadras[j].y;

      if (dx * dx + dy * dy <= raio2) {
        inst.cobre[j].push_back(i);
      }
    }
  }

  return inst;
}

struct Avaliador {
  const Instancia *inst;
  bool fleet;
  double bonus;
  int num_usa;
  std::vector<int> qtd_usa;
  std::vector<int> qtd_usb;
  double valor = 0;

  Avaliador(const Instancia &in, bool f, double b, int nu) {
    inst = &in;
    fleet = f;
    bonus = b;
    num_usa = nu;
    qtd_usa.assign(in.n, 0);
    qtd_usb.assign(in.n, 0);
  }

  bool eh_usa(int k) const { return k < num_usa; }

  double contribuicao(int i, int a, int b) const {
    double d = inst->quadras[i].demanda;

    if (fleet) {
      if (a > 0 && b > 0) {
        return d;
      }
      return 0;
    }

    if (a > 0) {
      return d * (1 + bonus);
    }
    if (b > 0) {
      return d;
    }
    return 0;
  }

  void colocar(int j, bool usa, int sinal) {
    for (int i : inst->cobre[j]) {
      valor -= contribuicao(i, qtd_usa[i], qtd_usb[i]);

      if (usa) {
        qtd_usa[i] += sinal;
      } else {
        qtd_usb[i] += sinal;
      }

      valor += contribuicao(i, qtd_usa[i], qtd_usb[i]);
    }
  }

  double ganho_colocar(int j, bool usa) const {
    double ganho = 0;

    for (int i : inst->cobre[j]) {
      int a = qtd_usa[i];
      int b = qtd_usb[i];

      if (usa) {
        ganho += contribuicao(i, a + 1, b) - contribuicao(i, a, b);
      } else {
        ganho += contribuicao(i, a, b + 1) - contribuicao(i, a, b);
      }
    }

    return ganho;
  }

  double ganho_simples(int j) const {
    double ganho = 0;

    for (int i : inst->cobre[j]) {
      if (qtd_usa[i] + qtd_usb[i] == 0) {
        ganho += inst->quadras[i].demanda;
      }
    }

    return ganho;
  }

  void carregar(const Solucao &s) {
    for (int k = 0; k < (int)s.local.size(); k++) {
      colocar(s.local[k], eh_usa(k), +1);
    }
  }

  void descarregar(const Solucao &s) {
    for (int k = 0; k < (int)s.local.size(); k++) {
      colocar(s.local[k], eh_usa(k), -1);
    }
    valor = 0;
  }

  double avaliar(const Solucao &s) {
    carregar(s);
    double v = valor;
    descarregar(s);
    return v;
  }

  double cobertura_simples(const Solucao &s) {
    carregar(s);

    double total = 0;
    for (int i = 0; i < inst->n; i++) {
      if (qtd_usa[i] + qtd_usb[i] > 0) {
        total += inst->quadras[i].demanda;
      }
    }

    descarregar(s);
    return total;
  }
};

std::vector<char> montar_ocupado(const Solucao &s, int n) {
  std::vector<char> ocupado(n, 0);

  for (int j : s.local) {
    ocupado[j] = 1;
  }

  return ocupado;
}

Solucao solucao_aleatoria(const Instancia &inst, Frota frota,
                          std::mt19937 &gen) {
  std::uniform_int_distribution<int> dist(0, inst.n - 1);
  std::vector<char> ocupado(inst.n, 0);
  Solucao s;

  while ((int)s.local.size() < frota.usa + frota.usb) {
    int j = dist(gen);

    if (!ocupado[j]) {
      ocupado[j] = 1;
      s.local.push_back(j);
    }
  }

  return s;
}

void mover(Avaliador &av, Solucao &s, std::vector<char> &ocupado, int k,
           int j) {
  av.colocar(s.local[k], av.eh_usa(k), -1);
  av.colocar(j, av.eh_usa(k), +1);

  ocupado[s.local[k]] = 0;
  ocupado[j] = 1;
  s.local[k] = j;
  s.valor = av.valor;
}

double ganho_mover(Avaliador &av, const Solucao &s, int k, int j) {
  double antes = av.valor;

  av.colocar(s.local[k], av.eh_usa(k), -1);
  double ganho = av.valor - antes + av.ganho_colocar(j, av.eh_usa(k));
  av.colocar(s.local[k], av.eh_usa(k), +1);

  av.valor = antes;
  return ganho;
}

// troca de tipo entre uma USA e uma USB = troca dos locais delas
void trocar(Avaliador &av, Solucao &s, int k1, int k2) {
  av.colocar(s.local[k1], av.eh_usa(k1), -1);
  av.colocar(s.local[k2], av.eh_usa(k2), -1);
  av.colocar(s.local[k2], av.eh_usa(k1), +1);
  av.colocar(s.local[k1], av.eh_usa(k2), +1);

  std::swap(s.local[k1], s.local[k2]);
  s.valor = av.valor;
}

double ganho_trocar(Avaliador &av, Solucao &s, int k1, int k2) {
  double antes = av.valor;

  trocar(av, s, k1, k2);
  double ganho = av.valor - antes;
  trocar(av, s, k1, k2);

  av.valor = antes;
  s.valor = antes;
  return ganho;
}

// melhor realocacao de uma viatura para uma quadra livre
double melhor_realocacao(Avaliador &av, const Solucao &s,
                         const std::vector<char> &ocupado, int &melhor_k,
                         int &melhor_j) {
  double antes = av.valor;
  double melhor_ganho = -1e18;
  melhor_k = -1;
  melhor_j = -1;

  for (int k = 0; k < (int)s.local.size(); k++) {
    bool usa = av.eh_usa(k);
    av.colocar(s.local[k], usa, -1);
    double base = av.valor - antes;

    for (int j = 0; j < av.inst->n; j++) {
      if (ocupado[j]) {
        continue;
      }

      double ganho = base + av.ganho_colocar(j, usa);
      if (ganho > melhor_ganho) {
        melhor_ganho = ganho;
        melhor_k = k;
        melhor_j = j;
      }
    }

    av.colocar(s.local[k], usa, +1);
  }

  av.valor = antes;
  return melhor_ganho;
}

double melhor_troca(Avaliador &av, Solucao &s, int &melhor_k1,
                    int &melhor_k2) {
  double melhor_ganho = -1e18;
  melhor_k1 = -1;
  melhor_k2 = -1;

  for (int k1 = 0; k1 < av.num_usa; k1++) {
    for (int k2 = av.num_usa; k2 < (int)s.local.size(); k2++) {
      double ganho = ganho_trocar(av, s, k1, k2);
      if (ganho > melhor_ganho) {
        melhor_ganho = ganho;
        melhor_k1 = k1;
        melhor_k2 = k2;
      }
    }
  }

  return melhor_ganho;
}

// vnd: realocacao e depois troca de tipo
void vnd(Avaliador &av, Solucao &s) {
  std::vector<char> ocupado = montar_ocupado(s, av.inst->n);

  while (true) {
    int k, j;
    if (melhor_realocacao(av, s, ocupado, k, j) > EPS) {
      mover(av, s, ocupado, k, j);
      continue;
    }

    int k1, k2;
    if (melhor_troca(av, s, k1, k2) > EPS) {
      trocar(av, s, k1, k2);
      continue;
    }

    break;
  }

  s.valor = av.valor;
}

// gulosa de church e revelle com lista restrita de diaz et al.
// alfa = 1 e a gulosa pura
Solucao construir(Avaliador &av, Frota frota, double alfa,
                  std::mt19937 &gen) {
  int n = av.inst->n;
  std::vector<char> ocupado(n, 0);
  Solucao s;
  s.local.assign(frota.usa + frota.usb, -1);

  int proxima_usa = 0;
  int proxima_usb = frota.usa;

  for (int passo = 0; passo < frota.usa + frota.usb; passo++) {
    std::vector<int> cand_j;
    std::vector<char> cand_usa;
    std::vector<double> w;
    std::vector<double> w_simples;

    for (int t = 0; t < 2; t++) {
      bool usa = (t == 0);
      if (usa && proxima_usa >= frota.usa) {
        continue;
      }
      if (!usa && proxima_usb >= frota.usa + frota.usb) {
        continue;
      }

      for (int j = 0; j < n; j++) {
        if (ocupado[j]) {
          continue;
        }
        cand_j.push_back(j);
        cand_usa.push_back(usa);
        w.push_back(av.ganho_colocar(j, usa));
        w_simples.push_back(av.ganho_simples(j));
      }
    }

    double w_max = -1e18;
    for (double x : w) {
      w_max = std::max(w_max, x);
    }

    // sem ganho (primeira viatura no fleet) usa a demanda descoberta
    if (w_max <= EPS) {
      w = w_simples;
    }

    w_max = -1e18;
    double w_min = 1e18;
    for (double x : w) {
      w_max = std::max(w_max, x);
      w_min = std::min(w_min, x);
    }

    double limite = w_min + alfa * (w_max - w_min);
    std::vector<int> lrc;
    for (int c = 0; c < (int)w.size(); c++) {
      if (w[c] >= limite - EPS) {
        lrc.push_back(c);
      }
    }

    std::uniform_int_distribution<int> dist(0, lrc.size() - 1);
    int c = lrc[dist(gen)];
    int j = cand_j[c];
    bool usa = cand_usa[c];

    int k;
    if (usa) {
      k = proxima_usa;
      proxima_usa++;
    } else {
      k = proxima_usb;
      proxima_usb++;
    }

    s.local[k] = j;
    ocupado[j] = 1;
    av.colocar(j, usa, +1);
  }

  s.valor = av.valor;
  return s;
}

// busca tabu de diaz et al.: a quadra desocupada fica proibida por tenure
// iteracoes exceto se o movimento gerar a melhor solucao (aspiracao)
Solucao busca_tabu(Avaliador &av, Solucao s, int tenure, int max_sem_melhora) {
  int n = av.inst->n;
  std::vector<char> ocupado = montar_ocupado(s, n);
  std::vector<int> proibido_ate(n, 0);

  Solucao melhor = s;
  int iter = 0;
  int sem_melhora = 0;

  while (sem_melhora < max_sem_melhora) {
    iter++;

    double antes = av.valor;
    double melhor_ganho = -1e18;
    int melhor_k = -1;
    int melhor_j = -1;

    for (int k = 0; k < (int)s.local.size(); k++) {
      bool usa = av.eh_usa(k);
      av.colocar(s.local[k], usa, -1);
      double base = av.valor - antes;

      for (int j = 0; j < n; j++) {
        if (ocupado[j]) {
          continue;
        }

        double ganho = base + av.ganho_colocar(j, usa);
        bool tabu = iter <= proibido_ate[j];
        bool aspiracao = antes + ganho > melhor.valor + EPS;

        if ((!tabu || aspiracao) && ganho > melhor_ganho) {
          melhor_ganho = ganho;
          melhor_k = k;
          melhor_j = j;
        }
      }

      av.colocar(s.local[k], usa, +1);
    }
    av.valor = antes;

    int k1, k2;
    double ganho_troca = melhor_troca(av, s, k1, k2);

    if (ganho_troca > EPS && ganho_troca > melhor_ganho) {
      trocar(av, s, k1, k2);
    } else if (melhor_k != -1) {
      int antigo = s.local[melhor_k];
      mover(av, s, ocupado, melhor_k, melhor_j);
      proibido_ate[antigo] = iter + tenure;
    } else {
      break;
    }

    if (s.valor > melhor.valor + EPS) {
      melhor = s;
      sem_melhora = 0;
    } else {
      sem_melhora++;
    }
  }

  av.descarregar(s);
  av.carregar(melhor);
  melhor.valor = av.valor;
  return melhor;
}

struct Parametros {
  std::vector<std::string> chaves;
  std::vector<double> valores;

  double pegar(const std::string &chave, double padrao) const {
    for (int i = 0; i < (int)chaves.size(); i++) {
      if (chaves[i] == chave) {
        return valores[i];
      }
    }
    return padrao;
  }
};

struct Execucao {
  std::string modelo;
  std::string nome_frota;
  int raio;
  int seed;
  Frota frota;
  Parametros param;
};

// uso: ./programa modelo frota raio seed [chave=valor ...]
Execucao ler_argumentos(int argc, char **argv) {
  if (argc < 5) {
    std::cerr << "uso: ./programa m1|fleet atual|5a|5b|6 10|15|30 seed "
                 "[chave=valor ...]\n";
    std::exit(1);
  }

  Execucao e;
  e.modelo = argv[1];
  e.nome_frota = argv[2];
  e.raio = std::stoi(argv[3]);
  e.seed = std::stoi(argv[4]);
  e.frota = ler_frota(e.nome_frota);

  if (e.modelo != "m1" && e.modelo != "fleet") {
    std::cerr << "modelo invalido, use m1 ou fleet\n";
    std::exit(1);
  }

  for (int i = 5; i < argc; i++) {
    std::string arg = argv[i];
    int pos = arg.find('=');
    e.param.chaves.push_back(arg.substr(0, pos));
    e.param.valores.push_back(std::stod(arg.substr(pos + 1)));
  }

  return e;
}

// uma linha por execucao:
// metodo,modelo,frota,raio,seed,valor,valor_pct,cobertura_pct,tempo_s,solucao
void imprimir(const std::string &metodo, const Execucao &e, Avaliador &av,
              const Solucao &s, double tempo) {
  double total = av.inst->demanda_total;
  double valor = av.avaliar(s);
  double simples = av.cobertura_simples(s);

  std::cout << metodo << ',' << e.modelo << ',' << e.nome_frota << ','
            << e.raio << ',' << e.seed << ',' << valor << ','
            << 100 * valor / total << ',' << 100 * simples / total << ','
            << tempo << ',';

  for (int k = 0; k < (int)s.local.size(); k++) {
    if (av.eh_usa(k)) {
      std::cout << "USA:";
    } else {
      std::cout << "USB:";
    }
    std::cout << s.local[k];

    if (k + 1 < (int)s.local.size()) {
      std::cout << ' ';
    }
  }
  std::cout << '\n';
}
