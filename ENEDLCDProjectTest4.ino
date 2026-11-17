
#include <Wire.h>
#include <LiquidCrystal.h>
#include "RTClib.h"
#include <Keypad.h>
#include <EEPROM.h>
#include <string.h>

// =====================================
// HARDWARE CONFIGURATION
// =====================================

// Note frequencies (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_Eb4 311
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_Bb4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_Eb5 622
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_Bb5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_G6  1568
#define NOTE_A6  1760

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// RTC
RTC_DS3231 rtc;

// Passive buzzer
const byte BUZZER_PIN = A2;

// Keypad
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Assumed keypad wiring order
byte rowPins[ROWS] = {6, 7, 8, 9};
byte colPins[COLS] = {10, A3, A0, A1};

Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);

// =====================================
// TASK STORAGE
// =====================================

const byte MAX_TASKS = 12;
const uint16_t EEPROM_MAGIC = 0xBEEF;

struct Task {
  char name[17];
  byte hour;
  byte minute;
  byte enabled;

  uint16_t lastYear;
  byte lastMonth;
  byte lastDay;
};

Task tasks[MAX_TASKS];

// =====================================
// EEPROM
// =====================================

// Play a single note
void playNote(int frequency, int durationMs) {
  if (frequency > 0) {
    tone(BUZZER_PIN, frequency);
  }
  delay(durationMs);
  noTone(BUZZER_PIN);
  delay(30);  // short gap between notes
}

// Simplified opening of Peter's Theme (recognizable on a piezo)
void playPeterTheme() {
  // Opening phrase — closer to the real rhythm
  playNote(NOTE_G5, 280);   // longer
  playNote(NOTE_C6, 220);   // shorter
  playNote(NOTE_E6, 140);
  playNote(NOTE_G6, 180);
  playNote(NOTE_A6, 180);
  playNote(NOTE_G6, 220);
  playNote(NOTE_E6, 140);   // longer

  playNote(NOTE_G5, 180);
  playNote(NOTE_A5, 180);
  playNote(NOTE_B5, 220);
  playNote(NOTE_C6, 140);
  playNote(NOTE_G5, 180);
  playNote(NOTE_E5, 180);   // longer

  playNote(NOTE_C5, 180);
  playNote(NOTE_D5, 180);
  playNote(NOTE_Eb5, 280);
  playNote(NOTE_Eb5, 180);
  playNote(NOTE_B5, 180);
  playNote(NOTE_Eb5, 280);
  playNote(NOTE_Eb5, 180);
  playNote(NOTE_B5, 180);

  playNote(NOTE_Eb5, 200);
  playNote(NOTE_Bb5, 180);
  playNote(NOTE_Bb5, 320);

  // Ending
  playNote(NOTE_C6, 200);
  playNote(NOTE_C6, 200);
  playNote(NOTE_G5, 200);
  playNote(NOTE_C6, 420);
}

void saveTasks() {
  EEPROM.put(0, EEPROM_MAGIC);
  EEPROM.put(sizeof(EEPROM_MAGIC), tasks);
}

void loadTasks() {
  uint16_t storedMagic;

  EEPROM.get(0, storedMagic);

  if (storedMagic != EEPROM_MAGIC) {
    memset(tasks, 0, sizeof(tasks));
    saveTasks();
    return;
  }

  EEPROM.get(sizeof(EEPROM_MAGIC), tasks);

  for (byte i = 0; i < MAX_TASKS; i++) {
    tasks[i].name[16] = '\0';

    if (tasks[i].hour > 23 ||
        tasks[i].minute > 59 ||
        tasks[i].enabled > 1 ||
        (tasks[i].enabled &&
         tasks[i].name[0] == '\0')) {

      memset(&tasks[i], 0, sizeof(Task));
    }
  }
}

// =====================================
// LCD DISPLAY HELPERS
// =====================================

void showMessage(const char* line1, const char* line2) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// =====================================
// TIME DISPLAY
// =====================================

void printTime(byte hour, byte minute) {
  byte displayHour = hour % 12;

  if (displayHour == 0) displayHour = 12;

  if (displayHour < 10) lcd.print('0');

  lcd.print(displayHour);
  lcd.print(':');

  if (minute < 10) lcd.print('0');

  lcd.print(minute);
  lcd.print(hour < 12 ? " AM" : " PM");
}

void displayClock(DateTime now) {
  lcd.setCursor(0, 0);

  if (now.month() < 10) lcd.print('0');
  lcd.print(now.month());
  lcd.print('/');

  if (now.day() < 10) lcd.print('0');
  lcd.print(now.day());

  lcd.print('/');
  lcd.print(now.year());

  lcd.print(" ");

  lcd.setCursor(0, 1);
  lcd.print("Time: ");

  printTime(now.hour(), now.minute());
}

// =====================================
// BUZZER
// =====================================

void beep(unsigned int duration = 200) {
  tone(BUZZER_PIN, 2000);
  delay(duration);
  noTone(BUZZER_PIN);
}

void playInitialAlarm() {
  for (byte i = 0; i < 3; i++) {
    beep(250);
    delay(200);
  }
}

void playCompletionSound() {
  beep(100);
  delay(100);
  beep(250);
}

// =====================================
// DATE VALIDATION
// =====================================

bool isLeapYear(uint16_t year) {
  return (year % 4 == 0 &&
         (year % 100 != 0 || year % 400 == 0));
}

byte daysInMonth(byte month, uint16_t year) {
  const byte days[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
  };

  if (month < 1 || month > 12) return 0;

  if (month == 2 && isLeapYear(year)) {
    return 29;
  }

  return days[month - 1];
}

// =====================================
// TASK COMPLETION CHECK
// =====================================

bool completedToday(byte index, DateTime now) {
  return (
    tasks[index].lastYear == now.year() &&
    tasks[index].lastMonth == now.month() &&
    tasks[index].lastDay == now.day()
  );
}

// =====================================
// FIND OVERDUE TASK
// =====================================

int findDueTask(DateTime now) {
  int currentMinutes =
    now.hour() * 60 + now.minute();

  int selected = -1;
  int earliestTime = 1441;

  for (byte i = 0; i < MAX_TASKS; i++) {

    if (!tasks[i].enabled) continue;

    if (completedToday(i, now)) continue;

    int taskMinutes =
      tasks[i].hour * 60 + tasks[i].minute;

    if (taskMinutes <= currentMinutes &&
        taskMinutes < earliestTime) {

      selected = i;
      earliestTime = taskMinutes;
    }
  }

  return selected;
}

// =====================================
// ENTER FOUR-DIGIT TIME
// =====================================

bool enterTime(byte &hour, byte &minute) {
  char input[5] = "";
  byte position = 0;

  lcd.clear();
  lcd.print("Enter time HHMM");
  lcd.setCursor(0, 1);

  while (position < 4) {
    char key = keypad.getKey();

    if (key >= '0' && key <= '9') {
      input[position++] = key;
      lcd.print(key);
    }

    if (key == '*') return false;
  }

  input[4] = '\0';

  hour =
    (input[0] - '0') * 10 +
    (input[1] - '0');

  minute =
    (input[2] - '0') * 10 +
    (input[3] - '0');

  if (hour > 23 || minute > 59) {
    showMessage("Invalid time!", "Try again");
    delay(1500);
    return false;
  }

  return true;
}

// =====================================
// SET TIME
// =====================================

void setTime() {
  byte hour, minute;

  if (!enterTime(hour, minute)) return;

  DateTime now = rtc.now();

  rtc.adjust(DateTime(
    now.year(),
    now.month(),
    now.day(),
    hour,
    minute,
    0
  ));

  showMessage("Time updated!", "Clock saved");
  delay(1500);
}

// =====================================
// SET DATE
// FORMAT: MMDDYYYY
// =====================================

void setDate() {
  char input[9] = "";
  byte position = 0;

  lcd.clear();
  lcd.print("Enter MMDDYYYY");
  lcd.setCursor(0, 1);

  while (position < 8) {
    char key = keypad.getKey();

    if (key >= '0' && key <= '9') {
      input[position++] = key;
      lcd.print(key);
    }

    if (key == '*') return;
  }

  input[8] = '\0';

  byte month =
    (input[0] - '0') * 10 +
    (input[1] - '0');

  byte day =
    (input[2] - '0') * 10 +
    (input[3] - '0');

  uint16_t year =
    (input[4] - '0') * 1000 +
    (input[5] - '0') * 100 +
    (input[6] - '0') * 10 +
    (input[7] - '0');

  if (year < 2000 || year > 2099 ||
      month < 1 || month > 12 ||
      day < 1 ||
      day > daysInMonth(month, year)) {

    showMessage("Invalid date!", "Try again");
    delay(1500);
    return;
  }

  DateTime now = rtc.now();

  rtc.adjust(DateTime(
    year,
    month,
    day,
    now.hour(),
    now.minute(),
    now.second()
  ));

  showMessage("Date updated!", "Calendar saved");
  delay(1500);
}

// =====================================
// CLOCK MENU
// =====================================

void setClock() {
  while (true) {
    lcd.clear();
    lcd.print("A:Set Time");
    lcd.setCursor(0, 1);
    lcd.print("B:Set Date");

    char key = 0;

    while (!key) {
      key = keypad.getKey();
    }

    if (key == 'A') {
      setTime();
    }
    else if (key == 'B') {
      setDate();
    }
    else if (key == '*') {
      return;
    }
  }
}

// =====================================
// CUSTOM TASK NAME ENTRY
// =====================================

void enterCustomName(char* destination) {
  const char* letters[10] = {
    " ",        // 0 = space
    ".,!?1",    // 1
    "ABC2",     // 2
    "DEF3",     // 3
    "GHI4",     // 4
    "JKL5",     // 5
    "MNO6",     // 6
    "PQRS7",    // 7
    "TUV8",     // 8
    "WXYZ9"     // 9
  };

  char buffer[17] = "";
  byte length = 0;

  char pending = 0;
  char lastKey = 0;
  byte letterPosition = 0;
  unsigned long lastPress = 0;

  lcd.clear();
  lcd.print("Name: #=Save *=Del");

  while (true) {
    char key = keypad.getKey();

    // Auto-commit after 1 second of no input
    if (pending && (millis() - lastPress > 1000)) {
      if (length < 16) {
        buffer[length++] = pending;
        buffer[length] = '\0';
      }
      pending = 0;
      lastKey = 0;
    }

    if (key) {
      // Save name
      if (key == '#') {
        if (pending && length < 16) {
          buffer[length++] = pending;
        }
        buffer[length] = '\0';
        strncpy(destination, buffer, 16);
        destination[16] = '\0';
        return;
      }

      // Delete last character
      if (key == '*') {
        pending = 0;
        lastKey = 0;
        if (length > 0) {
          length--;
          buffer[length] = '\0';
        }
      }

      // Letter entry
      else if (key >= '0' && key <= '9') {
        byte number = key - '0';

        // Same key pressed again within timeout → cycle letters
        if (pending && key == lastKey && (millis() - lastPress < 1000)) {
          letterPosition++;
          if (letterPosition >= strlen(letters[number])) {
            letterPosition = 0;
          }
        }
        else {
          // Commit previous pending letter
          if (pending && length < 16) {
            buffer[length++] = pending;
            buffer[length] = '\0';
          }
          letterPosition = 0;
        }

        pending = letters[number][letterPosition];
        lastKey = key;
        lastPress = millis();
      }
    }

    // Update display
    lcd.setCursor(0, 1);
    lcd.print("                ");          // clear line
    lcd.setCursor(0, 1);
    lcd.print(buffer);

    // Show the character currently being selected (with brackets so it’s obvious)
    if (pending && length < 16) {
      lcd.print('[');
      lcd.print(pending);
      lcd.print(']');
    }
  }
}

// =====================================
// ADD TASK
// =====================================

void addTask() {
  int slot = -1;

  for (byte i = 0; i < MAX_TASKS; i++) {
    if (!tasks[i].enabled) {
      slot = i;
      break;
    }
  }

  if (slot == -1) {
    showMessage("Task limit!", "Delete a task");
    delay(1500);
    return;
  }

  showMessage("Choose task:", "1-4 or 5 custom");

  char key = 0;

  while (!key) {
    key = keypad.getKey();
  }

  char taskName[17] = "";

  switch (key) {
    case '1':
      strcpy(taskName, "Diaper change");
      break;

    case '2':
      strcpy(taskName, "Feeding");
      break;

    case '3':
      strcpy(taskName, "Nap time");
      break;

    case '4':
      strcpy(taskName, "Bath time");
      break;

    case '5':
      enterCustomName(taskName);
      break;

    default:
      showMessage("Invalid choice", "Try again");
      delay(1200);
      return;
  }

  if (strlen(taskName) == 0) return;

  byte hour, minute;

  if (!enterTime(hour, minute)) return;

  strcpy(tasks[slot].name, taskName);

  tasks[slot].hour = hour;
  tasks[slot].minute = minute;
  tasks[slot].enabled = 1;

  tasks[slot].lastYear = 0;
  tasks[slot].lastMonth = 0;
  tasks[slot].lastDay = 0;

  saveTasks();

  showMessage("Task saved!", taskName);
  delay(1500);
}

// =====================================
// VIEW TASKS
// =====================================

void viewTasks() {
  bool found = false;

  for (byte i = 0; i < MAX_TASKS; i++) {
    if (!tasks[i].enabled) continue;

    found = true;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(i + 1);
    lcd.print(": ");
    lcd.print(tasks[i].name);

    lcd.setCursor(0, 1);
    printTime(tasks[i].hour, tasks[i].minute);
    lcd.print(" #=Next");

    while (true) {
      char key = keypad.getKey();

      if (key == '#') break;
      if (key == '*') return;
    }
  }

  if (!found) {
    showMessage("No saved tasks", "Press * to exit");

    while (keypad.getKey() != '*') {
    }
  }
}

// =====================================
// DELETE TASK
// =====================================

void deleteTask() {
  showMessage("Delete task 1-5", "* = Cancel");

  char key = 0;

  while (!key) {
    key = keypad.getKey();
  }

  if (key == '*') return;

  if (key < '1' || key > '5') return;

  byte index = key - '1';

  if (!tasks[index].enabled) {
    showMessage("No such task", "");
    delay(1200);
    return;
  }

  memset(&tasks[index], 0, sizeof(Task));

  saveTasks();

  showMessage("Task deleted!", "");
  delay(1200);
}

// =====================================
// TASK REMINDER
// =====================================

void displayReminder(byte index) {
  lcd.clear();

  lcd.setCursor(0, 0);
  printTime(tasks[index].hour, tasks[index].minute);
  lcd.print(" DUE!");

  lcd.setCursor(0, 1);
  lcd.print(tasks[index].name);

  // Initial three-beep alarm
  playInitialAlarm();
  playPeterTheme();

  unsigned long lastAlarm = millis();
  const unsigned long alarmInterval = 2000;  // beep every 2 seconds (change this value as desired)

  while (true) {
    char key = keypad.getKey();

    // Done key pressed
    if (key == '#') {
      DateTime now = rtc.now();

      tasks[index].lastYear  = now.year();
      tasks[index].lastMonth = now.month();
      tasks[index].lastDay   = now.day();

      saveTasks();

      noTone(BUZZER_PIN);
      playCompletionSound();

      showMessage("Task completed!", tasks[index].name);
      delay(1200);
      return;
    }

    // Keep beeping until acknowledged
    if (millis() - lastAlarm >= alarmInterval) {
      beep(150);                 // shorter, more insistent beep
      lastAlarm = millis();
    }
  }
}

// =====================================
// MAIN MENU
// =====================================

void showMenu() {
  lcd.clear();
  lcd.print("A:Clock B:Add");
  lcd.setCursor(0, 1);
  lcd.print("C:View D:Delete");

  char key = 0;

  while (!key) {
    key = keypad.getKey();
  }

  switch (key) {
    case 'A':
      setClock();
      break;

    case 'B':
      addTask();
      break;

    case 'C':
      viewTasks();
      break;

    case 'D':
      deleteTask();
      break;
  }
}

// =====================================
// SETUP
// =====================================

void setup() {
  lcd.begin(16, 2);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  showMessage("Starting system", "Please wait...");

  if (!rtc.begin()) {
    showMessage("RTC Error!", "Check wiring");

    while (true) {
    }
  }

  // Only set time if RTC has lost power
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(
      F(__DATE__),
      F(__TIME__)
    ));
  }

  loadTasks();

  showMessage("System ready!", "Daily reminders");

  delay(1500);
  lcd.clear();
}

// =====================================
// MAIN LOOP
// =====================================

void loop() {
  static unsigned long lastUpdate = 0;

  DateTime now = rtc.now();

  // Check for overdue tasks
  int dueTask = findDueTask(now);

  if (dueTask >= 0) {
    displayReminder(dueTask);

    lcd.clear();
    lastUpdate = 0;

    return;
  }

  // Update clock once per second
  if (millis() - lastUpdate >= 1000UL) {
    lastUpdate = millis();
    displayClock(now);
  }

  // Check keypad
  char key = keypad.getKey();

  if (key == 'A' ||
      key == 'B' ||
      key == 'C' ||
      key == 'D') {

    showMenu();

    lcd.clear();
    lastUpdate = 0;
  }
}
