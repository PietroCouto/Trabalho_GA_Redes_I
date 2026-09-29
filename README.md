# Trabalho do Grau A
**Disciplina:** Redes de Computadores: Aplicação e Transporte

**Nome:** Pietro do Couto Freitas

### Como rodar com Docker
Para rodar o projeto utilizando o Docker, basta baixá-lo, acessar a pasta docker e executar:

`docker compose up --build`

### Rodando apenas um peer
Na hipótese de rodar apenas um peer, o passso a passo é um pouco diferente.

PS: Ao utilizar o VSCode, já existe uma automação e debugger configurados.

1 - Acessar ./config/config.txt e configurar:

```
MACHINE nome_1
ADDRESS ip_1
PORT porta_1

MACHINE nome_2
ADDRESS ip_2
PORT porta_3
```

Substituindo nome, ip e porta de acordo com a configuração desejada. N máquinas podem ser adicionadas seguindo essa estrutura.

2 - Compilar na raiz do projeto:

`g++ -fdiagnostics-color=always -g ./code/main.cpp -o ./code/compiled/main`

3 - Executar na raiz do projeto:

`./code/compiled/main nome_da_maquina`

O nome da máquina deve ser passado para que o programa encontre-a no arquivo de configuração.
