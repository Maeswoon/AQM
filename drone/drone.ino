#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include "LoRa.h"
#include "Adafruit_CCS811.h"
#include <dht11.h>
#include <math.h>

#define Radio Serial1
#define s_pin 7                                               
#define dht_pin 6
#define RECV_TIMEOUT 200
#define LORA_TIMEOUT 1000
#define MAX_RDEFL 45
#define MAX_PDEFL 45
#define MAX_ROLL 30
#define MAX_PITCH 30
#define AIL_OFFSET 90
#define ELEV_OFFSET 90
#define ROLL_OFFSET 0
#define PITCalt_OFFSET 0

const int MPU6050_addr = 0x68;
int16_t AccX,AccY,AccZ,Temp,GyroX,GyroY,GyroZ;
double gX, gY, gZ, roll, pitch, sR, sP, r_defl, p_defl;
double d_curr, d_exp, alt_curr, alt_exp, lat, lon;
double rS[] = {0.0, 0.0, 0.0};
double pS[] = {0.0, 0.0, 0.0};
char r_payload[8][16];
char r_buf[256], l_out[128], r_out[128];
int k = 0, r_exp, p_exp;    
dht11 DHT11;
Adafruit_CCS811 voc;

Servo l_ail;
Servo r_ail;
Servo elev;

char l_addr = '1';
char dst = '0';  
bool autonomous = false;
bool debug = true;
long r, l_t;

void setup(){
  l_ail.attach(2);
  r_ail.attach(3);
  elev.attach(4);
  r_exp = 0;
  p_exp = 0;
  d_curr = 0.0;
  d_exp = 0.0;
  alt_curr = 10.0;
  alt_exp = 0.0;
  lat = 0.0;
  lon = 0.0;
  
  Wire.begin();
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x1C); 
  Wire.endTransmission();
  
  Wire.requestFrom(MPU6050_addr, 1);
  byte x = Wire.read(); 
  
  x = x | 0b00011000;     
  
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x1C); 
  Wire.write(x);      
  Wire.endTransmission();

  voc.begin();

  if (!LoRa.begin(915E6)) {
    while (1);
  }

  pinMode(s_pin, OUTPUT);
  digitalWrite(s_pin, LOW);
  delay(100);
  Serial.begin(115200);
  Radio.begin(9600, SERIAL_8N1);
  delay(100);
  Radio.println("AT+P8");
  delay(100);
  digitalWrite(s_pin, HIGH);
  delay(100);
  while (Radio.available()) Radio.read();
  l_t = millis();
}

void loop(){
  while ((!autonomous && !Radio.available()) || (autonomous && millis() - l_t < LORA_TIMEOUT)) {
    DHT11.read(dht_pin);
    Wire.beginTransmission(MPU6050_addr);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_addr, 14, true);
    
    AccX = Wire.read() << 8 | Wire.read();
    AccY = Wire.read() << 8 | Wire.read();
    AccZ = Wire.read() << 8 | Wire.read();
    Temp = Wire.read() << 8 | Wire.read();
    GyroX = Wire.read() << 8 | Wire.read();
    GyroY = Wire.read() << 8 | Wire.read();
    GyroZ = Wire.read() << 8 | Wire.read();
    
    gX = (AccX / 16384.0);
    gY = (AccY / 16384.0);
    gZ = (AccZ / 16384.0);
    pitch = -1.0 * grav(gY, gX, gZ);
    roll = grav(gX, gY, gZ);
    
    rS[k] = roll;
    pS[k] = pitch;
    sR = 0.0;
    sP = 0.0;
    for (int i = 0; i < 3; ++i) {
      sR += rS[i] / 3.0;
      sP += pS[i] / 3.0;
    }
    if (++k == 3) k = 0;

    if (autonomous) {
      r_exp = MAX_ROLL * pow(sin((d_curr - d_exp) * 0.1), 2) + ROLL_OFFSET;
      p_exp = MAX_PITCH * pow(sin((alt_curr - alt_exp) * 0.2), 2) + PITCalt_OFFSET;
    }

    r_defl = MAX_RDEFL * pow(sin(sR * (M_PI / 180.0)), 2) * sR / abs(sR);
    l_ail.write((int) r_defl - r_exp + AIL_OFFSET);
    r_ail.write(r_exp - (int) r_defl + AIL_OFFSET); 
    
    p_defl = MAX_PDEFL * pow(sin(sP * (M_PI / 180.0)), 2) * sP / abs(sP);
    elev.write((int) p_defl - p_exp + ELEV_OFFSET);
    
    if (debug) {
      Serial.print("r_exp: ");
      Serial.print(r_exp);
      Serial.print("\t");
      Serial.print("p_exp: ");
      Serial.print(p_exp);
      Serial.print("\t");
      Serial.print("r_defl: ");
      Serial.print((int) r_defl - r_exp + AIL_OFFSET);
      Serial.print("\t");
      Serial.print("p_defl: ");
      Serial.print((int) p_defl - p_exp + ELEV_OFFSET);
      Serial.print("\t");
      Serial.print("auto: ");
      Serial.println(autonomous ? "true" : "false");
    }
  }
  
  if (!autonomous && r_recv() > 0)  {
    
    Radio.write("<");
    Radio.write(dst);
    Radio.write(",");
    Radio.write(l_addr);
    Radio.write(",1,");
    Radio.print((int) sP);
    Radio.write(",");
    Radio.print((int) sR);
    Radio.write(">");
    Radio.write('\n');
  }
  
  if (autonomous && voc.available()) {
    voc.readData();
    memset(&l_out, 0, sizeof(l_out));
    sprintf(l_out, "<%d,%d,4,%.8f,%.8f,%d,%d,%d,%d,%d>\n", dst, l_addr, lat, lon, 
            (int) alt_curr, (int) DHT11.temperature, (int) DHT11.humidity,
            (int) voc.geteCO2(), (int) voc.getTVOC());
    
    Serial.println("Starting packet...");
    LoRa.beginPacket();
    LoRa.print(l_out);
    LoRa.endPacket();
    Serial.println("Packet sent.");
    l_t = millis();
  }
  
}
 
double dist(double a, double b) {
  return sqrt(pow(a, 2) + pow(b, 2));
}

double grav(double r, double a, double b) {
  return atan2(r, dist(a, b)) * (180.0 / M_PI);
}

int r_recv() {
  if (!Radio.available()) return 0;
  int i = 0, j = 0, p = 0;
  char p_cont[16];
  memset(&r_buf, 0, sizeof(r_buf));
  memset(&r_payload, 0, sizeof(r_payload));
  r = millis();
  bool msg_start = false;
  char temp_char = '0';
  while (temp_char != '\n' && millis() - r < RECV_TIMEOUT) {
    if (Radio.available()) {
      temp_char = Radio.read();
      if (msg_start == true || temp_char == '<' || temp_char == '!') {
        switch (temp_char) {
          case '!':
            autonomous = true;
            Radio.write("!");
            return 0;
          case '>':
          case ',':
            if (p == 0 && r_buf[i - 1] != l_addr) {
              memset(&r_buf, 0, sizeof(r_buf));
              delay(20);
              while (Radio.available()) Radio.read();
              return 0;
            }
            strcpy(r_payload[p++], p_cont);
            memset(&p_cont, 0, sizeof(p_cont));
            r_buf[i++] = temp_char;
            j = 0;   
            break;
          case '\n':
          case '<':
            msg_start = true;
            r_buf[i++] = temp_char;
            break;
          default:
            p_cont[j++] = temp_char;
            r_buf[i++] = temp_char;
            break;
        }
      }
    }
  }
  Serial.println(r_buf);
  while (Radio.available()) Radio.read();
  if (millis() - r >= RECV_TIMEOUT) return 0;
  return 1;
}
