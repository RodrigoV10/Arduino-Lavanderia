#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define pinButtonStart 2
#define pinButtonPause 3
#define motorMaquina 10

LiquidCrystal lcd(11, 9, 4, 5, 6, 7);

//Código enviada pela máquina quando estiver livre
const char LS_LIBERADO = 'd';
//Indica o término da mensagem
const char G_TERMINOU = '#';
//Indica que não houve mudança de status naquele arduino
//Para o caso das máquinas, serve para indicar indisponilidade
const char G_SEM_ALTERACAO = '&';
//a: Indica que a loja abriu
const char R_ABERTO = 'a';
//f: Indica que a loja fechou
const char R_FECHADO = 'f';
//p: Indica para a máquina para ele entrar em modo espera, pois foi realizado um pedido de uso da máquina.
const char LS_MODO_ESPERA = 'p';

//################################
//Operações de Lavagem
//################################
const byte OP_RAPIDA = 0;
const byte OP_DELICADA = 1;
const byte OP_NORMAL = 2;
const byte OP_CENTRIG = 3;

const byte ESTADO_DISPONIVEL = 0;
const byte ESTADO_ESPERA = 1;
const byte ESTADO_SEL_LAVAGEM = 2;
const byte ESTADO_PRE_OPERANDO = 3;
const byte ESTADO_OPERANDO = 3;
const byte ESTADO_FINALIZADO = 4;
const byte ESTADO_PRE_CANCELADO = 5;
const byte ESTADO_CANCELADO = 6;

String listaOperacoes[] = {"Rapida", "Delicada", "Normal", "Centrifugacao"};

byte modoOperacao = OP_RAPIDA;

byte tempoOperacao = 10;

boolean abertaFechada = true;

byte contador = 0;
byte rotacaoMotorMax = 0;

volatile byte statusAtualLavagem = ESTADO_DISPONIVEL;

//Define que recebeu um evento que abertua ou fechamendo da loja
boolean trocarStatusAbertaFechada = false;
//Define quando o arduino deve ficar no modo inativo, bloqueando os botões
boolean desligado = false;

void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);

    pinMode(pinButtonStart, INPUT_PULLUP);
    pinMode(pinButtonPause, INPUT_PULLUP);
    pinMode(motorMaquina, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(pinButtonStart), pressButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(pinButtonPause), pressPause, FALLING);

    Wire.begin(5);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);

    inicio();
}

void loop()
{
    if (statusAtualLavagem == ESTADO_FINALIZADO)
    {
        delay(5000);
        lcd.clear();
        inicio();
    }
    if (statusAtualLavagem == ESTADO_CANCELADO)
    {
        delay(2000);
        lcd.clear();
        inicio();
    }
    if (statusAtualLavagem == ESTADO_OPERANDO)
    {
        contador++;
        if (contador > tempoOperacao)
        {
            finalizarProcesso();
        }
        else
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(String(contador) + " min");
            delay(1000);
        }
    }
    if (statusAtualLavagem == ESTADO_PRE_CANCELADO)
    {
        desligaMotor();
        cancelarProcesso();
    }

    if (statusAtualLavagem == ESTADO_PRE_OPERANDO)
    {
        delay(100);
        ligaMotor();
        lcd.clear();
        statusAtualLavagem = ESTADO_OPERANDO;
    }
    //Faz a validação da loja aberta ou fechada
    if (statusAtualLavagem == ESTADO_DISPONIVEL && trocarStatusAbertaFechada)
    {
        trocarStatusAbertaFechada = false;
        if (abertaFechada)
        {
            inicio();
        }
        else
        {
            encerrarAtividades();
        }
    }
}

void inicio()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Disponivel");
    desligado = false;
    statusAtualLavagem = ESTADO_DISPONIVEL;
}

void encerrarAtividades()
{
    lcd.clear();
    desligado = true;
}

void inicializaModoEspera()
{
    statusAtualLavagem = ESTADO_ESPERA;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aguardando");
}

void inicializaModoLavagem()
{
    statusAtualLavagem = ESTADO_SEL_LAVAGEM;
    modoOperacao = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Modo");
    lcd.setCursor(0, 1);
    lcd.print(listaOperacoes[modoOperacao]);
}

void inicializaLavagem()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Iniciando...");
    contador = 0;
    switch (modoOperacao)
    {
    case OP_RAPIDA:
        tempoOperacao = 10;
        rotacaoMotorMax = 20;
        break;
    case OP_DELICADA:
        tempoOperacao = 40;
        rotacaoMotorMax = 10;
        break;
    case OP_NORMAL:
        tempoOperacao = 40;
        rotacaoMotorMax = 20;
        break;
    case OP_CENTRIG:
        tempoOperacao = 15;
        rotacaoMotorMax = 25;
        break;
    }
    statusAtualLavagem = ESTADO_PRE_OPERANDO;
}

//Método que liga o motor.
void ligaMotor()
{
    for (int rotacaoMotor = 0; rotacaoMotor < rotacaoMotorMax; rotacaoMotor++)
    {
        analogWrite(motorMaquina, rotacaoMotor);
        delay(10);
    }
}
//Método que desliga o motor.
void desligaMotor()
{
    for (int rotacaoMotor = rotacaoMotorMax; rotacaoMotor >= 0; rotacaoMotor--)
    {
        analogWrite(motorMaquina, rotacaoMotor);
        delay(10);
    }
    analogWrite(motorMaquina, 0);
}

void cancelarProcesso()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cancelado");
    statusAtualLavagem = ESTADO_CANCELADO;
}

void finalizarProcesso()
{
    statusAtualLavagem = ESTADO_FINALIZADO;
    desligaMotor();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Encerrado");
}

//################################
//Funções Auxiliares
//################################
void trocarLista(String lista[], byte index, String texto)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(texto);
    lcd.setCursor(0, 1);
    lcd.print(lista[index]);
}

//################################
//Funções de Botões
//################################
//Método ao pressionar o botão de iniciar
void pressButton()
{
    if (desligado)
    {
        return;
    }
    switch (statusAtualLavagem)
    {
    case ESTADO_ESPERA: //Inicia a operação de espera
        inicializaModoLavagem();
        break;
    case ESTADO_SEL_LAVAGEM: //Inicia a lavagem
        inicializaLavagem();
        break;
    case ESTADO_OPERANDO: //Cancela o processo
        statusAtualLavagem = ESTADO_PRE_CANCELADO;
        break;
    }
}
//Método ao pressionar o botão de pausar a lavagem
void pressPause()
{
    if (desligado)
    {
        return;
    }
    switch (statusAtualLavagem)
    {
    case ESTADO_SEL_LAVAGEM: //Inicia a operação de espera
        modoOperacao++;
        if (modoOperacao >= (sizeof(listaOperacoes) / sizeof(listaOperacoes[0])))
        {
            modoOperacao = 0;
        }
        trocarLista(listaOperacoes, modoOperacao, "Modo");
        break;
    }
}

//################################
//Funções de Eventos
//################################
//Método chamado ao receber solicitação do mestre.
void requestEvent()
{
    String msg;
    //Envia uma mensagem quando está ou não disponível para uso
    if (statusAtualLavagem != ESTADO_DISPONIVEL)
    {
        msg = String(G_SEM_ALTERACAO);
        char buffer[32];
        msg.toCharArray(buffer, 32);
        Wire.write(buffer);
        return;
    }

    msg = String(LS_LIBERADO) + String(G_TERMINOU);
    char buffer[32];
    msg.toCharArray(buffer, 32);
    Wire.write(buffer);
}
//Método chamado ao receber instrução do mestre.
void receiveEvent(int i)
{
    //Mensagem Recebida
    String msg = "";
    while (Wire.available())
    {
        char c = Wire.read();
        if (c == G_SEM_ALTERACAO)
        {
            return;
        }
        if (c == G_TERMINOU)
        {
            break;
        }
        msg = msg + c;
    }
    Wire.flush();

    //Faz o tratamento da mensagem
    switch (msg.charAt(0))
    {
    case R_ABERTO: //Caso receba 'a', Abre a Lavanderia.
        abertaFechada = true;
        trocarStatusAbertaFechada = true;
        break;
    case R_FECHADO: //Caso receba 'f', Fecha a Lavanderia.
        abertaFechada = false;
        trocarStatusAbertaFechada = true;
        break;
    case LS_MODO_ESPERA: //Caso receba 'p', coloca a maquina em espera.
        inicializaModoEspera();
        break;
    }
}