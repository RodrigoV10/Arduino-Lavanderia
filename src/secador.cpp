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
//Operações de Secagem
//################################
const byte OP_10MIN = 0; //10 Segundos
const byte OP_15MIN = 1; //15 Segundos
const byte OP_30MIN = 2; //30 Segundos
const byte OP_45MIN = 3; //45 Segundos

const byte ESTADO_DISPONIVEL = 0;
const byte ESTADO_ESPERA = 1;
const byte ESTADO_SEL_SECAGEM = 2;
const byte ESTADO_PRE_OPERANDO = 3;
const byte ESTADO_OPERANDO = 3;
const byte ESTADO_PAUSA = 4;
const byte ESTADO_FINALIZADO = 5;
const byte ESTADO_PRE_CANCELADO = 6;
const byte ESTADO_CANCELADO = 7;

String listaOperacoes[] = {"10 Minutos", "15 Minutos", "30 Minutos", "45 Minutos"};

byte modoOperacao = OP_10MIN;
byte tempoOperacao = 10;

boolean abertaFechada = true;

byte contador = 0;
byte rotacaoMotorMax = 10;

volatile byte statusAtualSecagem = ESTADO_DISPONIVEL;

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

    Wire.begin(6);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);

    inicio();
}

void loop()
{
    if (statusAtualSecagem == ESTADO_FINALIZADO)
    {
        delay(5000);
        lcd.clear();
        inicio();
    }
    if (statusAtualSecagem == ESTADO_CANCELADO)
    {
        delay(2000);
        lcd.clear();
        inicio();
    }
    if (statusAtualSecagem == ESTADO_OPERANDO)
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
    if (statusAtualSecagem == ESTADO_PRE_CANCELADO)
    {
        desligaMotor();
        cancelarProcesso();
    }

    if (statusAtualSecagem == ESTADO_PRE_OPERANDO)
    {
        delay(100);
        ligaMotor();
        lcd.clear();
        statusAtualSecagem = ESTADO_OPERANDO;
    }
    //Faz a validação da loja aberta ou fechada
    if (statusAtualSecagem == ESTADO_DISPONIVEL && trocarStatusAbertaFechada)
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
    statusAtualSecagem = ESTADO_DISPONIVEL;
}

void encerrarAtividades()
{
    lcd.clear();
    desligado = true;
}

void inicializaModoEspera()
{
    statusAtualSecagem = ESTADO_ESPERA;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aguardando");
}

void inicializaModoSecagem()
{
    statusAtualSecagem = ESTADO_SEL_SECAGEM;
    modoOperacao = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Modo");
    lcd.setCursor(0, 1);
    lcd.print(listaOperacoes[modoOperacao]);
}

void inicializaSecagem()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Iniciando...");
    contador = 0;
    switch (modoOperacao)
    {
    case OP_10MIN:
        tempoOperacao = 10;
        break;
    case OP_15MIN:
        tempoOperacao = 15;
        break;
    case OP_30MIN:
        tempoOperacao = 30;
        break;
    case OP_45MIN:
        tempoOperacao = 45;
        break;
    }
    statusAtualSecagem = ESTADO_PRE_OPERANDO;
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
    statusAtualSecagem = ESTADO_CANCELADO;
}

void pausarProcesso(boolean pausar)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(String(contador) + " min");
    if (pausar)
    {
        statusAtualSecagem = ESTADO_PAUSA;
        desligaMotor();
        lcd.setCursor(0, 1);
        lcd.print("Aguardando");
    }
    else
    {
        ligaMotor();
        statusAtualSecagem = ESTADO_OPERANDO;
    }
}

void finalizarProcesso()
{
    statusAtualSecagem = ESTADO_FINALIZADO;
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
    switch (statusAtualSecagem)
    {
    case ESTADO_ESPERA: //Inicia a operação de espera
        inicializaModoSecagem();
        break;
    case ESTADO_SEL_SECAGEM: //Inicia a secagem
        inicializaSecagem();
        break;
    case ESTADO_OPERANDO: //Cancela o processo
        statusAtualSecagem = ESTADO_PRE_CANCELADO;
        break;
    }
}
//Método ao pressionar o botão de pausar a secagem
void pressPause()
{
    if (desligado)
    {
        return;
    }
    switch (statusAtualSecagem)
    {
    case ESTADO_SEL_SECAGEM: //Inicia a operação de espera
        modoOperacao++;
        if (modoOperacao >= (sizeof(listaOperacoes) / sizeof(listaOperacoes[0])))
        {
            modoOperacao = 0;
        }
        trocarLista(listaOperacoes, modoOperacao, "Modo");
        break;
    case ESTADO_OPERANDO: //Pausa o processo
        pausarProcesso(true);
        break;
    case ESTADO_PAUSA: //Retoma o processo o processo
        pausarProcesso(false);
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
    if (statusAtualSecagem != ESTADO_DISPONIVEL)
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