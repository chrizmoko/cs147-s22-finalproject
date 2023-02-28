#include <Arduino.h>
#include <Esp.h>

#include <WiFi.h>
#include <WiFiManager.h>
#include <HttpClient.h>
#include <unordered_map>
#include <TFT_eSPI.h>

#include "network.hpp"

#define RECEIVE_BUTTON_PIN GPIO_NUM_33
#define SEND_BUTTON_PIN    GPIO_NUM_25
#define WRITE_BUTTON_PIN   GPIO_NUM_27
#define UNDO_BUTTON_PIN    GPIO_NUM_26

#define LED_PIN            GPIO_NUM_13
#define BUZZER_PIN         GPIO_NUM_12

TFT_eSPI oled = TFT_eSPI();
uint8_t textSize = 2;

//
const char address[] = "[your ip address here]";
short port = 5000;
ApplicationNetworkClient network = ApplicationNetworkClient(address, port);

//deocoder variables
const int NUM_CHAR = 26;
static String morse[NUM_CHAR] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
"..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-",
".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."};
static String letter[NUM_CHAR] = {"A", "B", "C", "D", "E", "F", "G", "H",
"I", "J", "K", "L", "M", "N", "O", "P", "Q",
"R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
struct Map {
    String key;
    String value;
};
struct Map decoder[NUM_CHAR]; 

static String encoded_message;
static String decoded_message;
//There's 3 modes, encode, decode, and read
//encoding mode allows the user to write(write button), edit(undo button), and decode(send button)
//the encoded message
//decoded mode allows the user to edit(undo button) and send(send button) the decoded message
//read mode allows the user to view received messages(read button)
const String ENCODING = "encoding";
const String DECODED = "decoded";
const String READ = "read";
static String mode = DECODED;
static String prevMode = DECODED;


bool led_toggle = false;
unsigned long last_pressed = millis();
unsigned long last_released = millis();
unsigned long last_LCD_update = millis();
unsigned long last_message_check = millis();


/**
 * Executes the connect to Wifi access point setup process for the
 * microcontroller. This process is blocking and will restart the device upon
 * failure to connect to an access point.
 */
void setupWifi();
void updateLCD();
void decodeMessage();
void buzzBuzzer();
void writeAlert(String message);
void checkMessages(bool override=false);


void setup()
{
  Serial.begin(9600);

  //setup oled
  oled.init();
  oled.setTextSize(textSize);
  oled.fillScreen(TFT_BLACK);
  oled.setRotation(3);

  oled.fillScreen(TFT_BLACK);
  oled.drawString("Setting Up Wifi...", 10, 10);
  setupWifi();  
  oled.fillScreen(TFT_BLACK);

  //register device with cloud
  network.makeVisible();

  // setup GPIO pins
  pinMode(RECEIVE_BUTTON_PIN, PULLUP);
  pinMode(SEND_BUTTON_PIN, PULLUP);
  pinMode(WRITE_BUTTON_PIN, PULLUP);
  pinMode(UNDO_BUTTON_PIN, PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  //initialize decoding map
  for (int i=0; i<NUM_CHAR; i++){
    struct Map map;
    map.key = morse[i];
    map.value = letter[i];
    decoder[i] = map;
  }
  
}

void loop()
{
  // Serial.println("------------------------------");
  if (!mode.equals(READ)){
    updateLCD();
    checkMessages();
  }
  led_toggle ? digitalWrite(LED_PIN, HIGH) : digitalWrite(LED_PIN, LOW);

  unsigned long current_time = millis();

  if (!digitalRead(WRITE_BUTTON_PIN))
  {
    Serial.println("Write button pressed");
    if (!mode.equals(READ)){
      mode = ENCODING;
      last_pressed = millis();
      while(!digitalRead(WRITE_BUTTON_PIN)){
        buzzBuzzer();
      }
      last_released = millis();
      if (last_released-last_pressed < 250){
        encoded_message.concat(".");
      }
      else if (last_released-last_pressed > 500){
        encoded_message.concat("-");
      }
    }
    
  }

  if (!digitalRead(UNDO_BUTTON_PIN))
  {
    Serial.println("Undo button pressed");
    while(!digitalRead(UNDO_BUTTON_PIN)){
      updateLCD();
      delay(10);
    }
    //remove last character in either decoded_message or encoded_message
    if (mode.equals(DECODED)){
      if (decoded_message.length() != 0){
        decoded_message.remove(decoded_message.length()-1);
      }
    }
    else if (mode.equals(ENCODING)){
      if (encoded_message.length() != 0){
        encoded_message.remove(encoded_message.length()-1);
      }
    }
    else if (mode.equals(READ)){
      oled.fillScreen(TFT_BLACK);
      mode = prevMode;
      updateLCD;
    }
  }

  if (!digitalRead(SEND_BUTTON_PIN))
  {
    Serial.println("Send button pressed");
    while(!digitalRead(SEND_BUTTON_PIN)){
      updateLCD();
      delay(10);
    }
    //if you're currently editing the decoded message and press send, send message to the cloud
    //if you're currently encoding a message and press send, proceed to decode the message
    if (mode.equals(DECODED)){
      network.sendMessage(decoded_message.c_str());
      decoded_message = "";
      writeAlert("SENT");
      updateLCD();
    }
    else if (mode.equals(ENCODING)){
      decodeMessage();
    }
  }

  if (!digitalRead(RECEIVE_BUTTON_PIN))
  {
    Serial.println("Receive button pressed");
    if (led_toggle == true){
      network.fetchPendingMessages(1);
      struct ApplicationNetworkClient::message_map message = network.getFetchedMessages().at(0);
      Serial.println(message.content);
      oled.fillScreen(TFT_BLACK);
      oled.drawString(message.content, 10, 10);
      oled.setTextSize(1);
      oled.drawString("From:", 10, 50);
      oled.drawString(message.macAddress, 10, 60);
      oled.drawString("When: ", 10, 80);
      oled.drawString(message.time, 10, 90);
      oled.setTextSize(textSize);

      if (!mode.equals(READ)){
        prevMode = mode;
      }
      mode = READ;
      checkMessages(true);
    }
    else{
      writeAlert("NO MESSAGES");
      if (mode.equals(READ)){
        mode = prevMode;
      }
      updateLCD();
    }
  }

  // Serial.println("------------------------------");

  // led_toggle ? digitalWrite(BUZZER_PIN, HIGH) : digitalWrite(BUZZER_PIN, LOW);
  // led_toggle = !led_toggle;

  delay(10);
}

void buzzBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);
}

void decodeMessage(){
  //empty encoded_message decodes into a whitespace
  if (encoded_message.equals("")){
    decoded_message.concat(" ");
    mode = DECODED;
  }
  else{
    bool decodable = false;
    //Iterate through decoder mapping to decode message from morse code to letter.
    for (int i = 0; i < NUM_CHAR; i++){
      if (encoded_message.equals(decoder[i].key) ){
        decoded_message.concat(decoder[i].value);
        decodable = true;
        encoded_message = "";
        oled.fillScreen(TFT_BLACK);
        updateLCD();
        mode = DECODED;
      }
    }
    //Write INVALID if encoded_message is unable to be decoded
    if (!decodable){
      writeAlert("INVALID");
      updateLCD();
    }
  }
}

void checkMessages(bool override){
  unsigned long current_time = millis();
  if ((current_time - last_message_check > 5000 && led_toggle == false) || override){
    int numMessages = network.countPendingMessages();
    if (numMessages > 0){
      led_toggle = true;
    }
    else{
      led_toggle = false;
    }
    last_message_check = current_time;
  }
  

}

void setupWifi()
{
  WiFiManager wifi_manager;

  // For debugging purposes only
  wifi_manager.resetSettings();

  const char* access_point_name = "Christian hello hello";
  const char* access_point_password = "helloworld";

  bool result = wifi_manager.autoConnect(
    access_point_name,
    access_point_password
  );

  if (!result)
  {
    // Serial.println("Could not connect to wifi network, attempting to reboot.");
    ESP.restart();
  }

  Serial.println("Successfully connected to wifi network.");
}

void updateLCD(){
  unsigned long current_time = millis();
  //Updates messages and blinks | indicator every half second
  if (current_time - last_LCD_update < 500){
      oled.drawString(encoded_message+"| ", 10, 10);
      oled.drawString(decoded_message+"| ", 10, 80);
  }
  else if (current_time - last_LCD_update >= 500){
    if (mode.equals(DECODED)){
      oled.drawString(encoded_message+"| ", 10, 10);
      oled.drawString(decoded_message+" ", 10, 80);
    }
    else if (mode.equals(ENCODING)){
      oled.drawString(encoded_message+" ", 10, 10);
      oled.drawString(decoded_message+"| ", 10, 80);
    }

  }
  if (current_time - last_LCD_update > 1000){
    last_LCD_update = current_time;
  }

}

void writeAlert(String message){
  oled.fillScreen(TFT_BLACK);
  oled.drawString(message, 10, 10);
  delay(1000);
  oled.fillScreen(TFT_BLACK);
}


