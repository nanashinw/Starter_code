#include <Wire.h>

#define PCA_MOTOR 0x40
#define PCA_SERVO 0x41

// ---------------- PCA9685 LOW LEVEL ----------------

void write8(uint8_t addr, uint8_t reg, uint8_t data){
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void setPWM(uint8_t addr, uint8_t ch, uint16_t on, uint16_t off){

  Wire.beginTransmission(addr);
  Wire.write(0x06 + 4 * ch);

  Wire.write(on & 0xFF);
  Wire.write(on >> 8);

  Wire.write(off & 0xFF);
  Wire.write(off >> 8);

  Wire.endTransmission();
}

// ---------------- INIT PCA ----------------

void initMotorPCA(){

  write8(PCA_MOTOR,0x00,0x10);   // sleep
  write8(PCA_MOTOR,0xFE,3);      // ~1kHz PWM
  write8(PCA_MOTOR,0x00,0x20);   // wake
  delay(10);
}

void initServoPCA(){

  write8(PCA_SERVO,0x00,0x10);   // sleep
  write8(PCA_SERVO,0xFE,121);    // 50Hz
  write8(PCA_SERVO,0x00,0xA1);   // restart
  delay(10);
}

// ---------------- MOTOR CONTROL ----------------

void motor(int id, int speed){

  int chA = (id-1)*2;
  int chB = chA + 1;

  speed = constrain(speed,-4095,4095);

  if(speed > 0){
    setPWM(PCA_MOTOR,chA,0,speed);
    setPWM(PCA_MOTOR,chB,0,0);
  }

  else if(speed < 0){
    setPWM(PCA_MOTOR,chA,0,0);
    setPWM(PCA_MOTOR,chB,0,-speed);
  }

  else{
    setPWM(PCA_MOTOR,chA,0,0);
    setPWM(PCA_MOTOR,chB,0,0);
  }
}

// ---------------- MECANUM DRIVE ----------------

void mecanumDrive(int vx, int vy, int omega){

  int m1 = vy + vx + omega;
  int m2 = vy - vx - omega;
  int m3 = vy - vx + omega;
  int m4 = vy + vx - omega;

  m1 = constrain(m1,-4095,4095);
  m2 = constrain(m2,-4095,4095);
  m3 = constrain(m3,-4095,4095);
  m4 = constrain(m4,-4095,4095);

  motor(1,m1);
  motor(2,m2);
  motor(3,m3);
  motor(4,m4);
}

// ---------------- SERVO CONTROL ----------------

void servoWrite(uint8_t ch, int angle){

  angle = constrain(angle,0,180);

  int pulse = map(angle,0,180,110,490);

  setPWM(PCA_SERVO,ch,0,pulse);
}

// ---------------- SETUP ----------------

// void setup(){

//   Wire.begin(21,22);

//   initMotorPCA();
//   initServoPCA();
// }

// // ---------------- DEMO ----------------

// void loop(){

//   // forward
//   mecanumDrive(0,400,0);
//   delay(2000);

//   // stop
//   mecanumDrive(0,0,0);
//   delay(1000);

//   // servo test
//   servoWrite(8,0);
//   delay(1000);

//   servoWrite(8,90);
//   delay(1000);

//   servoWrite(8,180);
//   delay(1000);
// }