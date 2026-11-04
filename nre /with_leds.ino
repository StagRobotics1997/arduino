// the loop function runs over and over again forever
//
// Project for Stag Robotics
// Purpose - use bluetooth gamepad controller to drive launcher robot
//
// Using libraries - Bluepad32, Adafruit NeoPixel, Adafruit PWM Servo Driver (PCA9685)
//
// Code significantly influenced by examples for Bluepad32 - https://gitlab.com/ricardoquesada/bluepad32
//
// https://gitlab.com/ricardoquesada/bluepad32/-/blob/main/docs/plat_nina.md
// Above link shows how to load the proper firmware onto the arduino/nina boards
//

#include <Bluepad32.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm;  // default I2C address 0x40

// Motor PWM settings (microseconds)
#define PWM_MIN 1000
#define PWM_NEUTRAL 1500
#define PWM_MAX 2000
#define LEFT_MOTOR_CH 0
#define RIGHT_MOTOR_CH 1
#define STICK_DEADBAND 50

// Set to true to reverse a side's direction (differential drives usually need one side reversed)
#define INVERT_LEFT true
#define INVERT_RIGHT true

int RELAY_PIN = 7;
ControllerPtr myControllers[BP32_MAX_CONTROLLERS];

// Smoothed motor outputs, in microseconds
int16_t old_left = PWM_NEUTRAL;
int16_t old_right = PWM_NEUTRAL;

// LED Definitions
#define STRIPLENGTH_SHORT 8  // Popular NeoPixel ring size
#define STRIPLENGTH_LONG 21
#define ANTLER_LED_STRIP 0
#define SHORT_STRIP_COUNT 5
#define CHARGE_COLOR 0, 0, 255
#define WHITE 55, 55, 55
#define RED 255, 0, 0
#define DIM_RED 10, 0, 0
#define BLUE 0, 0, 255
#define DELAYVAL 100  // Time (in milliseconds) to pause between pixels
#define LED_PIN 6
#define LEDS_ACTIVE true

Adafruit_NeoPixel pixels(STRIPLENGTH_SHORT * SHORT_STRIP_COUNT + STRIPLENGTH_LONG + ANTLER_LED_STRIP, LED_PIN, NEO_GRB + NEO_KHZ800);
int ledMode = 0;

// Put both motors in neutral and reset the smoothing state
void stopMotors() {
  pwm.writeMicroseconds(LEFT_MOTOR_CH, PWM_NEUTRAL);
  pwm.writeMicroseconds(RIGHT_MOTOR_CH, PWM_NEUTRAL);
  old_left = PWM_NEUTRAL;
  old_right = PWM_NEUTRAL;
}

// Arduino setup function. Runs in CPU 1
void setup() {
  // Initialize serial
  Serial.begin(9600);
  int retries = 0;
  while (!Serial) {
    delay(1000);
    retries++;
    if (retries > 5) break;
    // wait for serial port to connect (debugging only)
  }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Start the PWM driver first so the motors get neutral as early as possible
  pwm.begin();
  pwm.setOscillatorFrequency(28000000);  // nominal; calibrate so 1500 us measures as 1500 us
  pwm.setPWMFreq(50);
  stopMotors();

  String fv = BP32.firmwareVersion();
  Serial.print("Firmware version installed: ");
  Serial.println(fv);

  // To get the BD Address (MAC address) call:
  const uint8_t* addr = BP32.localBdAddress();
  Serial.print("BD Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(addr[i], HEX);
    if (i < 5)
      Serial.print(":");
    else
      Serial.println();
  }

  // This call is mandatory. It sets up Bluepad32 and creates the callbacks.
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // Forgetting Bluetooth keys prevents "paired" gamepads from reconnecting,
  // but might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  if (LEDS_ACTIVE) {
    pixels.begin();
    pixels.setBrightness(200);
    pixels.show();  // initialize all pixels to 'off'
        pixels.fill(pixels.Color(255, 0, 0));
    pixels.show();
    delay(2000);
  }
}

void charge() {
  if (!LEDS_ACTIVE) return;

  pixels.clear();  // Set all pixel colors to 'off'

  int diff = STRIPLENGTH_LONG - STRIPLENGTH_SHORT;
  for (int i = 0; i < STRIPLENGTH_LONG; i++) {  // For each pixel...

    pixels.setPixelColor(i, pixels.Color(CHARGE_COLOR));
    if (i > STRIPLENGTH_LONG - STRIPLENGTH_SHORT - 1) {
      for (int x = 0; x < SHORT_STRIP_COUNT; x++) {
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff, pixels.Color(CHARGE_COLOR));
      }
    }

    pixels.show();  // Send the updated pixel colors to the hardware.

    delay(DELAYVAL);  // Pause before next pass through loop
  }
}

void shoot() {
  if (!LEDS_ACTIVE) return;
  pixels.clear();  // Set all pixel colors to 'off'

  int diff = STRIPLENGTH_LONG - STRIPLENGTH_SHORT;

  pixels.fill(pixels.Color(WHITE));
  pixels.show();  // Send the updated pixel colors to the hardware.

  for (int i = 0; i < STRIPLENGTH_LONG; i++) {  // For each pixel...

    pixels.setPixelColor(i, pixels.Color(RED));
    if (i > 0) {
      pixels.setPixelColor(i - 1, pixels.Color(DIM_RED));
    }
    if (i > STRIPLENGTH_LONG - STRIPLENGTH_SHORT) {
      for (int x = 0; x < SHORT_STRIP_COUNT; x++) {
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff, pixels.Color(RED));
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff - 1, pixels.Color(DIM_RED));
      }
    }

    pixels.show();  // Send the updated pixel colors to the hardware.

    delay(DELAYVAL);  // Pause before next pass through loop
  }
  for (int i = 0; i < ANTLER_LED_STRIP; i++) {  // For each pixel...
    pixels.setPixelColor(i + STRIPLENGTH_LONG + STRIPLENGTH_SHORT * SHORT_STRIP_COUNT, pixels.Color(BLUE));
  }
  pixels.show();
  delay(1000);
}

void idle() {
  if (!LEDS_ACTIVE) return;
  pixels.clear();  // Set all pixel colors to 'off'

  int diff = STRIPLENGTH_LONG - STRIPLENGTH_SHORT;
  for (int i = 0; i < STRIPLENGTH_LONG - 2; i++) {  // For each pixel...

    pixels.setPixelColor(i, pixels.Color(50, 50, 50, 50));
    pixels.setPixelColor(i + 1, pixels.Color(255, 255, 255, 255));
    pixels.setPixelColor(i + 2, pixels.Color(50, 50, 50, 50));
    if (i > 0) {
      pixels.setPixelColor(i - 1, pixels.Color(0, 0, 0, 0));
    }
    if (i > (STRIPLENGTH_LONG - STRIPLENGTH_SHORT) - 1) {
      for (int x = 0; x < SHORT_STRIP_COUNT; x++) {
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff - 1, pixels.Color(0, 0, 0, 0));
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff, pixels.Color(50, 50, 50, 50));
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff + 1, pixels.Color(255, 255, 255, 255));
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff + 2, pixels.Color(50, 50, 50, 50));
      }
    }

    pixels.show();  // Send the updated pixel colors to the hardware.

    delay(50);  // Pause before next pass through loop
  }
  for (int i = STRIPLENGTH_LONG - 1; i > 0; i--) {  // For each pixel...

    pixels.setPixelColor(i, pixels.Color(50, 50, 50, 50));
    pixels.setPixelColor(i - 1, pixels.Color(255, 255, 255, 255));
    pixels.setPixelColor(i - 2, pixels.Color(50, 50, 50, 50));
    if (i > 0) {
      pixels.setPixelColor(i + 1, pixels.Color(0, 0, 0, 0));
    }
    if (i > STRIPLENGTH_LONG - STRIPLENGTH_SHORT + 2) {
      for (int x = 0; x < SHORT_STRIP_COUNT; x++) {
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff + 1, pixels.Color(0, 0, 0, 0));
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff, pixels.Color(50, 50, 50, 50));
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff - 1, pixels.Color(255, 255, 255, 255));
        pixels.setPixelColor(i + STRIPLENGTH_LONG + (x * STRIPLENGTH_SHORT) - diff - 2, pixels.Color(50, 50, 50, 50));
      }
    } else {
      pixels.fill(0, STRIPLENGTH_LONG - 2, STRIPLENGTH_LONG + (STRIPLENGTH_SHORT * SHORT_STRIP_COUNT));
    }

    pixels.show();  // Send the updated pixel colors to the hardware.

    delay(50);  // Pause before next pass through loop
  }
}

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.print("CALLBACK: Controller is connected, index=");
      Serial.println(i);
      myControllers[i] = ctl;
      foundEmptySlot = true;

      // Optional, once the gamepad is connected, request further info about the gamepad.
      ControllerProperties properties = ctl->getProperties();
      char buf[80];
      sprintf(buf,
              "BTAddr: %02x:%02x:%02x:%02x:%02x:%02x, VID/PID: %04x:%04x, "
              "flags: 0x%02x",
              properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
              properties.btaddr[3], properties.btaddr[4], properties.btaddr[5],
              properties.vendor_id, properties.product_id, properties.flags);
      Serial.println(buf);
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("CALLBACK: Controller connected, but could not find empty slot");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  bool foundGamepad = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.print("CALLBACK: Controller is disconnected from index=");
      Serial.println(i);
      myControllers[i] = nullptr;
      foundGamepad = true;
      stopMotors();  // make sure motors are set to neutral
      break;
    }
  }

  if (!foundGamepad) {
    Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
  }
}

void processGamepad(ControllerPtr gamepad) {
  if (gamepad->a()) {
    shoot();
    Serial.print("A pressed.");
    stopMotors();  // the delay below blocks the loop, so stop the motors first
    digitalWrite(RELAY_PIN, HIGH);
    delay(4000);
    digitalWrite(RELAY_PIN, LOW);
    ledMode = 1;
  }
  if (gamepad->b()) {
    Serial.print("B pressed.");
    if (ledMode == 0) {
      ledMode = 1;
    } else {
      ledMode = 2;
    }
  }
  if (gamepad->y()) {
    Serial.print("Y pressed.");
    ledMode = 0;
  }

  if (ledMode > 0) {
    stopMotors();  // driving is disabled in these modes, so hold neutral
    return;
  }

  // set motor speeds - algorithm from - https://xiaoxiae.github.io/Robotics-Simplified-Website/drivetrain-control/arcade-drive/
  int32_t drive = -gamepad->axisY();  // gamepad returns -512 for this axis when pushed fully up
  int32_t rotate = gamepad->axisX();

  // deadband on both axes so stick drift doesn't make the robot creep
  if (abs(drive) < STICK_DEADBAND) drive = 0;
  if (abs(rotate) < STICK_DEADBAND) rotate = 0;
  rotate = rotate / 2;

  int32_t maximum = max(abs(drive), abs(rotate));
  int32_t total = drive + rotate;
  int32_t difference = drive - rotate;
  int32_t right, left;

  // set speed according to the quadrant that the values are in
  if (drive < 0) {      // backwards
    if (rotate >= 0) {  // IV quadrant
      left = total;
      right = -maximum;
    } else {  // III quadrant
      left = -maximum;
      right = difference;
    }
  } else {              // forward
    if (rotate >= 0) {  // I quadrant
      left = maximum;
      right = difference;
    } else {  // II quadrant
      left = total;
      right = maximum;
    }
  }

  // convert -512..512 to pulse width in microseconds
  int32_t leftUs = INVERT_LEFT ? map(left, -512, 512, PWM_MAX, PWM_MIN)
                               : map(left, -512, 512, PWM_MIN, PWM_MAX);
  int32_t rightUs = INVERT_RIGHT ? map(right, -512, 512, PWM_MAX, PWM_MIN)
                                 : map(right, -512, 512, PWM_MIN, PWM_MAX);

  if (gamepad->r2() || gamepad->l2()) {
    leftUs = rightUs;
  }

  // simple smoothing: average with the previous output, clamped to the valid range
  old_left = constrain((leftUs + old_left) / 2, PWM_MIN, PWM_MAX);
  old_right = constrain((rightUs + old_right) / 2, PWM_MIN, PWM_MAX);

  pwm.writeMicroseconds(LEFT_MOTOR_CH, old_left);
  pwm.writeMicroseconds(RIGHT_MOTOR_CH, old_right);

  char buf[128];
  snprintf(buf, sizeof(buf) - 1,
           "X: %4li, Y: %4li => L: %4li, R: %4li => Lus: %4d, Rus: %4d",
           (long)rotate, (long)drive, (long)left, (long)right, old_left, old_right);
  Serial.println(buf);
}

// Arduino loop function. Runs in CPU 1
void loop() {
  // This call fetches all the controller info from the NINA (ESP32) module.
  BP32.update();

  // It is safe to always do this before using the controller API.
  // This guarantees that the controller is valid and connected.
  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    ControllerPtr myController = myControllers[i];
    if (myController && myController->isConnected()) {
      processGamepad(myController);
    }
  }
  if (ledMode == 1) idle();
  if (ledMode == 2) charge();
  delay(150);
}
