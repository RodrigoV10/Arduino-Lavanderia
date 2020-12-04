# Projeto Embarcado de Integração de Arduinos para Sistema de Lavanderia

Esse projeto reúne os arquivos necessários para integração de diferentes arduinos a fim de criar um sistema de lavanderia automatizado. O projeto é composto por seis diferentes scripts, sendo cada um para direcionado para um arduino diferente, que irá funções diferentes.

O projeto original foi elaborado utilizando a plataforma [Tinkercad](www.tinkercad.com), cujo arquivo `.brd` pode ser encontrado nesse repositório.

## Funções e Módulos

Existem cinco diferentes funções para cada script, sendo que um deles é duplicado, representando uma cópia de uma máquina de lavagem.

### Arduino de Comunicação

Essa placa é responsável por realizar a comunicação entre os diversos arduinos, portanto ela também será a placa mestre na comunicação [I2C](www.arduino.cc/en/reference/wire).
### Arduino de Automação Residencial

Responsável por automatizar o ambiente. No projeto foi escolhido representar o controle de abertura e fechamento da porta e controle de duas luzes.
### Arduino de Auto Atendimento

Realiza as funções de pagamento e "aluguel" das máquinas. É o primeiro contato com o cliente.

### Arduinos de Secagem e Lavagem

Semelhantes, porém com algumas particularidades. A controladora da máquina de secagem permite interromper a operação para simular a retirada de roupas e voltar com a operação normalmente.

Ambos os tipos de placas permitem e seleção de modos de operação, assim como o cancelamento do processo.

## Códigos de Comunicação
* #: Indica o término da mensagem.
* $: Indica que não houve mudança de estado naquela placa.
* a: Abertura da lavanderia.
* f: Fechamento da lavanderia.
* p: Novo pedido realizado. Deve ser seguido o tipo de serviço solicitado.
* l: Solicita um serviço de lavagem.
* s: Solicita um serviço de secagem.
* t: Solicita um serviço de lava e seca.
* y: Sinaliza booleano “sim”.
* n: Sinaliza um booleano “não”.
* d: Sinaliza o comunicador que determinado serviço está disponível.

# Sobre

Esse projeto foi elaborado para a disciplina de *Sistemas Embarcados e Tempo Real* da faculdade de Ciência da Computação na universidade [Unijuí](www.unijui.edu.br/).

## Autores

* Rodrigo Pillar Vianna
* Leonardo Zawatski Berton
* William Micael Hertz
* Guilherme Crestani
* Marcos José Kuchak Filho