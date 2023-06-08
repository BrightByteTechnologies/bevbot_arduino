#include <SPI.h>
#include <MFRC522.h>

class Motor {
private:
  int gsm; // Motor pin for speed control
  int in1; // Motor pin for direction control
  int in2; // Motor pin for direction control

  String currentDrivingState = "stop"; // Current state of motor driving

public:
  Motor(int motorPin, int input1Pin, int input2Pin)
    : gsm(motorPin), in1(input1Pin), in2(input2Pin) {
    pinMode(gsm, OUTPUT); // Set motor speed pin as output
    pinMode(in1, OUTPUT); // Set motor direction pin 1 as output
    pinMode(in2, OUTPUT); // Set motor direction pin 2 as output
  }

  void fullStop() {
    if (currentDrivingState != drivingStates[0])
      changeMotion(HIGH, LOW, 0); // Stop the motor
      currentDrivingState = drivingStates[0];
  }

  void middleSpeed(bool forwards) {
    if (currentDrivingState != drivingStates[1]) {
      changeMotion(forwards ? LOW : HIGH, forwards ? HIGH : LOW, 125); // Set motor to middle speed in the specified direction
      currentDrivingState = drivingStates[1];
    }
  }

  void fullSpeed(bool forwards) {
    if (currentDrivingState != drivingStates[2]) {
      changeMotion(forwards ? LOW : HIGH, forwards ? HIGH : LOW, 255); // Set motor to full speed in the specified direction
      currentDrivingState = drivingStates[2];
    }
  }

private:
  char* drivingStates[3] = { "stop", "middle", "full" }; // States of motor driving

  void changeMotion(uint8_t IN1, uint8_t IN2, uint8_t Speed) {
    digitalWrite(in1, IN1); // Set motor direction pin 1
    digitalWrite(in2, IN2); // Set motor direction pin 2
    analogWrite(gsm, Speed); // Set motor speed
  }
};

class UltrasonicSensor {
private:
  int trigPin; // Ultrasonic sensor pin for triggering
  int echoPin; // Ultrasonic sensor pin for receiving echo

public:
  UltrasonicSensor(int trig, int echo)
    : trigPin(trig), echoPin(echo) {
    pinMode(trigPin, OUTPUT); // Set trigger pin as output
    pinMode(echoPin, INPUT); // Set echo pin as input
  }

  int getDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH); // Measure the duration of echo pulse
    int distance = duration * 0.034 / 2; // Calculate distance based on the duration

    return distance;
  }
};

class RFIDReader {
private:
  byte* correctUID; // Correct UID for comparison
  MFRC522 mfrc522; // RFID module
  int completedTask; // Flag to track completion of tasks

public:
  RFIDReader(int ssPin, int rstPin)
    : correctUID(nullptr), mfrc522(ssPin, rstPin), completedTask(0) {}

  void setCorrectUID(byte* uid) {
    correctUID = uid; // Set the correct UID for comparison
  }

  void begin() {
    SPI.begin(); // Initialize SPI communication
    mfrc522.PCD_Init(); // Initialize RFID module
  }

  bool isCorrectCard() {
    if (correctUID == nullptr) {
      completedTask = -1; // No correct UID set, mark as incomplete
      return false;
    }

    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      completedTask = -1; // No card present or error reading card, mark as incomplete
      return false;
    }

    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i], HEX); // Print UID bytes
      Serial.print(" ");
    }
    Serial.println();

    if (memcmp(mfrc522.uid.uidByte, correctUID, mfrc522.uid.size) == 0) {
      completedTask = 1; // Correct card detected, mark as complete
      return true;
    } else {
      completedTask = -1; // Incorrect card detected, mark as incomplete
      return false;
    }
  }

  bool isCompleted() {
    return (completedTask == 1); // Check if task is completed
  }
};

UltrasonicSensor ultrasonic1(3, 4); // Create ultrasonic sensor instance 1
UltrasonicSensor ultrasonic2(6, 5); // Create ultrasonic sensor instance 2
RFIDReader reader(53, 3); // Create RFID reader instance

Motor motor1(8, 9, 10); // Create motor instance 1
Motor motor2(13, 11, 12); // Create motor instance 2
bool driveBackwards = true; // Flag to indicate driving direction

byte correctUID_A[] = { 0x4, 0xA3, 0xB0, 0x3F, 0x73, 0x0, 0x0 }; // Correct UID A
byte correctUID_B[] = { 0x4, 0x31, 0xD7, 0x3E, 0x73, 0x0, 0x0 }; // Correct UID B
byte correctUID_C[] = { 0x4, 0x9E, 0x7, 0x3F, 0x73, 0x0, 0x0 }; // Correct UID C

byte startUID[] = { 0xBA, 0x10, 0x7E, 0xBE }; // Start UID
byte endUID[] = { 0x99, 0x21, 0x12, 0x8F }; // End UID
int currentTableNo; // Current table number

void setup() {
  Serial.begin(9600); // Initialize serial communication
  SPI.begin(); // Initialize SPI communication
  reader.begin(); // Initialize RFID reader
}

void loop() {
  int distance1 = ultrasonic1.getDistance(); // Get distance from ultrasonic sensor 1
  int distance2 = ultrasonic2.getDistance(); // Get distance from ultrasonic sensor 2
  Serial.print("Distance1: ");
  Serial.println(distance1);

  Serial.print("Distance2: ");
  Serial.println(distance2);

  if (reader.isCompleted() || currentTableNo == 0) {
    if (currentTableNo == 0) {
      currentTableNo = getCurrentTableNo(); // Get current table number
    }
    Serial.print("CurrentTableNo: ");
    Serial.println(currentTableNo);
    switch (currentTableNo) {
      case 1:
        reader.setCorrectUID(correctUID_A); // Set correct UID for table 1
        currentTableNo = getCurrentTableNo(); // Get next table number
        driveBackwards = false; // Set driving direction to forwards
        break;
      case 2:
        reader.setCorrectUID(correctUID_B); // Set correct UID for table 2
        currentTableNo = getCurrentTableNo(); // Get next table number
        break;
      case 3:
        reader.setCorrectUID(correctUID_C); // Set correct UID for table 3
        currentTableNo = getCurrentTableNo(); // Get next table number
        break;
      case 4:
        reader.setCorrectUID(endUID); // Set correct UID for end table
        currentTableNo = getCurrentTableNo(); // Get next table number
        break;
      case 5:
        reader.setCorrectUID(startUID); // Set correct UID for start table
        currentTableNo = getCurrentTableNo(); // Get next table number
        break;
    }
  }

  if (reader.isCorrectCard()) {
    fullStop(); // Stop the motor
    Serial.println("Correct Card");

    if (currentTableNo == 5) {
      driveBackwards = true; // Set driving direction to backwards
      Serial.println("DriveBackwards");
    }

    delay(delaySeconds(30)); // Delay for 30 seconds
    return;
  }

  if (distance1 < 20 || distance2 < 20) {
    fullStop(); // Stop the motor
    return;
  } else if (distance1 > 80 && distance2 > 80) {
    if (!driveBackwards) {
      fullSpeed(true); // Drive at full speed forwards
    } else {
      Serial.println("Drives backwards fullSpeed");
      fullSpeed(false); // Drive at full speed backwards
    }
    return;
  } else {
    if (!driveBackwards) {
      middleSpeed(true); // Drive at middle speed forwards
    } else {
      middleSpeed(false); // Drive at middle speed backwards
      Serial.println("Drives backwards middleSpeed");
    }
    return;
  }
}

int tableNos[] = { 1, 2, 3, 4, 5 };
int currentTableNos[] = { -1, -1, -1, -1, -1 };  // Initialize currentTableNos with -1 values
int lastTableNo;

int getCurrentTableNo() {
  int arraySize = sizeof(currentTableNos) / sizeof(currentTableNos[0]);

  for (int i = 0; i < arraySize; i++) {
    if (currentTableNos[i] != -1) {
      lastTableNo = currentTableNos[i];
      currentTableNos[i] = -1;
      return lastTableNo; // Return the next available table number
    }
  }

  // If all entries are -1 or arraySize is 0, reset currentTableNos
  for (int i = 0; i < arraySize; i++) {
    if (i < sizeof(tableNos) / sizeof(tableNos[0]))
      currentTableNos[i] = tableNos[i];
    else
      currentTableNos[i] = -1;
  }
  lastTableNo = currentTableNos[0];
  currentTableNos[0] = -1;
  return lastTableNo; // Return the first table number if all tables have been visited
}

void fullStop() {
  motor1.fullStop(); // Stop motor 1
  motor2.fullStop(); // Stop motor 2
}

void middleSpeed(bool backwards) {
  motor1.middleSpeed(backwards); // Drive motor 1 at middle speed
  motor2.middleSpeed(backwards); // Drive motor 2 at middle speed
}

void fullSpeed(bool backwards) {
  motor1.fullSpeed(backwards); // Drive motor 1 at full speed
  motor2.fullSpeed(backwards); // Drive motor 2 at full speed
}
