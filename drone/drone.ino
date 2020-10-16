#include <MKRWAN.h>
#include <Wire.h>
#include "LoRa.h"
#include <cstdio>
#include <dht11.h>
#include <math.h>
#include <iostream>
#include <string>

const int MPU6050_addr=0x68;
int16_t AccX,AccY,AccZ,Temp,GyroX,GyroY,GyroZ;
double gX, gY, gZ, roll, pitch, sR, sP;
double rS[] = {0.0, 0.0, 0.0};
double pS[] = {0.0, 0.0, 0.0};
char l_buf[256], temp_char;
int buf_len, c_id, p = 0;
String l_in, l_out;     
byte l_addr = 0x11;    
byte dest = 0x00;     
dht11 DHT11;

void setup(){
  Wire.begin();
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  pinMode(7, OUTPUT);
  Serial.begin(9600);

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
  delay(2000);

  if (!LoRa.begin(915E6)) {
    while(1);
  }
}

void loop(){
  long t = millis();
  DHT11.read(6);
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
  roll = grav(gX, gY, gZ);
  pitch = grav(gY, gX, gZ);
  
  rS[p] = roll;
  pS[p] = pitch;
  sR = 0.0;
  sP = 0.0;
  for (int i = 0; i < 3; ++i) {
    sR += rS[i] / 3.0;
    sP += pS[i] / 3.0;
  }

  while (l_recv() == 0) delay(1);
  l_send(dest);
  
  if (++p == 3) p = 0;
}
 
double dist(double a, double b) {
  return sqrt(pow(a, 2) + pow(b, 2));
}

double grav(double r, double a, double b) {
  return atan2(r, dist(a, b)) * (180.0 / M_PI);
}

void l_send(byte dst) {
  LoRa.beginPacket();
  LoRa.write(dst);
  LoRa.write(l_addr);
  LoRa.write(c_id);
  LoRa.write(buf_len);
  LoRa.print(l_buf);
  LoRa.endPacket();
}

int l_recv() {
  if (LoRa.parsePacket() == 0) return 0;
  int i = 0;
  l_in = "";
  char temp_char;
  
  memset(&l_buf, 0, sizeof(l_buf));
  byte dst = LoRa.read();
  byte origin = LoRa.read();
  c_id = LoRa.read();
  int len = LoRa.read();

  while (LoRa.available()) {
    temp_char = (char)LoRa.read();
    if (dst == l_addr) l_buf[i++] = temp_char;
  }

  buf_len = i;

  return dst == l_addr;
}
