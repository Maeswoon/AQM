#include <dht11.h>
#include <math.h>

#define Radio Serial1
#define s_pin 6
#define TIMEOUT 5000
#define S_TIMEOUT 100
#define R_TIMEOUT 100

char r_payload[8][16];
char s_buf[256], r_buf[256];

char l_addr = '0';
long t, rt, st;

void setup(){
  pinMode(s_pin, OUTPUT);
  digitalWrite(s_pin, LOW);
  delay(100);
  Serial.begin(56000);
  Radio.begin(9600, SERIAL_8N1);
  delay(100);
  Radio.println("AT+P8");
  delay(100);
  digitalWrite(s_pin, HIGH);
  delay(100);
  while (Radio.available()) Radio.read();
  t = millis();
}

void loop(){
  while (s_recv() == 0) delay(1);
  while (r_recv() == 0 && millis() - t < TIMEOUT) delay(1);
}

int s_recv() {
  if (!Serial.available()) return 0;
  while (Radio.available()) Radio.read();
  int i = 0;
  memset(&s_buf, 0, 256);
  char c;
  st = millis();
  while (c != '\n' && millis() - st < S_TIMEOUT) {
    c = Serial.read();
    Radio.write(c);
    s_buf[i++] = c;
  }
  t = millis();
  return 1;
}

int r_recv() {
  if (!Radio.available()) return 0;
  while (Serial.available()) Serial.read();
  int i = 0, j = 0, p = 0;
  bool msg_start = false;
  char p_cont[16];
  char temp_char = '0';
  memset(&r_buf, 0, sizeof(r_buf));
  memset(&r_payload, 0, sizeof(r_payload));
  rt = millis();
  while (millis() - rt < R_TIMEOUT && temp_char != '\n') {
    if (Radio.available()) {
      temp_char = Radio.read();
      if (msg_start == true || temp_char == '<') {
         switch (temp_char) {
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
  if (msg_start == false) {
    delay(20);
    while (Radio.available()) Radio.read();
  }
  if (temp_char == '\n' && msg_start) Serial.print(r_buf);
  return 1;
}
