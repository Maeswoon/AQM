#include <MKRWAN.h>
#include <Wire.h>
#include <SPI.h> 
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
int payload[] = {0};
char s_buf[256], l_buf[256], temp_char;
int buf_len = 0;
int p = 0;

String l_in, l_out;     

byte l_addr = 0x00;    
byte dest = 0x11;     

void setup(){
  Serial.begin(9600);
  LoRa.begin(915E6);
}

void loop(){
  while (s_recv() == 0) delay(1);
  l_send(dest);
  while (l_recv() == 0) delay(1);
  s_send();
}

void s_send() {
  Serial.print(l_buf);
}

int s_recv() {
  if (!Serial.available()) return 0;
  int i = 0, j = 0, p = 0;
  char p_cont[16];
  temp_char = '0';
  memset(&s_buf, 0, sizeof(s_buf));
  while (Serial.available() > 0 && temp_char != '\n') {
    temp_char = Serial.read();
    if (temp_char == '\n') break;
    switch (temp_char) {
      case '>':
      case ',':
        payload[p++] = std::stoi(p_cont);
        memset(&p_cont, 0, sizeof(p_cont));
        j = 0;
      case '\n':
      case '<':
        break;
      default:
        p_cont[j++] = temp_char;
        s_buf[i++] = temp_char;
        break;
    }
  }
  buf_len = i;
  return 1;
}

void l_send(byte dst) {
  l_out = "<1,";
  for (int i = 0; i < sizeof(payload) / sizeof(payload[0]); ++i) {
    if (i > 0) l_out += ",";
    l_out += String(payload[i]);
  }
  l_out += ">";
  
  int msg_id = payload[0];

  LoRa.beginPacket();
  LoRa.write(dst);
  LoRa.write(l_addr);
  LoRa.write(msg_id);
  LoRa.write(l_out.length());
  LoRa.print(l_out);
  LoRa.endPacket();

  Serial.println(l_out);
}

int l_recv() {
  if (LoRa.parsePacket() == 0) return 0;
  int i = 0;
  l_in = "";
  char temp_char;
  
  memset(&l_buf, 0, sizeof(l_buf));
  byte dst = LoRa.read();
  byte origin = LoRa.read();
  int id = LoRa.read();
  int len = LoRa.read();

  while (LoRa.available()) {
    temp_char = (char)LoRa.read();
    if (dst == l_addr) l_buf[i++] = temp_char;
  }
  Serial.println(l_buf);

  return dst == l_addr;
}
