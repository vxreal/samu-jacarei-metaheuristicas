# samu-jacarei-metaheuristicas

Código do artigo sobre localização de ambulâncias do SAMU de Jacareí (SP),
disciplina de Metaheurísticas.

Dados convertidos de https://github.com/LuizWah/GeneticAlgoSAMU (Silva et
al., SBPO 2025, DOI 10.59254/sbpo-2025-212488).

## Requisitos

- g++ (usado: 14.2.0)
- Python 3 com `pulp` e `highspy`, só para o resolvedor exato
  (`pip install pulp highspy`)
- R com o pacote `irace`, só para a calibração

## Rodar um método

Cada método fica em uma pasta: `ga`, `simulatedannealing`, `grasp`,
`buscatabu`, `ils` e `grasptabu`. O programa tem que ser rodado de dentro da
pasta.

```
cd ils
g++ -O2 main.cpp -o programa
./programa m1 atual 15 1
```

Argumentos, nesta ordem:

1. modelo: `m1` ou `fleet`
2. frota: `atual` (1 USA + 3 USB), `5a` (1 USA + 4 USB), `5b` (2 USA + 3 USB)
   ou `6` (2 USA + 4 USB)
3. raio em minutos: `10`, `15` ou `30`
4. seed: número inteiro

Depois deles podem vir parâmetros no formato `chave=valor`. Os valores
calibrados estão em `parametros_calibrados.txt`. Exemplo com eles:

```
./programa m1 atual 15 1 nivel=6
```

A saída é uma linha CSV:

```
metodo,modelo,frota,raio,seed,valor,valor_pct,cobertura_pct,tempo_s,solucao
```

`cobertura_pct` é a cobertura sem o peso da USA. `solucao` lista a quadra de
cada ambulância.

## Avaliar a configuração atual

```
cd atual
g++ -O2 main.cpp -o programa
./programa m1 atual 15 0
```

## Rodar todos os testes

Compila tudo e roda os 6 métodos nos 18 cenários com as seeds de 1 a 10,
usando `parametros_calibrados.txt`. Grava `resultados.csv`.

```
./rodar_todos.sh
./rodar_todos.sh ga grasp   # refaz só esses métodos
```

## Resolvedor exato

```
cd exato
python3 exato.py m1 atual 15          # M1 com peso 0.2
python3 exato.py m1 atual 15 0.5      # outro peso da USA
python3 exato.py fleet 5b 30
./rodar_exato.sh                      # todos os cenários, grava exato.csv
```

## Calibração

Grava `parametros_calibrados.txt`.

```
./calibracao-final/calibrar.sh
./calibracao-final/calibrar.sh ga   # só esse método
```

## Resultados do artigo

- `resultados.csv`: metaheurísticas
- `exato/exato.csv`: resolvedor exato
- `parametros_calibrados.txt` e `calibracao-final/*/irace.log`: calibração
