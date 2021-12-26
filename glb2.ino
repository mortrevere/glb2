#include <WiFi.h>
#include <FastLED.h>
#define NUM_LEDS 240 //the number of LEDS we have in total
CRGBArray<NUM_LEDS> leds; //an array of LEDs, used by FastLED to set colors on each of them
#define ONBOARD_LED  2 //the ID of the onboard LED

const char* ssid = "beLow"; //wifi network name
const char* password = "belowpartout"; //wifi network password
const String ID_PREFIX = "BLW ";
const String ID_STRING = String(ID_PREFIX + WiFi.macAddress()); //unique identifier per globe

//each globe runs a tcp server on port 10000
//the client tries all ip adresses on the subnet and checks for an open port 10000
WiFiServer wifiServer(10000);

uint8_t hex_digit_to_int(unsigned char b) {
  //converts a single char to its hexadecimal integer value
  if (b >= '0' && b <= '9') b = b - '0';
  else if (b >= 'a' && b <= 'f') b = b - 'a' + 10;
  else if (b >= 'A' && b <= 'F') b = b - 'A' + 10;
  return b;
}

void decode_hex(char* input, unsigned char* out)
{
  //converts a whole hex string to its integer value, by pairs
  //0xFF -> 255
  char temp[3];
  int p = 0;
  int len = strlen(input);
  for (int i = 0; i < len; i += 2) {
    temp[0] = input[i];
    temp[1] = input[i + 1];
    temp[2] = '\0';



    //Serial.println(hex_digit_to_int(temp[0])*16 + hex_digit_to_int(temp[1]));
    out[p] = hex_digit_to_int(temp[0]) * 16 + hex_digit_to_int(temp[1]);
    Serial.println(out[p]);
    p++;
  }
}

//defines the current color for the globe
CRGB currentColor;

void setup() {

  currentColor = CRGB(255, 0, 0);

  //init wifi and sets the onboard led so we know if the globe is correctly connected to the wifi network
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH);
  delay(1000);
  FastLED.addLeds<NEOPIXEL, 25>(leds, NUM_LEDS);
  digitalWrite(ONBOARD_LED, LOW);
  Serial.begin(115200);
  digitalWrite(ONBOARD_LED, HIGH);
  delay(1000);
  digitalWrite(ONBOARD_LED, LOW);
  WiFi.begin(ssid, password);
  digitalWrite(ONBOARD_LED, HIGH);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ONBOARD_LED, HIGH);
    delay(1000);
    digitalWrite(ONBOARD_LED, LOW);
    Serial.print(".");
    delay(100);
  }

  //WiFi connection is OK 
  Serial.println("Connected to the WiFi network");
  //print IP address on Serial for debugging
  Serial.println(WiFi.localIP());
  digitalWrite(ONBOARD_LED, LOW);
  FastLED.show();
  wifiServer.begin();
  digitalWrite(ONBOARD_LED, HIGH);
}

unsigned char dimmer = 0;
unsigned int counter = 0;
unsigned char prev_buffer[16] = {0};
void loop() {

  static uint8_t hue = 0;

  uint8_t buffer[64] = "";
  unsigned char dbuffer[16] = "";

  WiFiClient client = wifiServer.available();
  digitalWrite(ONBOARD_LED, HIGH);
  
  //wait for a client to connect
  if (client) {
    Serial.print("WiFi client UP");

    while (client.connected()) {
      //counter is continuously incremented to keep track of time in animations
      counter++;
      digitalWrite(ONBOARD_LED, LOW);
      while (client.available() > 0) {
        digitalWrite(ONBOARD_LED, HIGH);

        //if we receive data from the client
        if (client.read(buffer, 64) > 0) {
          digitalWrite(ONBOARD_LED, LOW);

          //not sure if we actually need this 
          //but I can't test the circuit right now
          //dbuffer is overwritten a few lines down
          strncpy((char *)prev_buffer, (char *)dbuffer, 16);

          Serial.print("Server to client: ");
          Serial.println((char *)buffer);
          //decode what we received from client (the client sends hexadecimal strings)
          //the first byte encodes the type of actions
          //the following bytes encore the parameters of the action
          //Example : 
          //Client sends : 6358595a
          //This decodes to : cXYZ
          //c is the action type (color), XYZ are the parameters (RGB)
          //so dbuffer[0] == "c", dbuffer[1] == "X", dbuffer[2] == "Y", and dbuffer[3] == "Z"
          //we'll be using parameters as integers, so we have 
          //dbuffer[1] == 88, dbuffer[2] == 89, and dbuffer[3] == 90
          decode_hex((char *)buffer, dbuffer);
          Serial.println((char *)dbuffer);

          Serial.print((uint8_t)dbuffer[1]);
          Serial.print(" ");
          Serial.print((uint8_t)dbuffer[2]);
          Serial.print(" ");
          Serial.println((uint8_t)dbuffer[3]);

          

          //ping for discovery
          if (dbuffer[0] == 'p') {
            //respond with unique identifier
            client.write((uint8_t*)ID_STRING.c_str(), 21);
          }
          
          //color R G B
          if (dbuffer[0] == 'c') {
            //set the main color to RGB (we get 3 parameters after the "c")
            currentColor = CRGB(dbuffer[1], dbuffer[2], dbuffer[3]);
            FastLED.show();
            strncpy((char *)dbuffer, (char *)prev_buffer, 16);
          }

        }
      }

      //effects that need to loop are here
      //we react to dbuffer[0] to choose the animation
      //and we use dbuffer[1/2/3...] for parameters

      //rencontre
      if (dbuffer[0] == 'r') {
        //leds.fadeToBlackBy(1);
        leds[counter % NUM_LEDS] = currentColor;
        leds[(NUM_LEDS - 1) - counter % NUM_LEDS] = currentColor;
        if (counter % NUM_LEDS >= NUM_LEDS / 2) {
          leds.fadeToBlackBy(200);
          counter = 0;
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB(255, 255, 255);
          }
          FastLED.show();
          FastLED.delay(dbuffer[1]);
          leds.fadeToBlackBy(255);
          FastLED.show();
        }
        if ((counter + 1) % NUM_LEDS <= NUM_LEDS / 2) {
          FastLED.delay(256 * dbuffer[2] + dbuffer[1]);
        }
      }
      
      //double twister
      if (dbuffer[0] == 'C') {
        leds.fadeToBlackBy(dbuffer[2]);

        leds[NUM_LEDS - (counter%NUM_LEDS/2)] = currentColor;
        leds[(NUM_LEDS/2)-(counter%NUM_LEDS/2)] = currentColor;
        FastLED.delay(dbuffer[1]);
        FastLED.show();
      }

      //split from center (TODO)
      if (dbuffer[0] == 'D') {
        leds.fadeToBlackBy(dbuffer[2]);

        leds[NUM_LEDS - (counter%NUM_LEDS/2)] = currentColor;
        //leds(NUM_LEDS/2,NUM_LEDS) = leds(NUM_LEDS/2, 0);
        FastLED.delay(dbuffer[1]);
        FastLED.show();
      }

      //column rotate
      if (dbuffer[0] == 'E') {
        leds.fadeToBlackBy(dbuffer[2]);

        if(counter%20 < 10) {
          for (int i = 0; i < NUM_LEDS; i++) {
            if((i+counter)%7 == 0) {
              leds[i] = currentColor;
            }
            
          }

          } else {
            for (int i = NUM_LEDS; i != 0; i--) {
            if((i+counter)%7 == 0) {
              leds[i] = currentColor;
            }
            
          }
        }

        leds[NUM_LEDS - (counter%NUM_LEDS/2)] = currentColor;
        //leds(NUM_LEDS/2,NUM_LEDS) = leds(NUM_LEDS/2, 0);
        FastLED.delay(dbuffer[1]);
        FastLED.show();
      }

      //hors tension
      if (dbuffer[0] == 'B') {
        if (counter % dbuffer[1] == 0) {
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = currentColor;
          }
          FastLED.delay(100);
        } else {
          leds[random(0,NUM_LEDS)].fadeToBlackBy(255);
          FastLED.delay(dbuffer[2]);
        }
        FastLED.show();
      }

      //discoball
      if (dbuffer[0] == 'A') {
        leds.fadeToBlackBy(dbuffer[2]);
        leds[random(0, NUM_LEDS - 1)] = currentColor;
        FastLED.delay(dbuffer[1]);
      }

      if (dbuffer[0] == 'd') {
        dimmer = dbuffer[1];
      }

      //full
      if (dbuffer[0] == 'f') {
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = currentColor;
        }
        FastLED.show();
      }

      //blackout
      if (dbuffer[0] == 'b') {
        leds.fadeToBlackBy(255);
        FastLED.show();
      }

    }
  }

  client.stop();
}