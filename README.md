# Kids Routine Reminder

A simple, always-on reminder device that helps parents keep a steady daily routine for young children.

It shows the current date and time, beeps when a task is due, and keeps reminding you until you mark the task complete. Everything is controlled with a standard 4×4 membrane keypad and a small LCD screen.

## Features

- Shows the current date and time
- Up to 10 daily tasks
- Persistent storage (tasks survive power cycles)
- Tasks only remind you once per day
- Custom task names (multi-tap keypad entry)
- Predefined tasks: Diaper change, Feeding, Nap time, Bath time
- Alarm continues until acknowledged with the **#** key
- Simple menus for adding, viewing, and deleting tasks

## Keypad Layout

1	2	3	A
4	5	6	B
7	8	9	C
*	0	#	D

| Button | Function |
|--------|----------|
| **A** | Set clock (time or date) |
| **B** | Add a new task |
| **C** | View saved tasks |
| **D** | Delete a task |
| **#** | Mark task done / Save |
| **\*** | Back / Cancel / Delete character |

## Everyday Use

Most of the time the screen shows the date and time. When a task is due, the screen switches to the task name and the buzzer plays a short melody (Peter’s theme from *Peter and the Wolf*), then continues beeping until you press **#**.

## Adding a Task

1. Press **B**
2. Choose a task:
   - **1** — Diaper change
   - **2** — Feeding
   - **3** — Nap time
   - **4** — Bath time
   - **5** — Custom name
3. Enter the time as four digits (HHMM).  
   Examples: `0800` = 8:00 AM, `1730` = 5:30 PM

### Typing a Custom Name

Use the number keys like an old cell phone:

| Key | Letters |
|-----|---------|
| 2 | A B C 2 |
| 3 | D E F 3 |
| 4 | G H I 4 |
| 5 | J K L 5 |
| 6 | M N O 6 |
| 7 | P Q R S 7 |
| 8 | T U V 8 |
| 9 | W X Y Z 9 |
| 0 | Space |
| 1 | . , ! ? 1 |

- Press the same key repeatedly to cycle letters
- Wait ~1 second (or press another key) to lock the letter in
- **\*** deletes the last character
- **#** saves the name (max 16 characters)

## Setting the Time & Date

Press **A**, then:

- **A** again to set the time (HHMM)
- **B** to set the date (MMDDYYYY)

## Hardware

- Arduino (Uno / Nano)
- 16×2 LCD
- DS3231 RTC module
- Passive piezo buzzer
- 4×4 membrane keypad
- EEPROM for task storage

## License

This project is open source. Feel free to use, modify, and share.
