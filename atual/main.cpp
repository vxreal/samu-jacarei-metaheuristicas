#include "../comum.hpp"

// avalia a configuracao atual do samu de jacarei. cada base vai para a
// quadra de centroide mais proximo
int main(int argc, char **argv) {
  Execucao e = ler_argumentos(argc, argv);
  if (e.nome_frota != "atual") {
    std::cerr << "use a frota atual\n";
    return 1;
  }

  Instancia inst = carregar("../quadras.csv", raio_em_graus(e.raio));
  Avaliador av(inst, e.modelo == "fleet", e.param.pegar("bonus", 0.2),
               e.frota.usa);

  std::ifstream f("../bases_atuais.csv");
  std::string linha;
  std::getline(f, linha);

  std::vector<int> usa;
  std::vector<int> usb;

  while (std::getline(f, linha)) {
    if (linha.empty()) {
      continue;
    }

    std::vector<std::string> c = separar(linha);
    double x = std::stod(c[1]);
    double y = std::stod(c[2]);

    int mais_perto = 0;
    double menor = 1e18;
    for (int i = 0; i < inst.n; i++) {
      double dx = inst.quadras[i].x - x;
      double dy = inst.quadras[i].y - y;
      if (dx * dx + dy * dy < menor) {
        menor = dx * dx + dy * dy;
        mais_perto = i;
      }
    }

    // a usa fica na unidade dr. thelmo (silva et al. secao 4.1)
    if (c[0].find("THELMO") != std::string::npos) {
      usa.push_back(mais_perto);
    } else {
      usb.push_back(mais_perto);
    }
  }

  Solucao s;
  for (int j : usa) {
    s.local.push_back(j);
  }
  for (int j : usb) {
    s.local.push_back(j);
  }

  imprimir("atual", e, av, s, 0);
  return 0;
}
