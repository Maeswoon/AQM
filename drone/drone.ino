#include <Wire.h>
#include <dht11.h>
#include <math.h>

#define Radio Serial1
#define s_pin 7                                               
#define dht_pin 6

const int MPU6050_addr=0x68;
int16_t AccX,AccY,AccZ,Temp,GyroX,GyroY,GyroZ;
double gX, gY, gZ, roll, pitch, sR, sP;
double rS[] = {0.0, 0.0, 0.0};
double pS[] = {0.0, 0.0, 0.0};
char r_payload[8][16];
char r_buf[256];
int k = 0;    
dht11 DHT11;

char l_addr = '1';
char dst = '0';  

void setup(){
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
  while (!Radio.available()) {
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
    roll = grav(gX, gY, gZ);
    pitch = grav(gY, gX, gZ);
    
    rS[k] = roll;
    pS[k] = pitch;
    sR = 0.0;
    sP = 0.0;
    for (int i = 0; i < 3; ++i) {
      sR += rS[i] / 3.0;
      sP += pS[i] / 3.0;
    }
    if (++k == 3) k = 0;
    delay(5);
  }
  delay(20);
  if (r_recv() > 0) r_send(dst);
}
 
double dist(double a, double b) {
  return sqrt(pow(a, 2) + pow(b, 2));
}

double grav(double r, double a, double b) {
  return atan2(r, dist(a, b)) * (180.0 / M_PI);
}

void r_send(char dest) {
  Radio.write("<");
  Radio.write(dest);
  Radio.write(",");
  Radio.write(l_addr);
  Radio.write(",");
  Radio.print(r_payload[2]);
  Radio.write(",");
  Radio.print((int) sP);
  Radio.write(",");
  Radio.print((int) sR);
  Radio.write(",");
  Radio.print((int) DHT11.temperature);
  Radio.write(",");
  Radio.print((int) DHT11.humidity);
  Radio.write(">");
  Radio.write('\n');
}

int r_recv() {
  if (!Radio.available()) return 0;
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
  while(Radio.available()) Radio.read();
  return 1;
}
