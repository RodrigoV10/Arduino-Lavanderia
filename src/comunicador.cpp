#include <Arduino.h>
#include <Wire.h>

//################################
//Arduino de Pagamentos
//################################
//Códigos de quando cada tipo de máquina estiver disponível
const char P_LAVAGEM_DISP = 'l';
const char P_SECAGEM_DISP = 's';
const char P_LAVASEC_DISP = 't';
//Código recebido quando um novo pedido for realizado
const char P_PEDIDO = 'p';
//Código que solicita uma máquina de lavar
const char PP_LAVAGEM = 'l';
//Código que solicita uma máquina de secar
const char PP_SECAGEM = 's';
//Código que solicita ambas as máquinas, lava e seca
const char PP_LAVASECA = 't';

//################################
//Arduino de Lavagem/Secagem
//################################
//Identificadores de cada máquina
const char LAVADOR_1 = '1';
const char LAVADOR_2 = '2';
const char SECADOR_1 = '1';
//Definição de cada status
const byte LIBERADO = 0;
const byte ESPERA_ATIVO = 1; //Para o comunicador não importa se a máquina está em espera ou em funcionamento
//Status de cada máquina. É inicializado como liberada;
byte statusLavadora1 = LIBERADO;
byte statusLavadora2 = LIBERADO;
byte statusSecadora = LIBERADO;
//Código enviada pela máquina quando estiver livre
const char LS_LIBERADO = 'd';

//################################
//Arduino de Auto-Residencial
//################################
//Recebido quando a lavanderia estiver aberta
const char R_ABERTO = 'a';
//Recebido quando a lavanderia estiver fechada
const char R_FECHADO = 'f';
//Esses códigos serão enviados para outros arduinos para indicar a
//abertura e fechamento da lavanderia

//################################
//Arduino de Comunicador
//################################
//Código gerais que serão enviados para cada arduino
//------------------Pagamento------------------
//Resposta que será enviada para a máquina de pagamento, quando for perguntado
//se determinadas máquinas estão livres
const char P_SIM = 's';
const char P_NAO = 'n';
//------------------Lava/Seca------------------
//Indica para determinada máquina para que ela entre em modo espera
const char LS_MODO_ESPERA = 'p';

//################################
//Endereços Wire
//################################
const byte END_PAGAMENTO = 2;
const byte END_RESIDENCIAL = 3;
const byte END_LAVADORA1 = 4;
const byte END_LAVADORA2 = 5;
const byte END_SECADORA = 6;

//################################
//Outros Códigos
//################################
//Indica o término da mensagem
const char G_TERMINOU = '#';
//Indica que não houve mudança de status naquele arduino
const char G_SEM_ALTERACAO = '&';

//################################
//Outros
//################################
//Representa o número máximo de bytes que serão lidos
const byte BYTES_MENSAGEM = 6;
//Representa o LED do arduino que será usado para notificar o recebimento de mensagens
const int LED_LOG = 13;

void setup()
{
  Serial.begin(9600);
  pinMode(LED_LOG, OUTPUT);
  Wire.begin();
  delay(1000);
}

void loop()
{
  String msg;
  msg = verificaAlteracoes(END_RESIDENCIAL);
  funcResidencial(msg);
  delay(100);

  msg = verificaAlteracoes(END_SECADORA);
  funcSecador(msg);
  delay(100);

  msg = verificaAlteracoes(END_LAVADORA1);
  funcLavador(msg);
  delay(100);

  msg = verificaAlteracoes(END_LAVADORA2);
  funcLavador(msg);
  delay(100);

  msg = verificaAlteracoes(END_PAGAMENTO);
  funcPagador(msg);
  delay(100);

  delay(350);
}

void logLed()
{
  digitalWrite(LED_LOG, HIGH);
  delay(150);
  digitalWrite(LED_LOG, LOW);
}
////////////////////////////////////////////
//Funções Principais
////////////////////////////////////////////
//Função que verifica se houve mudança de estado no arduino, de acordo com o endereço passado por parâmetro
String verificaAlteracoes(byte endereco)
{
  Wire.requestFrom(endereco, BYTES_MENSAGEM);
  String msg = "";
  while (Wire.available())
  {
    char c = Wire.read();
    if (c == G_SEM_ALTERACAO)
    {
      return "";
    }
    if (c == G_TERMINOU)
    {
      break;
    }
    msg = msg + c;
  }
  Wire.flush();
  return msg;
}

//Função responsável por tratar mensagens recebidas pelo arduino de pagamento
void funcPagador(String msg)
{
  if (msg == NULL || msg.length() < 1)
  {
    return;
  }
  logLed();
  switch (msg.charAt(0))
  {
  //Nesta seção trata a mensagem quando é pedido para verificar
  //se existe uma máquina de lavar disponível
  case P_LAVAGEM_DISP:
    pgValidaLavagem();
    break;
  //Nesta seção trata a mensagem quando é pedido para verificar
  //se existe uma máquina de secar disponível
  case P_SECAGEM_DISP:
    pgValidaSecagem();
    break;
  //Nesta seção trata a mensagem quando é pedido para verificar
  //se existe uma máquina de lavar e uma de secar disponível
  case P_LAVASEC_DISP:
    pgValidaLavaSec();
    break;
  //Nesta seção trata a mensagem quando é feito um novo pedido
  case P_PEDIDO:
    pgNovoPedido(msg);
    break;
  }
}

//Função responsável por tratar mensagens recebidas pelas máquinas de lavar
void funcLavador(String msg)
{
  if (msg == NULL || msg.length() < 1)
  {
    return;
  }
  logLed();
  byte maquinaIdentificada = 0;
  switch (msg.charAt(0))
  {
  case LAVADOR_1:
    maquinaIdentificada = END_LAVADORA1;
    break;
  case LAVADOR_2:
    maquinaIdentificada = END_LAVADORA2;
    break;
  }

  //Se foi identificada a máquina e a mensagem é de liberação
  //altera o status da máquina
  if (maquinaIdentificada != 0 && msg.charAt(1) == LS_LIBERADO)
  {
    if (maquinaIdentificada == END_LAVADORA1)
    {
      statusLavadora1 = LIBERADO;
    }
    else if (maquinaIdentificada == END_LAVADORA2)
    {
      statusLavadora2 = LIBERADO;
    }
  }
}

//Função responsável por tratar mensagens recebidas pelas máquinas de secar
void funcSecador(String msg)
{
  if (msg == NULL || msg.length() < 1)
  {
    return;
  }
  logLed();
  byte maquinaIdentificada = 0;
  switch (msg.charAt(0))
  {
  case SECADOR_1:
    maquinaIdentificada = END_SECADORA;
    break;
  }

  //Se foi identificada a máquina e a mensagem é de liberação
  //altera o status da máquina
  if (maquinaIdentificada != 0 && msg.charAt(2) == LS_LIBERADO)
  {
    if (maquinaIdentificada == SECADOR_1)
    {
      statusSecadora = LIBERADO;
    }
  }
}

//Função responsável por tratar mensagens recebidas pelo arduino
//de automação residencial
void funcResidencial(String msg)
{
  if (msg == NULL || msg.length() < 1)
  {
    return;
  }
  logLed();
  //Verifica qual a mensagem recebida e seleciona qual
  //código deve ser repassado para os arduinos
  char estadoDaLoja;
  switch (msg.charAt(0))
  {
  case R_ABERTO:
    estadoDaLoja = R_ABERTO;
    break;
  case R_FECHADO:
    estadoDaLoja = R_FECHADO;
    break;
  }

  //Se o código for nulo, sai do método
  if (estadoDaLoja == NULL)
  {
    return;
  }

  String msgEstadoLoja = String(estadoDaLoja) + String(G_TERMINOU);
  char buffer[32];
  msgEstadoLoja.toCharArray(buffer, 32);

  //Avisa o arduino de pagamentos
  Wire.beginTransmission(END_PAGAMENTO);
  Wire.write(buffer);
  Wire.endTransmission();

  //Avisa os arduinos de lavagem
  Wire.beginTransmission(END_LAVADORA1);
  Wire.write(buffer);
  Wire.endTransmission();

  Wire.beginTransmission(END_LAVADORA2);
  Wire.write(buffer);
  Wire.endTransmission();

  //Avisa o arduino de secagem
  Wire.beginTransmission(END_SECADORA);
  Wire.write(buffer);
  Wire.endTransmission();
}

////////////////////////////////////////////
//Funções Auxiliares
////////////////////////////////////////////
void pgValidaLavagem()
{
  String resposta = "";
  if (statusLavadora1 == LIBERADO || statusLavadora2 == LIBERADO)
  {
    resposta = String(P_SIM);
  }
  else
  {
    resposta = String(P_NAO) + String(PP_LAVAGEM);
  }
  resposta = resposta + String(G_TERMINOU);
  char buffer[32];
  resposta.toCharArray(buffer, 32);

  Wire.beginTransmission(END_PAGAMENTO);
  Wire.write(buffer);
  Wire.endTransmission();
}

void pgValidaSecagem()
{
  String resposta = "";
  if (statusSecadora == LIBERADO)
  {
    resposta = String(P_SIM);
  }
  else
  {
    resposta = String(P_NAO) + String(PP_SECAGEM);
  }
  resposta = resposta + String(G_TERMINOU);
  char buffer[32];
  resposta.toCharArray(buffer, 32);

  Wire.beginTransmission(END_PAGAMENTO);
  Wire.write(buffer);
  Wire.endTransmission();
}

void pgValidaLavaSec()
{
  String resposta = "";
  if (statusSecadora == LIBERADO && (statusLavadora1 == LIBERADO || statusLavadora2 == LIBERADO))
  {
    resposta = String(P_SIM);
  }
  else if (statusSecadora == ESPERA_ATIVO && statusLavadora1 == ESPERA_ATIVO && statusLavadora2 == ESPERA_ATIVO)
  {
    resposta = String(P_NAO) + String(PP_LAVASECA);
  }
  else if (statusSecadora == ESPERA_ATIVO)
  {
    resposta = String(P_NAO) + String(PP_SECAGEM);
  }
  else
  {
    resposta = String(P_NAO) + String(PP_LAVAGEM);
  }
  resposta = resposta + String(G_TERMINOU);
  char buffer[32];
  resposta.toCharArray(buffer, 32);

  Wire.beginTransmission(END_PAGAMENTO);
  Wire.write(buffer);
  Wire.endTransmission();
}

void pgNovoPedido(String msg)
{
  byte lavadoraSelecionada;
  byte secadoraSelecionada;
  char lavadoraSelecionadaChar;
  char secadoraSelecionadaChar;
  char modoRequisitado;
  //Verifica qual tipo de pedido foi realizado, armazenando os endereços os
  //identificadores de cada máquina
  switch (msg.charAt(1))
  {
  case PP_LAVAGEM:
    if (statusLavadora1 == LIBERADO)
    {
      lavadoraSelecionada = END_LAVADORA1;
      lavadoraSelecionadaChar = LAVADOR_1;
      statusLavadora1 = ESPERA_ATIVO;
    }
    else
    {
      lavadoraSelecionada = END_LAVADORA2;
      lavadoraSelecionadaChar = LAVADOR_2;
      statusLavadora2 = ESPERA_ATIVO;
    }
    modoRequisitado = PP_LAVAGEM;
    break;
  case PP_SECAGEM:
    secadoraSelecionada = END_SECADORA;
    secadoraSelecionadaChar = SECADOR_1;
    modoRequisitado = PP_SECAGEM;
    statusSecadora = ESPERA_ATIVO;
    break;
  case PP_LAVASECA:
    if (statusLavadora1 == LIBERADO)
    {
      lavadoraSelecionada = END_LAVADORA1;
      lavadoraSelecionadaChar = LAVADOR_1;
      statusLavadora1 = ESPERA_ATIVO;
    }
    else
    {
      lavadoraSelecionada = END_LAVADORA2;
      lavadoraSelecionadaChar = LAVADOR_2;
      statusLavadora2 = ESPERA_ATIVO;
    }
    secadoraSelecionada = END_SECADORA;
    secadoraSelecionadaChar = SECADOR_1;
    modoRequisitado = PP_LAVASECA;
    statusSecadora = ESPERA_ATIVO;
    break;
  }

  //Notifica as máquinas para entrarem em modo espera
  String msgRetorno = String(P_PEDIDO) + String(modoRequisitado);
  if (lavadoraSelecionada != NULL && lavadoraSelecionadaChar != NULL)
  {
    msgRetorno = msgRetorno + String(lavadoraSelecionadaChar);
    String msgBuffer = String(LS_MODO_ESPERA) + String(G_TERMINOU);
    char buffer[32];
    msgBuffer.toCharArray(buffer, 32);
    Wire.beginTransmission(lavadoraSelecionada);
    Wire.write(buffer);
    Wire.endTransmission();
  }

  if (secadoraSelecionada != NULL && secadoraSelecionadaChar != NULL)
  {
    msgRetorno = msgRetorno + String(secadoraSelecionadaChar);
    String msgBuffer = String(LS_MODO_ESPERA) + String(G_TERMINOU);
    char buffer[32];
    msgBuffer.toCharArray(buffer, 32);
    Wire.beginTransmission(secadoraSelecionada);
    Wire.write(buffer);
    Wire.endTransmission();
  }

  //Notifica o arduino de pagamentos sobre as máquinas alugadas
  char buffer[32];
  msgRetorno.toCharArray(buffer, 32);
  Wire.beginTransmission(END_PAGAMENTO);
  Wire.write(buffer);
  Wire.endTransmission();
}