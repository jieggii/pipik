#include "Arduino.h"
#include "pins_arduino.h"

#include "LiquidCrystal_I2C.h"
#include "DHT.h"

#include "texts.h"

#define LCD_COLS 16
#define LCD_ROWS 2
#define LCD_ADDR 0x27

#define PIN_TRIGGER 11
#define PIN_RANDOM 0
#define PIN_DHT 7

#define DHT_TYPE DHT11

enum ArduinoState {
    /// Sensor values are displayed in the LCD.
    SENSOR,

    /// Random text is being displayed in the LCD.
    TEXT,
};

uint8_t PREV_TEXT_INDEX = 255;

const unsigned short SENSOR_DISPLAY_INTERVAL = 1000;
const unsigned short TEXT_DISPLAY_INTERVAL = 2000;

unsigned long SENSOR_LAST_DISPLAY_MILLIS = 0;
unsigned long TEXT_LAST_DISPLAY_MILLIS = 0;

ArduinoState STATE = ArduinoState::SENSOR;

LiquidCrystal_I2C LCD(LCD_ADDR, LCD_COLS, LCD_ROWS);
DHT DHT_SENSOR(PIN_DHT, DHT_TYPE);

/*
 * Fulfills a buffer with space (" ") characters, ensures proper 0-termination.
 * Example (considering that `length` is set to 16): "hello" will be fulfilled to "hello           ".
 * Note: length means "buffer size - 1".
 */
void fulfillRowBuffer(char *buffer, uint8_t length) {
    // find 0-terminator index:
    uint8_t terminator_index = 0;
    for (uint8_t i = 0; i <= length; i++) {
        if (buffer[i] == '\0') {
            terminator_index = i;
            break;
        }
    }
    // fulfill buffer with spaces:
    for (uint8_t i = terminator_index; i < length; i++) {
        buffer[i] = ' ';
    }
    // add 0-terminator in the end of the buffer
    buffer[length] = '\0';
}

/*
 * Initializes `row1` and `row2` buffers from `src` string. Ensures proper termination and fulfills buffers with spaces.
 * For example, initializes `row1` and `row2` as follows:
 * row1: 'hello world! 123'
 * row2: '4567890         '
 * from the string: 'hello world! 1234567890'.
 */
void initRowBuffers(const char* src, char* row1, char* row2, uint8_t row_length) {
    size_t src_len = strlen(src);
    bool use_row2 = true;
    if (src_len + 1 <= row_length) {
        use_row2 = false;
    }

    for (size_t i = 0; i < src_len + 1; i++) {
        if (i == row_length * 2) {
            row2[row_length] = '\0';
            break;
        }
        if (i < row_length) {
            row1[i] = src[i];
        } else if (i == row_length) {
            row1[row_length] = '\0';
            row2[0] = src[i];
        } else {
            row2[i - row_length] = src[i];
        }
    }

    if (!use_row2)  {
        row2[0] = '\0';
        fulfillRowBuffer(row1, row_length);
    }
    fulfillRowBuffer(row2, row_length);
}

void parse_float(float value, char *buffer) {
    dtostrf(value, 3, 1, buffer);
}

void printRows(char* row1, char* row2) {
    LCD.setCursor(0, 0);
    LCD.print(row1);
    LCD.setCursor(0, 1);
    LCD.print(row2);
}

void setup() {
    Serial.begin(9600);

    LCD.init();
    LCD.backlight();

    DHT_SENSOR.begin();

    randomSeed(analogRead(PIN_RANDOM));
}


void sensor_loop() {
    if (digitalRead(PIN_TRIGGER) == HIGH) {
        STATE = ArduinoState::TEXT;
        return;
    }

    unsigned long current_millis = millis();
    if (current_millis - SENSOR_LAST_DISPLAY_MILLIS >= SENSOR_DISPLAY_INTERVAL) {
        float temperature = DHT_SENSOR.readTemperature();
        char temperature_buffer[5 + 1];
        parse_float(temperature, temperature_buffer);

        float humidity = DHT_SENSOR.readHumidity();
        char humidity_buffer[5 + 1];
        parse_float(humidity, humidity_buffer);

        float heat_index = DHT_SENSOR.computeHeatIndex(temperature, humidity, false);
        char heat_index_buffer[5 + 1];
        parse_float(heat_index, heat_index_buffer);

        char row1[LCD_COLS + 1];
        char row2[LCD_COLS + 1];

        snprintf(
                row1, sizeof(row1), "%s%cC (%s%cC)", temperature_buffer, (char)223, heat_index_buffer, (char)223
        );
        fulfillRowBuffer(row1, LCD_COLS);

        snprintf(row2, sizeof(row2), "Humidity: %s%%", humidity_buffer);
        fulfillRowBuffer(row2, LCD_COLS);

        printRows(row1, row2);
        SENSOR_LAST_DISPLAY_MILLIS = current_millis;
    }
}

void text_loop() {
    if (digitalRead(PIN_TRIGGER) == LOW) {
        STATE = ArduinoState::SENSOR;
        return;
    }

    unsigned long current_millis = millis();
    if (current_millis - TEXT_LAST_DISPLAY_MILLIS >= TEXT_DISPLAY_INTERVAL) {
        // generate new text index:
        uint8_t text_index = random(0, TEXTS_COUNT);
        while (text_index == PREV_TEXT_INDEX) {
            text_index = random(0, TEXTS_COUNT);
        }
        PREV_TEXT_INDEX = text_index;

        char text[LCD_COLS * LCD_ROWS + 1];
        strncpy_P(text, (char *)pgm_read_ptr(&(TEXTS[text_index])), sizeof (text));

        char row1[LCD_COLS + 1];
        char row2[LCD_COLS + 1];
        initRowBuffers(text, row1, row2, LCD_COLS);
        printRows(row1, row2);

        TEXT_LAST_DISPLAY_MILLIS = current_millis;
    }
}

void loop() {
    switch (STATE) {
        case SENSOR:
            sensor_loop();
            break;
        case TEXT:
            text_loop();
            break;
    }
}