# Schematics

The add-on board is assembled from modules and through-hole components on an Arduino Uno prototype shield; it is not a custom PCB, so the schematic below is a circuit-level schematic without a PCB layout.

## `addon_board/` – circuit schematic of the sensor/display add-on board (KiCad 10)

`addon_board.kicad_sch` shows the add-on board and its connections to the STM32L433 (Fig. 2 of the laboratory handout). It covers:

- IC2: I2C temperature sensor (LM75 or MAX7500, address pins A2..A0 = GND, I2C address 0x90) on I2C3 (PC0 = SCL, PC1 = SDA) with 4.7 kΩ pull-ups R1/R2;
- J1: 0.91" 128×32 OLED module (SSD1306), supplied from PA12 (OLED_VCC);
- two analog voltage dividers excited from PA0 (ADC_DRIVE, also read on PA1): R5/R7 to PC2 (ADC_R5) and R6/R8 to PC3 (ADC_R6);
- LEDs D1/D2 (active low on PA9/PA10) and push button S1 on PA11.

IC1 (STM32L433RCT6P) is the microcontroller on the NUCLEO-L433RC-P board; only the pins used by the add-on board are wired, the rest are marked as not connected. IC1 uses a project symbol drawn in LQFP-64 package order (`addon_board.kicad_sym`, registered in the project `sym-lib-table`); all other symbols come from the standard KiCad libraries. Component values that depend on the board variant (R5–R8, IC2) are listed in `../bom/board_variants.csv` and in Table 2 of the laboratory handout.

The schematic has been verified against the laboratory boards and the STM32CubeMX pin configuration (`Lab2_DemoFirmware.ioc`).

Open `addon_board.kicad_pro` in KiCad 10 or later.
