#include <SPI.h>
#include <MFRC522.h>

class Motor {
  private:
    int gsm;
    int in1;
    int in2;

  public:
    Motor(int motorPin, int input1Pin, int input2Pin)
      : gsm(motorPin), in1(input1Pin), in2(input2Pin) {
      pinMode(gsm, OUTPUT);
      pinMode(in1, OUTPUT);
      pinMode(in2, OUTPUT);
    }

    void fullSpeed(bool forwards) {
      changeMotion(forwards ? LOW : HIGH, forwards ? HIGH : LOW, 255);
    }

    void middleSpeed(bool forwards) {
      changeMotion(forwards ? LOW : HIGH, forwards ? HIGH : LOW, 125);
    }

    void fullStop() {
      changeMotion(HIGH, LOW, 0);
    }

  private:
    void changeMotion(uint8_t IN1, uint8_t IN2, uint8_t Speed) {
      digitalWrite(in1, IN1);
      digitalWrite(in2, IN2);
      analogWrite(gsm, Speed);
    }
};

class UltrasonicSensor {
  private:
    int trigPin;
    int echoPin;

  public:
    UltrasonicSensor(int trig, int echo)
      : trigPin(trig), echoPin(echo) {
      pinMode(trigPin, OUTPUT);
      pinMode(echoPin, INPUT);
    }

    int getDistance() {
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      long duration = pulseIn(echoPin, HIGH);
      int distance = duration * 0.034 / 2;

      return distance;
    }
};

class RFIDReader {
  private:
    byte* correctUID;
    MFRC522 mfrc522;
    int completedTask;

  public:
    RFIDReader(int ssPin, int rstPin)
      : correctUID(nullptr), mfrc522(ssPin, rstPin), completedTask(0) {}

    void setCorrectUID(byte* uid) {
      correctUID = uid;
    }

    void begin() {
      SPI.begin();
      mfrc522.PCD_Init();
    }

    bool isCorrectCard() {
      if (correctUID == nullptr) {
        completedTask = -1;
        return false;
      }

      if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        completedTask = -1;
        return false;
      }

      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      if (memcmp(mfrc522.uid.uidByte, correctUID, mfrc522.uid.size) == 0) {
        completedTask = 1;
        return true;
      } else {
        completedTask = -1;
        return false;
      }
    }

    bool isCompleted() {
      return (completedTask == 1);
    }
};

UltrasonicSensor ultrasonic1(3, 4);
UltrasonicSensor ultrasonic2(6, 5);
RFIDReader reader(53, 3);

Motor motor1(8, 9, 10);
Motor motor2(13, 11, 12);
bool driveBackwards = true;

byte correctUID_A[] = {0x4, 0xA3, 0xB0, 0x3F , 0x73 , 0x0 , 0x0};
byte correctUID_B[] = {0x4, 0x31, 0xD7, 0x3E , 0x73 , 0x0 , 0x0};
byte correctUID_C[] = {0x4, 0x9E, 0x7, 0x3F , 0x73 , 0x0 , 0x0};

byte startUID[] = {0xBA, 0x10, 0x7E, 0xBE};
byte endUID[] = {0x99, 0x21, 0x12, 0x8F};
int currentTableNo;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  reader.begin();
}

void loop() {
  int distance1 = ultrasonic1.getDistance();
  int distance2 = ultrasonic2.getDistance();
  Serial.print("Distance1: ");
  Serial.println(distance1);

  Serial.print("Distance2: ");
  Serial.println(distance2);

  if (reader.isCompleted() || currentTableNo == 0) {
    if(currentTableNo == 0) {
      currentTableNo = getCurrentTableNo();
    }
    Serial.print("CurrentTableNo: ");
    Serial.println(currentTableNo);
    switch (currentTableNo) {
      case 1:
        reader.setCorrectUID(correctUID_A);
        currentTableNo = getCurrentTableNo();
        driveBackwards = false;
        break;
      case 2:
        reader.setCorrectUID(correctUID_B);
        currentTableNo = getCurrentTableNo();
        break;
      case 3:
        reader.setCorrectUID(correctUID_C);
        currentTableNo = getCurrentTableNo();
        break;
      case 4:
        reader.setCorrectUID(endUID);
        currentTableNo = getCurrentTableNo();
        break;
      case 5:
        reader.setCorrectUID(startUID);
        currentTableNo = getCurrentTableNo();
        break;
    }
  }

  if (reader.isCorrectCard()) {
    fullStop();
    Serial.println("Correct Card");

    if (currentTableNo == 5) {

      driveBackwards = true;
      Serial.println("DriveBackwards");
    }

    delay(delaySeconds(30));
    return;
  }

  if (distance1 < 20 || distance2 < 20) {
    fullStop();
    return;
  } else if (distance1 > 80 && distance2 > 80) {
    if (!driveBackwards) {
      fullSpeed(true);
    } else {
      Serial.println("Drives backwards fullSpeed");
      fullSpeed(false);
    }
    return;
  } else {
    if (!driveBackwards) {
      middleSpeed(true);
    } else {
      middleSpeed(false);
      Serial.println("Drives backwards middleSpeed");
    }
    return;
  }
}

int tableNos[] = {1, 2, 3, 4, 5};
int currentTableNos[] = { -1, -1, -1, -1, -1}; // Initialize currentTableNos with -1 values
int lastTableNo;

int getCurrentTableNo() {
  int arraySize = sizeof(currentTableNos) / sizeof(currentTableNos[0]);

  for (int i = 0; i < arraySize; i++) {
    if (currentTableNos[i] != -1) {
      lastTableNo = currentTableNos[i];
      currentTableNos[i] = -1;
      return lastTableNo;
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
  return lastTableNo;
}

void fullStop() {
  motor1.fullStop();
  motor2.fullStop();
}

void middleSpeed(bool backwards) {
  motor1.middleSpeed(backwards);
  motor2.middleSpeed(backwards);
}

void fullSpeed(bool backwards) {
  motor1.fullSpeed(backwards);
  motor2.fullSpeed(backwards);
}

int delaySeconds(int delayTime) {
  return delayTime * 1000;
}
