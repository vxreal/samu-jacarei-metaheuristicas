#!/bin/bash
# compila tudo e roda cada metodo 10 vezes por cenario
# cenarios: modelo m1/fleet x raio 10/15/30 x frota atual/5a/5b
# parametros: parametros_calibrados.txt
# uso: ./rodar_todos.sh             (tudo)
#      ./rodar_todos.sh ga grasp    (so esses)

cd "$(dirname "$0")"

metodos="$*"
parcial=1
if [ -z "$metodos" ]; then
  metodos="ga grasp buscatabu simulatedannealing ils grasptabu"
  parcial=0
fi

if [ ! -s parametros_calibrados.txt ]; then
  echo "falta parametros_calibrados.txt, rode calibracao-final/calibrar.sh" >&2
  exit 1
fi

for m in atual $metodos; do
  (cd $m && g++ -O2 main.cpp -o programa) || exit 1
done

# nome que cada programa escreve na coluna metodo
nome_csv() {
  case $1 in
    simulatedannealing) echo sa ;;
    buscatabu) echo tabu ;;
    *) echo $1 ;;
  esac
}

saida=resultados.csv
if [ $parcial = 1 ]; then
  # mantem as linhas dos metodos que nao vao rodar
  nomes=""
  for m in $metodos; do
    nomes="$nomes $(nome_csv $m)"
  done
  awk -F, -v ms=" $nomes " 'NR == 1 || index(ms, " " $1 " ") == 0' $saida > $saida.tmp
  mv $saida.tmp $saida
else
  echo "metodo,modelo,frota,raio,seed,valor,valor_pct,cobertura_pct,tempo_s,solucao" > $saida
fi

for modelo in m1 fleet; do
  for raio in 10 15 30; do
    # a configuracao atual so existe com a frota atual
    if [ $parcial = 0 ]; then
      (cd atual && ./programa $modelo atual $raio 0) >> $saida
    fi

    for frota in atual 5a 5b; do
      for m in $metodos; do
        params=$(awk -v m=$m '$1 == m {$1 = ""; print substr($0, 2)}' parametros_calibrados.txt)
        for seed in 1 2 3 4 5 6 7 8 9 10; do
          (cd $m && ./programa $modelo $frota $raio $seed $params) >> $saida
          echo "$m $modelo $frota $raio seed $seed ok"
        done
      done
    done
  done
done
