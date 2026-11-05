# Bill of materials

Bill of materials for one laboratory workstation: the sensor/display add-on board and the boards, cables and software it is used with. Component references match the schematic in `../schematics/addon_board/`. All add-on board components are through-hole (THT) except the I2C sensors (SO-8 or breakout module).

## Files

| File | Content |
|---|---|
| `addon_board_bom.csv` | Components of the add-on board with manufacturer part numbers and supplier links |
| `board_variants.csv` | Sensor and divider values fitted on each of the eight board variants |
| `workstation_bom.csv` | Nucleo board, power measurement board, USB cables and PC software |

## Add-on board

| Ref | Qty | Component | Manufacturer part | Supplier |
|---|---|---|---|---|
| IC2 | 1 | I2C temperature sensor, SO-8 (variants 1–6) | NXP LM75BD or MAX7500MSA+ | DigiKey |
| IC2 | 1 | I2C pressure sensor module (variants 7–8) | GY-68 BMP180 or Adafruit BMP280 (2651) | Opencircuit / RS Components |
| R1, R2 | 2 | 4.7 kΩ 5 % 1/4 W | Yageo CFR-25JB-52-4K7 | DigiKey |
| R5, R6 | 2 | 10 kΩ (variants 1, 2, 6) or 20 kΩ (variants 3, 4, 5, 7, 8), 1 % 1/4 W | Yageo MFR-25FBF52-10K / MFR-25FBF52-20K | DigiKey |
| R7, R8 | 2 | NTC thermistor probe 10 kΩ, B = 3435 K (variants 1, 2, 6) | TEWA Sensors TT05-10KC8-1S-T105-1500 | DigiKey |
| R7, R8 | 2 | Photoresistor 8–20 kΩ at 10 lux, γ = 0.7 (variants 3, 4, 5, 7, 8) | GL5528 | TinyTronics |
| J1 | 1 | OLED module 0.91" 128×32, SSD1306, I2C | 0.91" 128×32 I2C OLED, white | TinyTronics |
| J1-S | 1 | 1×4 female header 2.54 mm | Sullins PPTC041LFBN-RC | DigiKey |
| D1, D2 | 2 | LED 5 mm red | Kingbright WP7113ID | DigiKey |
| R3, R4 | 2 | 100 Ω 5 % 1/4 W | Yageo CFR-25JB-52-100R | DigiKey |
| S1 | 1 | Tactile switch 6×6 mm | Omron B3F-1000 | DigiKey |
| J2 | 2 | 2-pin header 2.5 mm (NTC variants) | JST B2B-XH-A | DigiKey |
| J2-P, J2-C | 2 + 4 | Plug housing and crimp contacts for the probe cables | JST XHP-2, SXH-001T-P0.6 | DigiKey |
| PCB1 | 1 | Prototype shield for Arduino Uno | Proto shield for Arduino Uno | Electrokit |
| PCB1-H | 1 | Arduino R3 stacking headers | Adafruit Shield stacking headers (R3) | Core Electronics |

Supplier links are given in `addon_board_bom.csv`. Equivalent parts with the same value and mounting can be used.

## Board variants

| Variant | IC2 | R5, R6 | R7, R8 |
|---|---|---|---|
| 1 | MAX7500MSA | 10 kΩ | NTC 10 kΩ |
| 2 | MAX7500MSA | 10 kΩ | NTC 10 kΩ |
| 3 | MAX7500MSA | 20 kΩ | Photoresistor 8–20 kΩ |
| 4 | LM75 | 20 kΩ | Photoresistor 8–20 kΩ |
| 5 | LM75 | 20 kΩ | Photoresistor 8–20 kΩ |
| 6 | LM75 | 10 kΩ | NTC 10 kΩ |
| 7 | BMP180 | 20 kΩ | Photoresistor 8–20 kΩ |
| 8 | BMP280 | 20 kΩ | Photoresistor 8–20 kΩ |

## Assembly notes

- **Firmware constants:** set `R_REF` in `main.c` to the fitted R5/R6 value (10e3 or 20e3) and `B_NTC` to the B value of the NTC probe.
- **OLED module:** connect the header according to the pin order printed on the module (VCC, GND, SDA, SCL for the listed module); VCC is supplied from PA12.
- **Sensor address:** LM75/MAX7500 address pins A2..A0 are tied to GND (I2C address 0x90).
- **BMP180/BMP280 variants:** the pressure sensors use their own register map and require a separate driver in the firmware.
