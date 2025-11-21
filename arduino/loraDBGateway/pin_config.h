// --- Pin Configuration for ESP32 Feather V2 / ESP32-S3 Feather and RFM9x FeatherWing ---
// The ESP32 communicates with the RFM9x module using the Serial Peripheral Interface (SPI).


// Chip Select (CS): Must be a GPIO pin. D10 is the Adafruit standard.
#define RFM95_CS 33 

// Interrupt Request (IRQ/DIO0): This pin signals the ESP32 when a packet is received or sent.
// D3 is the Adafruit standard for the FeatherWing IRQ line.
#define RFM95_INT 12 

// Reset (RST): This pin is used to hardware-reset the radio module.
// D4 is the Adafruit standard for the FeatherWing Reset line.
#define RFM95_RST 32 

// Note on Pin Definitions:
// These pin numbers (10, 3, 4) correspond to the silkscreen labels on the Adafruit Feather/FeatherWing stack.
// The ESP32 automatically maps these logical pins to the correct physical GPIO pins for SPI communication (D11, D12, D13).
// If you are using a different ESP32 Feather or a different LoRa library, you may need to adjust these definitions.