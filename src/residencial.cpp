#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(10, 3, 4, 5, 6, 7);
Servo doorLocker; //Criação de um Motor Servo

//Para Comunicação
const char R_ABERTO = 'a';
const char R_FECHADO = 'f';

//Indica o término da mensagem
const char G_TERMINOU = '#';
//Indica que não houve mudança de status naquele arduino
const char G_SEM_ALTERACAO = '&';

//Pino do Botão
const int pinButton = 2;
//Pino da Relé das Lampadas
const int pinLight = 8;
//Pino do Motor Servo(Porta)
const int pinDoor = 9;

//Variavel de Abertura(1) ou Fechamento(0) da Lavanderia
volatile int isOn = 0;

volatile boolean mudouStatus = false;

void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);

    pinMode(pinButton, INPUT_PULLUP);
    pinMode(pinLight, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(pinButton), lockOrOpen, FALLING);

    Wire.begin(3);
    Wire.onRequest(requestEvent);

    doorLocker.attach(pinDoor);

    lockOrOpen();
}

void loop()
{
    delay(250);
}

void requestEvent()
{
    String msg;
    if (!mudouStatus)
    {
        msg = String(G_SEM_ALTERACAO);
        char buffer[32];
        msg.toCharArray(buffer, 32);
        Wire.write(buffer);
        return;
    }
    mudouStatus = false;
    msg = isOn == HIGH ? String(R_ABERTO) : String(R_FECHADO);

    msg = msg + String(G_TERMINOU);
    char buffer[32];
    msg.toCharArray(buffer, 32);
    Wire.write(buffer);
}

void lockOrOpen()
{
    isOn = !isOn;
    //Controle da Porta e Luz.
    //Caso isOn seja 1, Liga a Luz e Abre a Porta.
    //Caso isOn seja 0, Desliga a Luz e Fecha a Porta.
    lcd.clear();
    lcd.setCursor(0, 0);
    if (isOn == HIGH)
    {
        digitalWrite(pinLight, HIGH); //Luz Ligada
        doorLocker.write(0);          //Porta Aberta
        lcd.print("Aberto");
    }
    else
    {
        digitalWrite(pinLight, LOW); //Luz Desligada
        doorLocker.write(90);        //Porta Fechada
        lcd.print("Fechado");
    }
    mudouStatus = true;
}