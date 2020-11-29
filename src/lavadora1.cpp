#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define pinButtonStart 2
#define motorMaquina 10

LiquidCrystal lcd(11, 3, 4, 5, 6, 7);

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

volatile boolean buttonPressed = false;

boolean abertaFechada = true;
boolean disponivel = true;
boolean emEspera = false;
boolean emUso = false;
boolean lcdDisponivel = true;
boolean ligarMaquina = false;
boolean motorLigado = true;

//Define que recebeu um evento que abertua ou fechamendo da loja
boolean trocarStatusAbertaFechada = false;
//Define quando o arduino deve ficar no modo inativo, bloqueando os botões
boolean desligado = false;

int contador = 0;
int rotacaoMotor = 0;

void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);

    pinMode(pinButtonStart, INPUT_PULLUP);
    pinMode(motorMaquina, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(pinButtonStart), pressButton, FALLING);

    Wire.begin(4);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
}

void loop()
{
    //Apenas se a Lavanderia está aberta
    if (abertaFechada == true)
    {
        //Caso esteja disponivel, exibe na tela
        if (disponivel == true && lcdDisponivel == true)
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Disponivel");
            lcdDisponivel = false;
        }
        //Se a maquina está em espera
        if (emEspera == true)
        {
            //Se pressionar o Botão, Começa a Lavar
            if (buttonPressed == true)
            {
                ligarMaquina = true;
                if (motorLigado == true)
                {
                    ligaMotor();
                    motorLigado = false;
                }
                //Começa o Ciclo de lavagem
                cicloLavagem();
            }
        }
    }
    //Caso a Lavanderia feche, mas ainda esteja em uso, continua lavando.
    if (abertaFechada == false && emUso == true)
    {
        cicloLavagem();
    }
    if (trocarStatusAbertaFechada)
    {
        trocarStatusAbertaFechada = false;
        if (abertaFechada)
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Disponivel");
            lcdDisponivel = false;
            desligado = false;
        }
        else
        {
            lcd.clear();
            desligado = true;
        }
    }

    delay(100);
}
//Método que faz a Maquina Lavar
void cicloLavagem()
{
    if (ligarMaquina == true)
    {
        //Diz que a maquina está em uso.
        emUso == true;
        if (contador < 3)
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Lavando");
            Serial.println("Lavando!");
            contador++;
            delay(1000);
        }
        if (contador > 2)
        {
            contador = 0;
            emEspera = false;
            ligarMaquina = false;
            disponivel = true;
            lcdDisponivel = true;
            buttonPressed = false;
            motorLigado = true;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.println("Finalizada");
            desligaMotor();
            //Diz que a maquina não está em uso.
            emUso = false;
            delay(1000);
        }
    }
}
//Método chamado ao receber solicitação do mestre.
void requestEvent()
{
    String msg;
    //Envia uma mensagem quando está ou não disponível para uso
    if (!disponivel)
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
        emEspera = true;
        disponivel = false;
        break;
    }
}
//Método que liga o motor.
void ligaMotor()
{
    for (rotacaoMotor = 0; rotacaoMotor < 10; rotacaoMotor++)
    {
        analogWrite(motorMaquina, rotacaoMotor);
        delay(10);
    }
}
//Método que desliga o motor.
void desligaMotor()
{
    for (rotacaoMotor = 9; rotacaoMotor >= 0; rotacaoMotor--)
    {
        analogWrite(motorMaquina, rotacaoMotor);
        delay(10);
    }
}
//Método ao pressionar o botão
void pressButton()
{
    buttonPressed = true;
}
