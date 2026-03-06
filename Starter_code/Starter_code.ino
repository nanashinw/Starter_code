#include <Bluepad32.h>
#include <Wire.h>

// PIN CONNECTION
const int builtIn_Led = 2;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
      // Additionally, you can get certain gamepad properties like:
      // Model, VID, PID, BTAddr, flags, etc.
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                    properties.product_id);
      myControllers[i] = ctl;
      foundEmptySlot = true;
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("CALLBACK: Controller connected, but could not found empty slot");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  bool foundController = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
      myControllers[i] = nullptr;
      foundController = true;
      break;
    }
  }

  if (!foundController) {
    Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
  }
}

// ===================== SEE CONTROLLER VALUE IN SERIAL MONITOR ============================

void dumpGamepad(ControllerPtr ctl) {
  Serial.printf(
    "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
    "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
    ctl->index(),        // Controller Index
    ctl->dpad(),         // D-pad
    ctl->buttons(),      // bitmask of pressed buttons
    ctl->axisX(),        // (-511 - 512) left X Axis
    ctl->axisY(),        // (-511 - 512) left Y axis
    ctl->axisRX(),       // (-511 - 512) right X axis
    ctl->axisRY(),       // (-511 - 512) right Y axis
    ctl->brake(),        // (0 - 1023): brake button
    ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
    ctl->miscButtons(),  // bitmask of pressed "misc" buttons
    ctl->gyroX(),        // Gyro X
    ctl->gyroY(),        // Gyro Y
    ctl->gyroZ(),        // Gyro Z
    ctl->accelX(),       // Accelerometer X
    ctl->accelY(),       // Accelerometer Y
    ctl->accelZ()        // Accelerometer Z
  );
}

void processControllers() {
  for (auto myController : myControllers) {
    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isGamepad()) {
        processGamepad(myController);
      }
    }
  }
}


void processGamepad(ControllerPtr ctl) {

  // =========================
  // Arm Control (3 Servos)
  // =========================

  static bool gripperOpen = true;

  if (ctl->a()) {  // CROSS
    Serial.println("ARM: Default position");

    servoWrite(9, 45);    // Servo2
    servoWrite(10, 150);  // Servo3
    servoWrite(11, 0);    // Gripper open

    gripperOpen = true;
  }

  else if (ctl->b()) {  // CIRCLE
    Serial.println("ARM: Prepare grab ball");

    servoWrite(10, 150);  // Servo3 ก่อน
    delay(150);

    servoWrite(9, 35);  // Servo2
    delay(150);

    servoWrite(11, 0);  // Gripper เปิด
  }

  else if (ctl->x()) {  // SQUARE
    Serial.println("ARM: Toggle gripper");

    if (gripperOpen) {
      servoWrite(11, 90);  // ปิด gripper
      gripperOpen = false;
    } else {
      servoWrite(11, 0);  // เปิด gripper
      gripperOpen = true;
    }

    delay(300);  // กันกดค้าง
  }

  else if (ctl->y()) {  // TRIANGLE
    Serial.println("ARM: Prepare drop box");

    servoWrite(9, 45);  // Servo2
    servoWrite(10, 0);  // Servo3
  }


  // =========================
  // 2. Shoulder buttons
  // =========================

  if (ctl->l1()) Serial.println("L1 pressed");
  if (ctl->r1()) Serial.println("R1 pressed");
  if (ctl->l2()) Serial.println("L2 pressed");
  if (ctl->r2()) Serial.println("R2 pressed");


  // =========================
  // 3. Analog sticks (Drive)
  // =========================

  int ly = ctl->axisY();
  int rx = ctl->axisRX();

  int deadzone = 40;

  if (abs(ly) < deadzone) ly = 0;
  if (abs(rx) < deadzone) rx = 0;

  int forward = -ly * 4;
  int turn = rx * 3;

  int leftSpeed = forward + turn;
  int rightSpeed = forward - turn;

  leftSpeed = constrain(leftSpeed, -3000, 3000);
  rightSpeed = constrain(rightSpeed, -3000, 3000);

  // ถ้าไม่มี input ให้หยุด motor
  if (ly == 0 && rx == 0) {
    motor(1, 0);
    motor(2, 0);
    motor(3, 0);
    motor(4, 0);
  } else {
    motor(1, leftSpeed);
    motor(2, leftSpeed);
    motor(3, rightSpeed);
    motor(4, rightSpeed);
  }


  // =========================
  // 4. Trigger Analog
  // =========================

  int brake = ctl->brake();
  int throttle = ctl->throttle();

  if (brake > 100) {
    Serial.printf("L2 Pressure: %d\n", brake);
  }

  if (throttle > 100) {
    Serial.printf("R2 Pressure: %d\n", throttle);
  }


  // =========================
  // 5. D-Pad Servo
  // =========================

  static int lastAngle = -1;

  uint8_t dpad = ctl->dpad();
  int angle = -1;

  // UP / DOWN (ของเดิม)
  if (dpad & DPAD_UP) angle = 10;
  else if (dpad & DPAD_DOWN) angle = 90;


  // =========================
  // LEFT = เตรียมโยน
  // =========================
  if (dpad & DPAD_LEFT) {

    servoWrite(8, 130);  // Servo1
    delay(100);

    servoWrite(10, 90);  // Servo3
    delay(100);

  for(int pos = 45; pos <= 120; pos++){
    servoWrite(9, pos);
    delay(10);
  }
  }


  // =========================
  // RIGHT = โยนลูกบอล
  // =========================
  if (dpad & DPAD_RIGHT) {

    servoWrite(9, 45);  // ดีดแขน
    delay(300);

    servoWrite(11, 0);  // เปิด Gripper ปล่อยลูก
  }

  // =========================
  // ระบบเดิม UP / DOWN
  // =========================

  // สั่ง servo เฉพาะตอนเปลี่ยนมุม
  if (angle != -1 && angle != lastAngle) {

    if (angle == 90) {  // ก่อนจะไป 90°

      servoWrite(10, 180);  // Servo3
      delay(120);

      servoWrite(9, 45);  // Servo2
      delay(120);
    }

    servoWrite(8, angle);  // Servo1
    lastAngle = angle;
  }
}

void setup() {
  Wire.begin(21, 22);
  initServoPCA();
  initMotorPCA();
  pinMode(builtIn_Led, OUTPUT);
  Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But it might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  // Enables mouse / touchpad support for gamepads that support them.
  // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
  // - First one: the gamepad
  // - Second one, which is a "virtual device", is a mouse.
  // By default, it is disabled.
  BP32.enableVirtualDevice(false);
}

void loop() {
  // This call fetches all the controllers' data.
  // Call this function in your main loop.
  bool dataUpdated = BP32.update();
  if (dataUpdated)
    processControllers();

  // The main loop must have some kind of "yield to lower priority task" event.
  // Otherwise, the watchdog will get triggered.
  // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
  // Detailed info here:
  // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

  //     vTaskDelay(1);
  delay(1);
}
