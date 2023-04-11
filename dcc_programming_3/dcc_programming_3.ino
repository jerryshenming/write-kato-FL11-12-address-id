// Configurations
// Output pin
#define PIN 40
// Output pin 13 as enable pin
#define PINofENABLE 13
// Output pin 12 as output for L298
#define PINofL298 11

// Time in microsecond
#define ZERO_TIME 100
#define ONE_TIME 50

// Whether to loop forever (repeat sequence)
bool loopForever = false;

// Write CV
int cv = 0b01111100;
int midSection = 0;
int newAddr = 6;

//---------------------------------------------

// Main
unsigned long previousTime = 0;
// Current state of pin
bool on = false;
// Value currently writing: false - 0, true - 1
bool state = false;
// Index of sequence currently reading
int currentReadingIndex = 0;
// Number of bits written for a sequence
int counter = 0;
// Whole program finished
bool finished = false;

// Program with the amount of digits per alteration
// For example:
// [20, 27, 10, 15] will output 20 0s, then 27 1s, then 10 0s, then 15 1s
//int sequence[] = {20, 26, 20, 26, 20, 26, 20, 26};
int sequence[600];
size_t currentSequenceIndex = 0;

void writePreamble(int count) {
  for (int i = 0; i < count; i++) {
    sequence[currentSequenceIndex++] = 21;
    sequence[currentSequenceIndex++] = 27;
  }
}

void writeSignalBlock() {
  sequence[currentSequenceIndex++] = 1;
  sequence[currentSequenceIndex++] = 0; // Pad section, no 1s will be written
}

void writeSignalSync() {
  sequence[currentSequenceIndex++] = 21; // Pad section, no 1s will be written
}


void writeBinary(int number) {
  int counter = 0;
  bool isOne = false;
  for (int i = 7; i >= 0; i--) {
    int nextBit = (number >> i) & 1;
    if (nextBit == 0) {
      // 0
      if (isOne) {
        sequence[currentSequenceIndex++] = counter;
        counter = 0;
        isOne = false;
      }
      counter += 1;
    } else {
      // 1
      if (!isOne) {
        sequence[currentSequenceIndex++] = counter;
        counter = 0;
        isOne = true;
      }
      counter += 1;
    }
  }
  sequence[currentSequenceIndex++] = counter;

  if (!isOne) {
    // Write 0 ones so that other functions don't get confused
    sequence[currentSequenceIndex++] = 0;
  }
}

void setup() {
  // put your setup code here, to run once:
  
  int testPosition;  
  
  Serial.begin(115200);
  Serial.println("Begin");

  // Initialization
  sequence[currentSequenceIndex++] = 100;
  //sequence[currentSequenceIndex++] = 0;

  writePreamble(150);

  writeSignalSync();
  writeSignalBlock();
  writeBinary(cv);

  testPosition = currentSequenceIndex % 2;
  if (testPosition == 1) {
    sequence[currentSequenceIndex++] = 0;
  }
  writeSignalBlock();
  writeBinary(midSection);

  testPosition = currentSequenceIndex % 2;
  if (testPosition == 1) {
    sequence[currentSequenceIndex++] = 0;
  }  
  writeSignalBlock();
  writeBinary(newAddr);
  
  testPosition = currentSequenceIndex % 2;
  if (testPosition == 1) {
    sequence[currentSequenceIndex++] = 0;
  }
  writeSignalBlock();
  // Since the middle block is 0, it doesn't make any difference
  // But if it ever isn't, it may need to be added here
  int checksum = cv ^ newAddr;
  writeBinary(checksum);

  testPosition = currentSequenceIndex % 2;
  if (testPosition == 0) {
    sequence[currentSequenceIndex++] = 0;
  }
  
  writePreamble(100);

  pinMode(PIN, OUTPUT);
  pinMode(PINofENABLE, OUTPUT);
  pinMode(PINofL298, OUTPUT);

  digitalWrite(PINofENABLE, LOW);
  //digitalWrite(PINofL298, LOW);
  digitalWrite(PINofL298, HIGH);

  noInterrupts();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (finished) { return; }

  // Check counter
  int currentMax = sequence[currentReadingIndex];

  // Only runs code if max is greater than 0
  if (currentMax > 0) {
    int waitTime = state ? ONE_TIME : ZERO_TIME;
    //digitalWrite(PIN, HIGH);
    digitalWrite(PINofENABLE, LOW);
    delayMicroseconds(waitTime);
    //digitalWrite(PIN, LOW);
    digitalWrite(PINofENABLE, HIGH);
    delayMicroseconds(waitTime-4); // Allow some time for the below code to run
  }

  // Increment counter
  counter++;
  if (counter >= currentMax) {

    currentReadingIndex++;
    counter = 0;
    state = !state;

    if (currentReadingIndex == currentSequenceIndex) {
      if (loopForever) {
        currentReadingIndex = 0;
      } else {
        finished = true;
      }
    }
  }
}
