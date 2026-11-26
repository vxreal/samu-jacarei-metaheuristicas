#!/bin/bash
# roda o resolvedor exato em todos os cenarios (limite de 5 h cada)
# pula o que ja esta em exato.csv

cd "$(dirname "$0")"
py=${PY:-python3}
saida=exato.csv

if [ ! -f $saida ]; then
  echo "modelo,frota,raio,lambda,valor,valor_pct,cobertura_pct,limitante,gap,otimo,tempo_s,montagem_s,solucao" > $saida
fi

rodar() {
  if grep -q "^$1,$2,$3,$4," $saida; then
    echo "$1 $2 $3 $4 ja feito"
    return
  fi
  echo "$1 $2 $3 $4 comecou $(date)"
  $py exato.py $1 $2 $3 $4 >> $saida
  echo "$1 $2 $3 $4 terminou $(date)"
}

# 1) frota atual
for raio in 10 15 30; do
  rodar m1 atual $raio 0.2
  rodar fleet atual $raio 0
done

# 2) frotas 5a e 5b
for frota in 5a 5b; do
  for raio in 10 15 30; do
    rodar m1 $frota $raio 0.2
    rodar fleet $frota $raio 0
  done
done

# 3) outros lambdas na frota atual
for lam in 0 0.5 1 2; do
  for raio in 10 15 30; do
    rodar m1 atual $raio $lam
  done
done
