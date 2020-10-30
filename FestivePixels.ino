#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

#ifndef STASSID
#define STASSID "taconet"
#define STAPSK  "nobeansplease"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

const char* www_username = "admin";
const char* www_password = "stnick";

#include <FastLED.h>

#define NUM_LEDS 300
#define DATA_PIN 2
#define BUTTON_PIN 5
#define FASTLED_ALLOW_INTERRUPTS 0

#define FRAMES_PER_SECOND 10
#define PATTERN_DURATION 30

// LEDs
int patternIndex = 0;
int step = 0;
int stepDirection = 1;
bool patternReady = false;
int baseHue = 0;
int brightness = 255;
unsigned long lastChangedMillis;

CRGB leds[NUM_LEDS];

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
void handleChange();

void blink(int pin) {
  digitalWrite(pin, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(250);                       // wait for a second
  digitalWrite(pin, HIGH);    // turn the LED off by making the voltage LOW  
}

void setup() {

  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Connect Failed! Rebooting...");
    delay(1000);
    ESP.restart(); 
  }
  ArduinoOTA.begin();

  server.on("/change", handleChange);
  server.on("/", handleForm);
  server.begin();  

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ in your browser to see it working");
      
  pinMode(LED_BUILTIN, OUTPUT);
//  pinMode(BUTTON_PIN, INPUT_PULLUP);
//  pinMode(POT_PIN,INPUT);
  
  blink(LED_BUILTIN);
   
//  button.setEventHandler(handleEvent);

  lastChangedMillis = millis();
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 
  FastLED.setBrightness(brightness);
  FastLED.show();
  Serial.println("Starting show!");
}

typedef void (*PatternList[])();
PatternList patterns = {
  holly,
  strand_march,
  candycane,
  strand_single,
  fireworks,  
  fadeWithGlitter,  
};

void handleChange() {
  digitalWrite(LED_BUILTIN, LOW);
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  String message = "POST form was:\n";
  if (server.hasArg("brightness")) {
    int newBrightness = server.arg("brightness").toInt();
    if (newBrightness > 255) {
      brightness = 255;
    } else {
      brightness = newBrightness;
    }
    Serial.print("Changing brightness to: ");
    Serial.println(brightness);
    FastLED.setBrightness(brightness);
  }
  if (server.hasArg("pattern")) {
    int newPattern = server.arg("pattern").toInt();
    setPattern(newPattern);
    Serial.print("Changing pattern to: ");
    Serial.println(newPattern);    
  }
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");  
  digitalWrite(LED_BUILTIN, HIGH);
}

void handleForm() {
  String form;
  char data[1000];
  sprintf(data, R"raw(
  <html>
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body>
      <form method='post' action='/change'>
        Brightness: <input type='text' name='brightness' value='%d'><br/>
        Pattern:<br/>
        <input type='radio' name='pattern' value='0' %s> Holly<br/>
        <input type='radio' name='pattern' value='1' %s> All March<br/>        
        <input type='radio' name='pattern' value='2' %s> Candy Cane<br/>
        <input type='radio' name='pattern' value='3' %s> Single March<br/>
        <input type='radio' name='pattern' value='4' %s> Fireworks<br/>
        <input type='radio' name='pattern' value='5' %s> Glitter Fade<br/>
        <input type='submit' value='Change'>
      </form>
    </body>
  </html>
  )raw",
  brightness,
  patternIndex == 0 ? "checked" : "",
  patternIndex == 1 ? "checked" : "",
  patternIndex == 2 ? "checked" : "",
  patternIndex == 3 ? "checked" : "",
  patternIndex == 4 ? "checked" : "",
  patternIndex == 5 ? "checked" : ""
  );
  
  server.send(200,"text/html",data);
}

void nextPattern() {
  setPattern((patternIndex + 1) % ARRAY_SIZE(patterns));
}

void setPattern(int newPattern) {
  patternIndex = newPattern;


  // reset counters
  step = 0;
  stepDirection = 1;
  patternReady = false;
  baseHue = 0;

  blink(LED_BUILTIN);
  Serial.print("Changing to pattern ");
  Serial.println(patternIndex);
  lastChangedMillis = millis();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();  

  patterns[patternIndex]();
  if (millis() - lastChangedMillis >= (PATTERN_DURATION * 1000)) {
    nextPattern();
  }

//  
//  int potValue = analogRead(POT_PIN);
//  if (potValue != lastPotValue) {
//    lastPotValue = potValue;
//    double newBrightness = 255 * (potValue / 1023.0); 
//    FastLED.setBrightness(newBrightness);
//  }
    
//  button.check();
}

//void handleEvent(AceButton* /*button*/, uint8_t eventType, uint8_t /*buttonState*/) {
//  switch (eventType) {
//    case AceButton::kEventPressed:
//      nextPattern();
//      break;
//  }
//}

void fadeWithGlitter() {
  
  if (step == 0) {
    if (baseHue == 0) {
      baseHue = 96;
    } else if (baseHue == 96) {
      baseHue = 160;
    } else {
      baseHue = 0;
    }
  }  
  int v = ease8InOutCubic(step);
  fill_solid(leds,NUM_LEDS,CHSV(baseHue,255,v)); 
  
  addGlitter(50);
  
  FastLED.delay(10);
  step = step + stepDirection;
  flipDirectionAfter(255);

}

void fireworks() {
  int step = 0;
  int range;
  if (!patternReady) {
    FastLED.clear();
    FastLED.show();
    patternReady = true;
  }

  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  switch (pos % 3){
    case 0: 
      baseHue = 8;
      range = 8;
      break;
    case 1:
      baseHue = 96;
      
      break;
    case 2:
      baseHue = 160;
      break;
  }
  leds[pos] += CHSV(baseHue + 4 - random8(8), 255, 255);

  FastLED.show();
  if (baseHue == 8) {
    baseHue = 96;
  } else {
    baseHue = 8;
  }
  FastLED.delay(50);
}

void strand_single() {
  
  for (int led = 0; led < NUM_LEDS; led++) {
    int ledColor = led % 5;
    int highlightedColor;
    
    if (stepDirection == 1) {
      highlightedColor = step % 5;
    } else {
      highlightedColor = 4 - (step % 5);
    }
    
    if (ledColor == highlightedColor) {
      switch (ledColor) {
        case 0:
          leds[led] = CRGB::Red;
          break;
        case 1:
          leds[led] = CRGB::Green;
          break;
        case 2:
          leds[led] = CRGB::Blue;
          break;
        case 3:
          leds[led] = CRGB::OrangeRed;
          break;
        case 4:
          leds[led] = CRGB::Purple;
          break;
      }
    } else {
      leds[led] = CRGB::Black;
    }
  }
  
  FastLED.show();
  FastLED.delay(100);
  step++;
  flipDirectionAfter(50);
}

void strand_march() {
  for (int led = 0; led < NUM_LEDS; led++) {
    int ledColor = led % 5;
    
    switch (ledColor) {
      case 0:
        leds[led] = CRGB::Red;
        break;
      case 1:
        leds[led] = CRGB::Green;
        break;
      case 2:
        leds[led] = CRGB::Blue;
        break;
      case 3:
        leds[led] = CRGB::OrangeRed;
        break;
      case 4:
        leds[led] = CRGB::Purple;
        break;
    }
    if (led != step && led != (NUM_LEDS - 1 - step)) {
      leds[led] %= 16;
    }    
  }

  
  FastLED.show();
  FastLED.delay(100);
  step = step + stepDirection;
  flipDirectionAtEnd();
}

void candycane() {
  for (int led = 0; led <= NUM_LEDS; led++) {
    int ledColor = led % 10;
    int ledOffset = (led + step) % NUM_LEDS;

    switch (ledColor) {
      case 0:
      case 1:
      case 2:
        leds[ledOffset] = CRGB::White;
        break;
      case 3:
      case 4:
      case 5:
        leds[ledOffset] = CRGB::Red;
        break;
      case 6:
        leds[ledOffset] = CRGB::White;
        break;
      case 7:
        leds[ledOffset] = CRGB::Red;
        break;
      case 8:
        leds[ledOffset] = CRGB::White;
        break;
      case 9:
        leds[ledOffset] = CRGB::Red;
        break;
    }
  }
  FastLED.show();
  FastLED.delay(100);
  step++;
}

void holly() {
  for (int led = 0; led <= NUM_LEDS; led++) {
    int ledColor = led % 10;
    int ledOffset = (led + step) % NUM_LEDS;

    switch (ledColor) {
      case 0:
      case 1:
      case 2:
        leds[ledOffset] = CRGB::White;
        break;
      case 3:
      case 4:
      case 5:
        leds[ledOffset] = CRGB::Green;
        break;
      case 6:
        leds[ledOffset] = CRGB::Red;
        break;
      case 7:
      case 8:
      case 9:
        leds[ledOffset] = CRGB::Green;
        break;
    }
  }
  FastLED.show();
  FastLED.delay(100);
  step++;
}

void addGlitter( fract8 chanceOfGlitter) {
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void flipDirectionAtEnd() {
  if (step == 0) {
    stepDirection = 1;
  } else if (step == NUM_LEDS - 1){
    stepDirection = -1;
  }
}

void flipDirectionAfter(int flipAfter) {

  if (step % flipAfter == 0) {
    if (stepDirection == 1) {
      stepDirection = -1;
    } else {
      stepDirection = 1;  
    }
  }
}
