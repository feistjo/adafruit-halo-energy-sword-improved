/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#if not defined(_VARIANT_ARDUINO_DUE_X_) && not defined(_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#include <Adafruit_NeoPixel.h>

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    PIN                       Which pin on the Arduino is connected to the NeoPixels?
    NUMPIXELS                 How many NeoPixels are attached to the Arduino?
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE 1

#define PIN 6
#define NUMPIXELS 53
/*=========================================================================*/

class DualNeopixel : public Adafruit_NeoPixel
{
public:
  DualNeopixel(uint16_t n, int16_t pin1, int16_t pin2) : p1{n, pin1}, p2{n, pin2} {}

  void begin()
  {
    p1.begin();
    p2.begin();
  }

  void setPixelColor(uint16_t n, uint32_t c)
  {
    p1.setPixelColor(n, c);
    p2.setPixelColor(n, c);
  }

  void setPixelColor(bool pixel, uint16_t n, uint32_t c)
  {
    if (pixel)
    {
      p2.setPixelColor(n, c);
    }
    else
    {
      p1.setPixelColor(n, c);
    }
  }

  void show()
  {
    p1.show();
    p2.show();
  }

  void setBrightness(uint8_t b)
  {
    p1.setBrightness(b);
    p2.setBrightness(b);
  }

  inline uint16_t numPixels() const { return p1.numPixels(); }

private:
  Adafruit_NeoPixel p1;
  Adafruit_NeoPixel p2;
};

DualNeopixel pixel{NUMPIXELS, 6, 9}; // NeoPixel Object for Visor Strips
// Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, 6);

// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
// Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                              BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                              BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// A small helper
void error(const __FlashStringHelper *err)
{
  Serial.println(err);
  while (1)
    ;
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t *data, const uint32_t numBytes);

// function prototypes of functions declared later
void colorWipe(uint32_t c, uint8_t wait);
void larsonScanner(uint32_t c, uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);
void rainbowCycle(uint8_t wait);
uint32_t Wheel(byte WheelPos);

void StartColorWipe(uint32_t c, uint8_t wait);
void ProcessAnimationState();
bool ProcessColorWipe();
bool ProcessRotateColorWipe();

// the packet buffer
extern uint8_t packetbuffer[];

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
// additional variables

// Color
uint8_t red = 255;
uint8_t green = 255;
uint8_t blue = 255;
uint8_t animationState = 1;

unsigned int pos = 0, dir = 1; // Position, direction of "eye" for larson scanner animation

void setup(void)
{
  // while (!Serial);  // required for Flora & Micro
  delay(500);
  Serial.begin(9600);
  Serial.println("Starting neopixels");

  // turn off neopixel
  pixel.begin(); // This initializes the NeoPixel library.

  for (uint8_t i = 0; i < NUMPIXELS; i++)
  {
    pixel.setPixelColor(i, pixel.Color(0, 0, 0)); // off
  }
  // colorWipe(pixel.Color(255, 255, 255), 15);
  // colorWipe(pixel.Color(0, 0, 0), 15);
  // pixel.show();

  Serial.println(F("Adafruit Bluefruit Neopixel Color Picker Example"));
  Serial.println(F("------------------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));
  // ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Lightsaber"));

  if (!ble.begin(VERBOSE_MODE))
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println(F("OK!"));

  if (FACTORYRESET_ENABLE)
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset())
    {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();

  ble.verbose(false); // debug info is a little annoying after this point!

  /* Wait for connection */
  while (!ble.isConnected())
  {
    delay(500);
  }

  Serial.println(F("***********************"));

  // Set Bluefruit to DATA mode
  Serial.println(F("Switching to DATA mode!"));
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("***********************"));
}

enum class Mode
{
  Static,
  ColorWipes,
  RotateColorWipes,
  LarsonScanners //,
                 // EtCetera
};

Mode current_mode{Mode::Static};
Mode previous_mode{Mode::Static};

uint8_t num_color_wipe_colors{8};
uint32_t color_wipe_colors[] = {pixel.Color(114, 0, 255),
                                pixel.Color(0, 0, 0),
                                pixel.Color(0, 50, 255),
                                pixel.Color(0, 0, 0),
                                pixel.Color(0, 220, 255),
                                pixel.Color(0, 0, 0),
                                pixel.Color(255, 225, 255),
                                pixel.Color(0, 0, 0)};

uint32_t animation_loop_counter = 0;

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
  ProcessAnimationState();

  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len != 0)
  {

    /* Got a packet! */
    // printHex(packetbuffer, len);

    // Color
    if (packetbuffer[1] == 'C')
    {
      current_mode = Mode::Static;
      uint8_t red = packetbuffer[2];
      uint8_t green = packetbuffer[3];
      uint8_t blue = packetbuffer[4];
      Serial.print("RGB #");
      if (red < 0x10)
        Serial.print("0");
      Serial.print(red, HEX);
      if (green < 0x10)
        Serial.print("0");
      Serial.print(green, HEX);
      if (blue < 0x10)
        Serial.print("0");
      Serial.println(blue, HEX);

      for (uint8_t i = 0; i < NUMPIXELS; i++)
      {
        pixel.setPixelColor(i, pixel.Color(red, green, blue));
      }
      pixel.show(); // This sends the updated pixel color to the hardware.
    }

    // Buttons
    if (packetbuffer[1] == 'B')
    {

      uint8_t buttnum = packetbuffer[2] - '0';
      boolean pressed = packetbuffer[3] - '0';
      Serial.print("Button ");
      Serial.print(buttnum);
      animationState = buttnum;
      if (pressed)
      {
        Serial.println(" pressed");

        /*  if (animationState == 1)
         {
           larsonScanner(pixel.Color(255, 255, 255), 20);
           larsonScanner(pixel.Color(0, 255, 255), 20);
           larsonScanner(pixel.Color(0, 100, 255), 20);
           larsonScanner(pixel.Color(0, 50, 255), 20);
           pixel.show(); // This sends the updated pixel color to the hardware.
         } */

        if (animationState == 2)
        {
          current_mode = Mode::ColorWipes;
          animation_loop_counter = 0;
          StartColorWipe(color_wipe_colors[animation_loop_counter], 30);
        }

        /* if (animationState == 3)
        {
          for (uint16_t i = 0; i < pixel.numPixels(); i++)
          {
            pixel.setPixelColor(i, pixel.Color(0, 0, 0));
          }
          pixel.setBrightness(255);
          theaterChase(255, 30);
          theaterChase(255, 40);
          theaterChase(255, 50);
          theaterChase(255, 60);
          theaterChase(255, 70);
          theaterChase(255, 80);
          theaterChase(255, 90);
          theaterChase(255, 100);
          colorWipe(pixel.Color(0, 0, 255), 20);
          colorWipe(pixel.Color(0, 0, 0), 20);
          pixel.show(); // This sends the updated pixel color to the hardware.
        }

        if (animationState == 4)
        {
          for (uint16_t i = 0; i < pixel.numPixels(); i++)
          {
            pixel.setPixelColor(i, pixel.Color(0, 0, 0));
          }
          pixel.setBrightness(255);
          rainbowCycle(10);
          pixel.show(); // This sends the updated pixel color to the hardware.
        } */

        if (animationState == 6) // pause
        {
          if (current_mode != Mode::Static)
          {
            previous_mode = current_mode;
            current_mode = Mode::Static;
          }
        }

        if (animationState == 5) // resume
        {
          if (current_mode == Mode::Static)
          {
            current_mode = previous_mode;
          }
        }

        if (animationState == 8)
        {
          current_mode = Mode::RotateColorWipes;
          animation_loop_counter = 0;
          StartColorWipe(color_wipe_colors[animation_loop_counter], 30);
        }
      }
      else
      {
        Serial.println(" released");
      }
    }
  }
}

void ProcessAnimationState()
{
  switch (current_mode)
  {
  case Mode::ColorWipes:
    if (ProcessColorWipe())
    {
      animation_loop_counter++;
      if (animation_loop_counter >= num_color_wipe_colors)
      {
        animation_loop_counter = 0;
      }
      StartColorWipe(color_wipe_colors[animation_loop_counter], 30);
    }
    // do for other animations
    break;
  case Mode::RotateColorWipes:
    if (ProcessRotateColorWipe())
    {
      animation_loop_counter++;
      if (animation_loop_counter >= num_color_wipe_colors)
      {
        animation_loop_counter = 0;
      }
      StartColorWipe(color_wipe_colors[animation_loop_counter], 30);
    }
    break;
  default:
    break;
  }
}
// Fill the dots one after the other with a color

uint32_t pixel_number{0};
uint32_t last_frame_time;
uint32_t wait_time;
uint32_t color;

void StartColorWipe(uint32_t c, uint8_t wait)
{
  pixel_number = 0;
  last_frame_time = millis();
  wait_time = wait;
  color = c;
}

// returns whether or not the colorwipe is fiished
bool ProcessColorWipe()
{
  if (pixel_number < pixel.numPixels())
  {
    if (millis() - last_frame_time >= wait_time) // if current time is after when the current loop iteration should be over
    {
      if (millis() - last_frame_time <= 2 * wait_time)
      {
        last_frame_time += wait_time;
      }
      else
      {
        last_frame_time = millis();
      }
      pixel.setPixelColor(pixel_number, color);
      pixel.show();
      pixel_number++; // now on the next loop iteration
    }
    return false;
  }
  else
  {
    return true;
  }
}

// returns whether or not the colorwipe is fiished
bool ProcessRotateColorWipe()
{
  if (pixel_number < pixel.numPixels())
  {
    if (millis() - last_frame_time >= wait_time) // if current time is after when the current loop iteration should be over
    {
      if (millis() - last_frame_time <= 2 * wait_time)
      {
        last_frame_time += wait_time;
      }
      else
      {
        last_frame_time = millis();
      }
      pixel.setPixelColor(0, pixel_number, color);
      pixel.setPixelColor(1, pixel.numPixels() - pixel_number, color);
      pixel.show();
      pixel_number++; // now on the next loop iteration
    }
    return false;
  }
  else
  {
    return true;
  }
}

// void colorWipe(uint32_t c, uint8_t wait)
// {
//   for (uint16_t i = 0; i < pixel.numPixels(); i++)
//   {
//     pixel.setPixelColor(i, c);
//     pixel.show();
//     delay(wait);
//   }
// }

void larsonScanner(uint32_t c, uint8_t wait)
{
  int j;

  for (uint16_t i = 0; i < pixel.numPixels() + 5; i++)
  {
    // Draw 5 pixels centered on pos.  setPixelColor() will clip any
    // pixels off the ends of the strip, we don't need to watch for that.
    pixel.setPixelColor(pos - 2, 0x003b85); // Dark red
    pixel.setPixelColor(pos - 1, 0x005ed2); // Medium red
    pixel.setPixelColor(pos, 0x00c0ff);     // Center pixel is brightest
    pixel.setPixelColor(pos + 1, 0x005ed2); // Medium red
    pixel.setPixelColor(pos + 2, 0x003b85); // Dark red

    pixel.show();
    delay(wait);

    // Rather than being sneaky and erasing just the tail pixel,
    // it's easier to erase it all and draw a new one next time.
    for (j = -2; j <= 2; j++)
      pixel.setPixelColor(pos + j, 0);

    // Bounce off ends of strip
    pos += dir;
    if (pos < 0)
    {
      pos = 1;
      dir = -dir;
    }
    else if (pos >= pixel.numPixels())
    {
      pos = pixel.numPixels() - 2;
      dir = -dir;
    }
  }
  // colorWipe(pixel.Color(0, 0, 0), 20);
}

void flashRandom(int wait, uint8_t howmany)
{

  for (uint16_t i = 0; i < howmany; i++)
  {
    // get a random pixel from the list
    int j = random(pixel.numPixels());

    // now we will 'fade' it in 5 steps
    for (int x = 0; x < 5; x++)
    {
      int r = red * (x + 1);
      r /= 5;
      int g = green * (x + 1);
      g /= 5;
      int b = blue * (x + 1);
      b /= 5;

      pixel.setPixelColor(j, pixel.Color(r, g, b));
      pixel.show();
      delay(wait);
    }
    // & fade out in 5 steps
    for (int x = 5; x >= 0; x--)
    {
      int r = red * x;
      r /= 5;
      int g = green * x;
      g /= 5;
      int b = blue * x;
      b /= 5;

      pixel.setPixelColor(j, pixel.Color(r, g, b));
      pixel.show();
      delay(wait);
    }
  }
  // LEDs will be off when done (they are faded to 0)
}

void rainbow(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256; j++)
  {
    for (i = 0; i < pixel.numPixels(); i++)
    {
      pixel.setPixelColor(i, Wheel((i + j) & 255));
    }
    pixel.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++)
  { // 5 cycles of all colors on wheel
    for (i = 0; i < pixel.numPixels(); i++)
    {
      pixel.setPixelColor(i, Wheel(((i * 256 / pixel.numPixels()) + j) & 255));
    }
    pixel.show();
    delay(wait);
  }
}

// Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait)
{
  for (int j = 0; j < 10; j++)
  { // do 10 cycles of chasing
    for (int q = 0; q < 3; q++)
    {
      for (uint8_t i = 0; i < pixel.numPixels(); i = i + 3)
      {
        pixel.setPixelColor(i + q, c); // turn every third pixel on
      }
      pixel.show();

      delay(wait);

      for (uint8_t i = 0; i < pixel.numPixels(); i = i + 3)
      {
        pixel.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait)
{
  for (int j = 0; j < 256; j++)
  { // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++)
    {
      for (uint8_t i = 0; i < pixel.numPixels(); i = i + 3)
      {
        pixel.setPixelColor(i + q, Wheel((i + j) % 255)); // turn every third pixel on
      }
      pixel.show();

      delay(wait);

      for (uint8_t i = 0; i < pixel.numPixels(); i = i + 3)
      {
        pixel.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return pixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return pixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
