#include <dht11.h>
#include <math.h>

#define Radio Serial1
#define s_pin 6

const int MPU6050_addr=0x68;
int16_t AccX,AccY,AccZ,Temp,GyroX,GyroY,GyroZ;
double gX, gY, gZ, roll, pitch, sR, sP;
double rS[] = {0.0, 0.0, 0.0};
double pS[] = {0.0, 0.0, 0.0};
char r_payload[8][16];
char s_buf[256], r_buf[256];

char l_addr = '0';

void setup(){
  pinMode(s_pin, OUTPUT);
  digitalWrite(s_pin, LOW);
  delay(100);
  Serial.begin(9600);
  Radio.begin(9600, SERIAL_8N1);
  delay(100);
  Radio.println("AT+P8");
  delay(100);
  digitalWrite(s_pin, HIGH);
  delay(100);
  while (Radio.available()) Radio.read();
}

void loop(){
  while (s_recv() == 0) delay(1);
  while (r_recv() == 0) delay(1);
}

int s_recv() {
  if (!Serial.available()) return 0;
  while (Radio.available()) Radio.read();
  int i = 0;
  memset(&s_buf, 0, 256);
  while (Serial.available()) {
    char x = Serial.read();
    Radio.write(x);
  }
  return 1;
}

int r_recv() {
  if (!Radio.available()) return 0;
  delay(30);
  int i = 0, j = 0, p = 0;
  char p_cont[16];
  char temp_char = '0';
  memset(&r_buf, 0, sizeof(r_buf));
  memset(&r_payload, 0, sizeof(r_payload));
  while (Radio.available() && temp_char != '\n') {
    temp_char = Radio.read();
    switch (temp_char) {
      case '>':
      case ',':
        if (p == 0 && r_buf[i - 1] != l_addr) {
          memset(&r_buf, 0, sizeof(r_buf));
          delay(10);
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
        r_buf[i++] = temp_char;
        break;
      default:
        p_cont[j++] = temp_char;
        r_buf[i++] = temp_char;
        break;
    }
  }
  while (Radio.available()) Radio.read();
  Serial.print(r_buf);
  return 1;
}
