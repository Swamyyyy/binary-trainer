# Binary Trainer

An interactive Arduino learning device that teaches how binary numbers work. Four switches and four LEDs show the bit values 8, 4, 2 and 1, a 0.96" OLED displays the number and the result, and a timed quiz tests the learner with a score, reaction time and a saved high score.

![Circuit diagram](docs/circuit_diagram.svg)

## Objective

To build a low-cost, interactive electronic learning device that teaches students how binary numbers work, using LEDs, switches and an OLED display, and tests their understanding through a scored, timed quiz.

**Specific objectives**
1. Show how decimal numbers (0 to 15) are written in binary, using four LEDs for the bits 8, 4, 2 and 1.
2. Provide a **Learn mode** that displays each number with its binary pattern and the sum behind it, such as `10 = 8+2`.
3. Provide a **Reasoning mode** where the student converts a given number to binary using the bit inputs, with the LEDs showing their input.
4. Give instant feedback: a happy "wow" face and +1 point for a right answer, and a sad face with the correct answer for a wrong one or when time runs out.
5. Motivate repeated practice with a score, a reaction time and a high score that is saved when the power is off.
6. Demonstrate embedded-systems basics: a microcontroller reading inputs, driving LEDs and updating a display.

**Target users:** school students (about 6 to 17 years), beginners in electronics and computer science, and teachers who want a hands-on classroom demo.

## Features

- **Learn mode:** numbers 0 to 15 with the binary boxes, the matching LEDs and the sum (for example `10 = 8+2`). It moves to the next number every 2 seconds, or when the Learn button is pressed.
- **Reasoning mode:** a random number from 1 to 15 is shown and the learner sets the binary value within the time limit (5 seconds by default), with a countdown bar on the OLED.
- **Right answer:** wow face, "Right!" with the reaction time, and +1 point.
- **Wrong answer or time up:** sad face, the correct answer (also lit on the LEDs), then the score and whether a new high score was set.
- **High score** stored in the Nano's EEPROM, so it stays after power off.
- Works with momentary buttons (each press flips a bit) or real latching toggle switches.

## How to play

1. On the menu, press **Learn** to study numbers or **Reason** to start the quiz.
2. In the quiz, set the bit inputs to the binary value of the number shown (8, 4, 2, 1), then press **Reason** to check.
3. A right answer scores a point and shows the next number. A wrong answer, or running out of time, ends the run.
4. Press **Reason** to start a new run from 0, or **Learn** to go back to Learn mode.
5. To reset the high score, hold **both** control buttons while powering on.

## Components

| # | Component | Qty |
|---|---|---|
| 1 | Arduino Nano | 1 |
| 2 | 0.96" I2C OLED display (SSD1306, 128x64) | 1 |
| 3 | Switches or push buttons for the 4 bit inputs | 4 |
| 4 | Push buttons for Learn and Reason | 2 |
| 5 | LEDs | 4 |
| 6 | 220 ohm resistors | 4 |
| 7 | Breadboard | 1 |
| 8 | Jumper wires | About 25 |
| 9 | USB cable | 1 |
| 10 | Power bank (optional) | 1 |

Quick list: Arduino Nano, 0.96" I2C OLED display, 6 push buttons or switches, 4 LEDs, 4 resistors (220 ohm), breadboard, jumper wires, USB cable, power bank (optional).

## Wiring

All inputs use the Nano's internal pull-ups, so the switches and buttons need no resistors and no 5V wire. Each one has one terminal on its Nano pin and the other terminal on GND.

### Bit inputs

| Input | Bit value | Wire 1 | Wire 2 |
|---|---|---|---|
| Switch 1 | 8 | D2 | GND |
| Switch 2 | 4 | D3 | GND |
| Switch 3 | 2 | D4 | GND |
| Switch 4 | 1 | D5 | GND |

### Control buttons

| Button | Job | Wire 1 | Wire 2 |
|---|---|---|---|
| Learn | Learn mode | D7 | GND |
| Reason / Check | Reasoning mode, and checks the answer | D6 | GND |

### LEDs (each through a 220 ohm resistor)

| LED | Bit value | Connection |
|---|---|---|
| LED 1 | 8 | D9, then resistor, then LED long leg (+). Short leg (-) to GND |
| LED 2 | 4 | D10, then resistor, then LED long leg (+). Short leg (-) to GND |
| LED 3 | 2 | D11, then resistor, then LED long leg (+). Short leg (-) to GND |
| LED 4 | 1 | D12, then resistor, then LED long leg (+). Short leg (-) to GND |

### OLED (I2C)

| OLED pin | Connects to |
|---|---|
| GND | GND |
| VCC | 5V |
| SDA | A4 |
| SCL | A5 |

Check the pin names printed on your OLED, because some boards list VCC before GND.

### Nano pin summary

| Nano pin | Goes to |
|---|---|
| D2 | Bit 8 input |
| D3 | Bit 4 input |
| D4 | Bit 2 input |
| D5 | Bit 1 input |
| D6 | Reason / Check button |
| D7 | Learn button |
| D9 | LED 8 (via resistor) |
| D10 | LED 4 (via resistor) |
| D11 | LED 2 (via resistor) |
| D12 | LED 1 (via resistor) |
| A4 | OLED SDA |
| A5 | OLED SCL |
| 5V | OLED VCC |
| GND | All GND connections |

## Software setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. In **Tools, Manage Libraries**, install **U8g2** (by olikraus). `Wire` and `EEPROM` come with the IDE.
3. Select **Tools, Board: Arduino Nano**. If the upload fails, set **Processor: ATmega328P (Old Bootloader)**, which most clone Nanos need. Clone boards may also need the CH340 USB driver.
4. Open `binary_trainer/binary_trainer.ino`, select the port and upload.

If your OLED is a 1.3" SH1106 (the picture looks shifted or has noise at the edges), swap the constructor line at the top of the sketch for the SH1106 one that is already included as a comment.

## Configuration

These settings are at the top of `binary_trainer.ino`.

| Setting | Default | Meaning |
|---|---|---|
| `SW_BIT` | `{2, 3, 4, 5}` | Pins for the bit inputs (8, 4, 2, 1) |
| `LED_P` | `{9, 10, 11, 12}` | Pins for the LEDs (8, 4, 2, 1) |
| `BTN_LEARN` | `7` | Learn button pin |
| `BTN_REASON` | `6` | Reason / Check button pin |
| `MOMENTARY_BITS` | `true` | `true`: each press flips a bit and it stays. `false`: real latching toggle switches |
| `TIME_LIMIT_MS` | `5000` | Time allowed per question in milliseconds |

## Project structure

```
binary-trainer/
├── binary_trainer/
│   └── binary_trainer.ino     main program
├── tests/
│   ├── led_test/              checks the 4 LEDs
│   ├── input_test/            checks the 4 bit inputs and 2 buttons (Serial Monitor)
│   ├── oled_test/             checks the OLED and the control buttons
│   └── i2c_scanner/           finds the OLED address (usually 0x3C)
├── docs/
│   └── circuit_diagram.svg    wiring diagram
├── .gitignore
└── README.md
```

## Testing the hardware

Upload the test sketches in this order, so a fault is easy to find.

1. `tests/led_test`: the four LEDs light one by one, blink together, then count 0 to 15 in binary.
2. `tests/input_test`: open the Serial Monitor at 9600 baud and press every input. Each one prints a line, and the first four also light their LED.
3. `tests/oled_test`: the OLED shows "OLED works!" and the press counters for both control buttons.
4. Then upload `binary_trainer.ino`.

## Troubleshooting

| Problem | Fix |
|---|---|
| Upload fails | Select Board: Arduino Nano, try Processor: ATmega328P (Old Bootloader), check the port and the USB cable (some cables only charge). |
| OLED is blank | Check VCC on 5V, GND, SDA on A4 and SCL on A5. Run `tests/i2c_scanner` and look for `0x3C`. Try the SH1106 constructor. |
| An LED does not light | Check its direction (long leg toward the resistor and pin), the resistor, and that the short leg reaches GND. Test it with `tests/led_test`. |
| An input always reads pressed | The two terminals of a push button are on the same side. Use diagonal legs, or turn the button 90 degrees. |
| An input never reads pressed | One terminal is not on GND, or the wire is on the wrong pin. Check with `tests/input_test`. |
| Bits jump back to 0 after pressing | Your inputs are momentary, so keep `MOMENTARY_BITS = true`. Use `false` only with real latching toggle switches. |
| A toggle switch works backwards | On a 3-pin switch, move the GND wire to the other outer pin. |
| Learn and Reason are the wrong way round | Swap the values of `BTN_LEARN` and `BTN_REASON` in the sketch, or swap the two wires. |

## Enclosure tips

- Foam board is soft, so use panel-mount switches and buttons with a washer on both sides, or glue a small piece of cardboard behind each hole.
- Put each LED directly above its switch, and label the switches 8, 4, 2, 1.
- Use a different colour for the two control buttons, and label them LEARN and REASON.
- Power it from a power bank so no laptop cable shows in photos or videos.

## Ideas for upgrades

- A buzzer for right and wrong answers.
- Levels with different time limits, or a mode that uses 2-bit numbers first.
- Save the best reaction time with the high score.
- An 8-bit version with eight switches and LEDs.

## Notes

This is an educational demo and is not a certified teaching instrument.
