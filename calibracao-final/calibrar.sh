#!/bin/bash
# calibra os metodos na frota 6 com raio 30 (fora dos testes)
# grava a melhor configuracao em ../parametros_calibrados.txt
# uso: ./calibrar.sh              (os 6 metodos)
#      ./calibrar.sh ga grasp     (so esses)
cd "$(dirname "$0")"
saida=../parametros_calibrados.txt
metodos="$*"
if [ -z "$metodos" ]; then
  metodos="ga simulatedannealing grasp buscatabu ils grasptabu"
  > $saida
fi
touch $saida
for m in $metodos; do
  (cd ../$m && g++ -O2 main.cpp -o programa) || exit 1
  (cd $m && irace --scenario scenario.txt > irace.log 2>&1)
  melhor=$(awk '/Best configurations as commandlines/{getline; l=$0} END{$0=l; $1=""; print substr($0,2)}' $m/irace.log)
  if [ -z "$melhor" ]; then
    echo "$m: irace nao terminou, ver $m/irace.log" >&2
    exit 1
  fi
  # troca so a linha desse metodo
  awk -v m=$m '$1 != m' $saida > $saida.tmp
  echo "$m $melhor" >> $saida.tmp
  mv $saida.tmp $saida
  echo "$m: $melhor"
done
