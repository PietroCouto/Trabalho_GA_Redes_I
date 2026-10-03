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

O código compilado pode ser movido para qualquer local e executado, desde que a estrutura do diretório atenda:

```
├── config
│   └── config.txt
├── main
└── tmp
```

O código foi desenvolvido para sistemas Linux. Em Windows falhará devido a peculiaridades de arquivos, bem como a função clear_screen().

# O Projeto

## main.cpp
O código principal do projeto pode ser encontrado em ./code/main.cpp. Nele está localizada a integração entre as demais bibliotecas 
desenvolvidas e utilizadas, bem como a lógica do protocolo utilizado.

## config_handler.cpp
Em ./code/libs/config_handler.cpp está o primeiro módulo desenvolvido para esse projeto. É basicamente um conjunto de funções responsáveis por abrir 
e ler as configurações das máquinas definidas em ./config/config.txt.

## server.hpp
Em ./code/libs/server.hpp está o coração da aplicação. Todas as funcionalidades relacionadas a redes encontram-se nesse arquivo. Nele é possível encontrar 
a fila responsável por armazenar os pacotes que chegam, a criação do socket e a capacidade de enviar mensagens para outros endereços.


## filesystem_handler.hpp
Em ./code/libs/filesystem_handler.hpp podem ser encontradas todas as funcionalidades relacionadas ao diretório ./tmp. Isso inclui o watcher do diretório, a 
fila de mudanças e as funções e estruturas responsáveis por quebrar os arquivos em pacotes e remontá-los.

## utils.hpp
Em ./code/libs/utils.hpp estão três funções simples. A finalidade é apenas deixar os demais arquivos mais limpos, separando essas funções.