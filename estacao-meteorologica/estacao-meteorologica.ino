#include <DHT.h>         // Biblioteca para o sensor DHT11
#include <SFE_BMP180.h>  // Biblioteca para o sensor BMP180
#include <Wire.h>        // Biblioteca para comunicação I2C

// --- Mapeamento de Pinos ---
#define DHTPIN D4        // Pino digital D3 conectado ao DHT11
#define DHTTYPE DHT11    // Tipo do sensor DHT
#define SENSOR_AR A0     // Pino analógico A0 conectado ao MQ-135
#define SENSOR_CHUVA D5  // Pino digital D5 conectado ao Sensor de Chuva
#define LED_CHUVA D6     // Pino digital D6 para o LED de alerta de chuva
#define BUZZER_CHUVA D7  // Pino digital D7 para o Buzzer Piezo

// --- Objetos ---
DHT dht(DHTPIN, DHTTYPE);
SFE_BMP180 sensorP;

// --- Configurações dos Sensores ---
#define ALTITUDE 110.0

// Parâmetros de calibração do MQ-135
const float m = -0.3376;
const float b = 0.7165;
const float R0 = 10.55;  // Valor de calibração do seu MQ-135

// --- Variáveis Globais ---
float umidade = 0.0;
float temp_dht = 0.0;
double temp_bmp = 0.0;
double pressao_abs = 0.0;
double pressao_relativa = 0.0;
float ppm = 0.0;
int leituraChuva = 0;

// --- Controle de Tempo (millis) ---
unsigned long tempoAnterior = 0;
const unsigned long intervaloLeitura = 3000; 

// --- Função: Leitura DHT11 ---
void lerDHT() {
  umidade = dht.readHumidity();
  temp_dht = dht.readTemperature();
}

// --- Função: Leitura MQ-135 ---
void lerQualidadeAr() {
  float sensor_volt = analogRead(SENSOR_AR) * (3.3 / 1023.0);
  if (sensor_volt > 0) {
    float Rs = ((5.0 * 10.0) / sensor_volt) - 10.0;
    float razao = Rs / R0;
    float ppm_log = (log10(razao) - b) / m;
    ppm = pow(10, ppm_log);
  }
}

// --- Função: Leitura BMP180 ---
void lerPressao() {
  char status = sensorP.startTemperature();
  if (status != 0) {
    delay(status);
    status = sensorP.getTemperature(temp_bmp);
    if (status != 0) {
      status = sensorP.startPressure(3);
      if (status != 0) {
        delay(status);
        status = sensorP.getPressure(pressao_abs, temp_bmp);
        if (status != 0) {
          pressao_relativa = sensorP.sealevel(pressao_abs, ALTITUDE);
        }
      }
    }
  } else {
    Serial.println("[ERRO] Falha na leitura do BMP180.");
  }
}

// --- Função: Leitura e Alarme de Chuva ---
void lerEControlarChuva() {
  leituraChuva = digitalRead(SENSOR_CHUVA);

  if (leituraChuva == LOW) { 
    digitalWrite(LED_CHUVA, HIGH);
    tone(BUZZER_CHUVA, 1500);
  } else { 
    // Sem chuva
    digitalWrite(LED_CHUVA, LOW);
    noTone(BUZZER_CHUVA); 
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==============================================");
  Serial.println("   ESTAÇÃO METEOROLÓGICA - TESTE LOCAL (OFFLINE)");
  Serial.println("==============================================");

  dht.begin();
  
  if (sensorP.begin()) {
    Serial.println("[OK] Sensor BMP180 inicializado.");
  } else {
    Serial.println("[ERRO] Falha ao inicializar o sensor BMP180!");
  }

  // Configuração dos Pinos
  pinMode(SENSOR_AR, INPUT);
  pinMode(SENSOR_CHUVA, INPUT);
  pinMode(LED_CHUVA, OUTPUT);
  pinMode(BUZZER_CHUVA, OUTPUT);

  // Estado inicial das saídas (Desligadas)
  digitalWrite(LED_CHUVA, LOW);
  noTone(BUZZER_CHUVA);
  
  Serial.println("Iniciando leituras...\n");
}

void loop() {
  unsigned long tempoAtual = millis();

  // Monitora a chuva continuamente
  lerEControlarChuva();

  // Exibe o relatório de todos os sensores no Monitor Serial a cada 3 segundos
  if (tempoAtual - tempoAnterior >= intervaloLeitura) {
    tempoAnterior = tempoAtual;

    lerDHT();
    lerQualidadeAr();
    lerPressao();

    Serial.println("----------------------------------------------");
    
    if (isnan(umidade) || isnan(temp_dht)) {
      Serial.println("DHT11: Falha na leitura!");
    } else {
      Serial.print("Temperatura (DHT11): ");
      Serial.print(temp_dht, 1);
      Serial.println(" °C");
      
      Serial.print("Umidade Relativa:    ");
      Serial.print(umidade, 1);
      Serial.println(" %");
    }

    Serial.print("Pressão Absoluta:    ");
    Serial.print(pressao_abs, 1);
    Serial.println(" hPa");

    Serial.print("Pressão Relativa:    ");
    Serial.print(pressao_relativa, 1);
    Serial.println(" hPa");

    Serial.print("Qualidade do Ar:     ");
    Serial.print(ppm, 1);
    Serial.println(" PPM");

    Serial.print("Status do Tempo:     ");
    if (leituraChuva == LOW) {
      Serial.println("Chuva Detectada! [ALARME ATIVO]");
    } else {
      Serial.println("Sem Chuva [NÃO]");
    }

    Serial.println("----------------------------------------------\n");
  }
}