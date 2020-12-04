#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define PinoAcao 2
#define PinoTroca 3

LiquidCrystal lcd(11, 9, 4, 5, 6, 7);

//Indica o término da mensagem
const char G_TERMINOU = '#';
//Indica que não houve mudança de status naquele arduino
const char G_SEM_ALTERACAO = '&';
//Código recebido quando um novo pedido for realizado
const char P_PEDIDO = 'p';
//Código que solicita uma máquina de lavar
const char PP_LAVAGEM = 'l';
//Código que solicita uma máquina de secar
const char PP_SECAGEM = 's';
//Código que solicita ambas as máquinas, lava e seca
const char PP_LAVASECA = 't';
//Recebido quando a lavanderia estiver aberta
const char R_ABERTO = 'a';
//Recebido quando a lavanderia estiver fechada
const char R_FECHADO = 'f';
//Respostas quanto a disponibilidade de máquinas
const char P_SIM = 'y';
const char P_NAO = 'n';
//################################
//Códigos de Espera
//################################
const byte ES_NADA = 0;
const byte ES_MODO = 1;
const byte ES_NPED = 2;
//################################
//Operações
//################################
const byte OP_LAVAGEM = 0;
const byte OP_SECAGEM = 1;
const byte OP_LAVASEC = 2;
const byte OP_CANCELA = 3;
//################################
//Pagamentos
//################################
const byte PG_DINHEI = 0;
const byte PG_CARTAO = 1;
const byte PG_OUTROS = 2;
const byte PG_CANCEL = 3;

const byte PEDIDO_NAO_INICIADO = 0;
const byte PEDIDO_EM_ESPERA = 1;
const byte PEDIDO_SEL_MODO_OPERACAO = 2;
const byte PEDIDO_SEL_PAGAMENTO = 3;
const byte PEDIDO_FINALIZADO = 4;
const byte PEDIDO_CANCELADO = 5;
const byte PEDIDO_MODO_OP_INDISPONIVEL = 6;

String listaOperacoes[] = {"Lavagem", "Secagem", "Lava/Seca", "Cancelar"};
String listaPagamentos[] = {"Cartao", "Dinheiro", "Outro", "Cancelar"};

byte modoOperacao = OP_LAVAGEM;
byte modoPagamento = PG_DINHEI;
byte codEsperaAtual = ES_NADA;
boolean abertaFechada = true;
//Define que recebeu um evento que abertua ou fechamendo da loja
boolean trocarStatusAbertaFechada = false;
//Define quando o arduino deve ficar no modo inativo, bloqueando os botões
boolean desligado = false;

volatile byte statusAtualPedido = PEDIDO_NAO_INICIADO;

void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.clear();

    pinMode(PinoAcao, INPUT_PULLUP);
    pinMode(PinoTroca, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PinoAcao), pressButtonAcao, FALLING);
    attachInterrupt(digitalPinToInterrupt(PinoTroca), pressButtonTroca, FALLING);

    Wire.begin(2);

    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);

    inicio();
}

void loop()
{
    if (statusAtualPedido == PEDIDO_FINALIZADO)
    {
        delay(5000);
        lcd.clear();
        inicio();
    }
    if (statusAtualPedido == PEDIDO_CANCELADO)
    {
        delay(2000);
        lcd.clear();
        inicio();
    }
    if (statusAtualPedido == PEDIDO_MODO_OP_INDISPONIVEL)
    {
        delay(2000);
        lcd.clear();
        inicializaModoOperacao();
    }
    //Faz a validação da loja aberta ou fechada
    if (statusAtualPedido == PEDIDO_NAO_INICIADO && trocarStatusAbertaFechada)
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
    delay(500);
}

//################################
//Funções Principais
//################################
void inicio()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bem Vindo");
    desligado = false;
    statusAtualPedido = PEDIDO_NAO_INICIADO;
}

void encerrarAtividades()
{
    lcd.clear();
    desligado = true;
}

void inicializaModoOperacao()
{
    statusAtualPedido = PEDIDO_SEL_MODO_OPERACAO;
    modoOperacao = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Operacao");
    lcd.setCursor(0, 1);
    lcd.print(listaOperacoes[modoOperacao]);
}

boolean validaOperacao()
{
    statusAtualPedido = PEDIDO_EM_ESPERA;
    if (modoOperacao == OP_CANCELA)
    {
        return false;
    }
    codEsperaAtual = ES_MODO;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aguarde...");
    return true;
}

void inicializaModoPagamento()
{
    statusAtualPedido = PEDIDO_SEL_PAGAMENTO;
    modoPagamento = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pagamento");
    lcd.setCursor(0, 1);
    lcd.print(listaPagamentos[modoPagamento]);
}

boolean validaPagamento()
{
    statusAtualPedido = PEDIDO_EM_ESPERA;
    if (modoPagamento == PG_CANCEL)
    {
        return false;
    }
    codEsperaAtual = ES_NPED;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aguarde...");
    return true;
}

void cancelarProcesso()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cancelado");
    statusAtualPedido = PEDIDO_CANCELADO;
}

void finalizarProcesso(String msg)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    switch (msg.charAt(1)) //Modo que foi requisitado
    {
    case PP_LAVAGEM:
        lcd.print("Lavadora " + String(msg.charAt(2)));
        break;

    case PP_SECAGEM:
        lcd.print("Secadora " + String(msg.charAt(2)));
        break;

    case PP_LAVASECA:
        lcd.print("Lavadora " + String(msg.charAt(2)));
        lcd.setCursor(0, 1);
        lcd.print("Secadora " + String(msg.charAt(3)));
        break;

    default:
        cancelarProcesso();
        break;
    }
    statusAtualPedido = PEDIDO_FINALIZADO;
}

void validarDisponibilidadeMaquinas(String msg)
{
    //Se o segundo caracter for "SIM", então não tem
    //indisponibilidade na solicitação e pode seguir
    if (msg.charAt(0) == P_SIM)
    {
        inicializaModoPagamento();
        return;
    }

    //Escape para indicar problema na comunicação, pois
    //se não for sim ou não então não há o que fazer
    if (msg.charAt(0) != P_NAO)
    {
        cancelarProcesso();
    }

    //Se tiver máquinas indisponíveis
    lcd.clear();
    lcd.setCursor(0, 0);
    switch (msg.charAt(1))
    {
    case PP_LAVAGEM:
        lcd.print("Lavagem Indisp.");
        break;
    case PP_SECAGEM:
        lcd.print("Secagem Indisp.");
        break;
    case PP_LAVASECA:
        lcd.print("Lavagem Indisp.");
        lcd.setCursor(0, 1);
        lcd.print("Secagem Indisp.");
        break;
    }
    statusAtualPedido = PEDIDO_MODO_OP_INDISPONIVEL;
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
void pressButtonAcao()
{
    if (desligado)
    {
        return;
    }
    switch (statusAtualPedido)
    {
    case PEDIDO_NAO_INICIADO:
        inicializaModoOperacao();
        break;
    case PEDIDO_SEL_MODO_OPERACAO:
        if (!validaOperacao())
        {
            cancelarProcesso();
        }
        break;
    case PEDIDO_SEL_PAGAMENTO:
        if (!validaPagamento())
        {
            cancelarProcesso();
        }
        break;
    }
}

void pressButtonTroca()
{
    if (desligado)
    {
        return;
    }

    switch (statusAtualPedido)
    {
    case PEDIDO_SEL_MODO_OPERACAO:
        modoOperacao++;
        if (modoOperacao >= (sizeof(listaOperacoes) / sizeof(listaOperacoes[0])))
        {
            modoOperacao = 0;
        }
        trocarLista(listaOperacoes, modoOperacao, "Operacao");
        break;
    case PEDIDO_SEL_PAGAMENTO:
        modoPagamento++;
        if (modoPagamento >= (sizeof(listaPagamentos) / sizeof(listaPagamentos[0])))
        {
            modoPagamento = 0;
        }
        trocarLista(listaPagamentos, modoPagamento, "Pagamento");
        break;
    }
}

//################################
//Funções de Eventos
//################################
//Função chamada quando o mestre solicita uma mensagem
void requestEvent()
{
    String msg;
    if (statusAtualPedido != PEDIDO_EM_ESPERA)
    {
        msg = String(G_SEM_ALTERACAO);
        char buffer[32];
        msg.toCharArray(buffer, 32);
        Wire.write(buffer);
        return;
    }
    switch (codEsperaAtual)
    {
    case ES_MODO: //Pergunta sobre os status da máquinas
        switch (modoOperacao)
        {
        case OP_LAVAGEM:
            msg = String(PP_LAVAGEM);
            break;
        case OP_SECAGEM:
            msg = String(PP_SECAGEM);
            break;
        case OP_LAVASEC:
        default:
            msg = String(PP_LAVASECA);
            break;
        }
        msg = msg + String(G_TERMINOU);
        codEsperaAtual = ES_NADA;
        break;
    case ES_NPED: //Realiza um novo pedido
        msg = String(P_PEDIDO);
        switch (modoOperacao)
        {
        case OP_LAVAGEM:
            msg = msg + String(PP_LAVAGEM);
            break;
        case OP_SECAGEM:
            msg = msg + String(PP_SECAGEM);
            break;
        case OP_LAVASEC:
        default:
            msg = msg + String(PP_LAVASECA);
            break;
        }
        msg = msg + String(G_TERMINOU);
        codEsperaAtual = ES_NADA;
        break;
    default:
        msg = String(G_SEM_ALTERACAO);
        break;
    }
    char buffer[32];
    msg.toCharArray(buffer, 32);
    Wire.write(buffer);
}

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
    case P_PEDIDO: //Para exibir na tela as máquinas alugadas
        finalizarProcesso(msg);
        break;
    case P_SIM: //Respostas quando a disponibilidade de
    case P_NAO: //máquinas de lavar e secar
        validarDisponibilidadeMaquinas(msg);
        break;
    }
}
