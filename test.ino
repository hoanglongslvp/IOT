#include <FirebaseArduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define PIN_LED 16
#define PIN_BUTTON 10

#define LED_ON() digitalWrite(PIN_LED, HIGH)
#define LED_OFF() digitalWrite(PIN_LED, LOW)
#define LED_TOGGLE() digitalWrite(PIN_LED, digitalRead(PIN_LED) ^ 0x01)

boolean LED_state[2] = {0};
bool in_smartconfig = false;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "2.asia.pool.ntp.org", 7*3600);
Ticker ticker;
int dem=0;

//class

class Control
{
  //các biến
  String Port;
  int ledPin;
  int ledState;
  unsigned long previousMillis;
  int starts[30] = {0};
  int ends[30]= {0};
  int powers[30]= {0};
  int len;
  //các hàm
  //hàm khởi tạo
  public:
  Control(int mPin, String mPort)
  {
    ledPin = mPin;
    pinMode(ledPin, OUTPUT);
    Port = mPort;
    ledState = LOW;
    previousMillis = 0;
    len = 0;
  }
  //hàm lấy dữ liệu
  void getData()
  {
        //Serial.println("Start getdata "+Port);
        FirebaseObject obj = Firebase.get("/tculink1/"+Port+"/alarm");
        len = getValue(starts,obj.getString("/starts"));
        getValue(ends,obj.getString("/ends"));
        getValue(powers,obj.getString("/powers"));
        //Serial.println("Getdata "+Port+" ok");
  }
  //hàm điều khiển
  void setLed()
  {
    int times = timeClient.getFormattedTimeFull().toInt();
    int temp=0;
    for(int i=0;i<len;i++)
    {
        if(((ends[i] > starts[i]) && (powers[i] == 1) && (times >= (starts[i]*1000)) && (times <= (ends[i]*1000)))
        ||((ends[i] <= starts[i]) && (powers[i] == 1) && ((times >= starts[i]) ||  (times <= ends[i]))))
        {
          temp++;
        }
    }
    Serial.println();
    if(temp ==0 )
    {
      ledState = LOW;  // Turn it off
      digitalWrite(ledPin, !ledState);  // Update the actual LED
      Firebase.setInt("/tculink1/"+Port+"/state",0);
      Serial.println("Setdata "+Port+" =0 ok");
      previousMillis = millis();  // Remember the time
    }
    else
    {
      ledState = HIGH;  // Turn it on
      digitalWrite(ledPin, !ledState);  // Update the actual LED
      Firebase.setInt("/tculink1/"+Port+"/state",1);
      Serial.println("Setdata "+Port+" =1 ok");
      previousMillis = millis();  // Remember the time
    }
  }
  //tách dữ liệu
  int getValue(int temp[],String data)
  {
    int r=0, t=0;
    for (int i=0; i <= data.length(); i++)
    { 
      if(data.charAt(i) == ',') 
      { 
        temp[t] = data.substring(r, i).toInt();
        r=(i+1);
        t++; 
      }
    }
    return t;
  }
};
Control controla(5,"SW1");
Control controlb(4,"SW2");
Control controlc(0,"SW3");
Control controld(2,"SW4");
//end class
void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  
  Serial.begin(9600);
  
  delay(5000);
  Firebase.begin("tculink-49f5d.firebaseio.com");
  Firebase.stream("/tculink1");
  timeClient.begin();
  
}


void loop() {
  if (longPress()) {
    enter_smartconfig();
    //Serial.println("Enter smartconfig");
  }
  if (WiFi.status() == WL_CONNECTED && in_smartconfig && WiFi.smartConfigDone()) {
    exit_smart();
    Firebase.begin("tculink-49f5d.firebaseio.com");
    Firebase.stream("/tculink1");
    //Serial.println("Connected, Exit smartconfig");
    timeClient.begin();
  }
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    //Serial.println(timeClient.getFormattedTime());
    if (Firebase.failed()) {
      Serial.println("streaming error");
      //Serial.println(Firebase.error());
      ESP.reset();
    }
    if (Firebase.available()) {
       digitalWrite(PIN_LED, HIGH);    
       FirebaseObject event = Firebase.readEvent();
       String eventType = event.getString("type");
       eventType.toLowerCase(); 
       Serial.print("event: ");
       Serial.println(eventType);
       if (eventType == "put" ||eventType == "patch" ) {
        String path = event.getString("path");
        if(dem==0)
        {
          controla.getData();
          controlb.getData();
          controlc.getData();
          controld.getData();
          dem ++;
        }
        else
        {
          if(path == "/SW1/alarm") controla.getData();
          else if(path == "/SW2/alarm") controlb.getData();
          else if(path == "/SW3/alarm") controlc.getData(); 
          else if(path == "/SW4/alarm") controld.getData(); 
        }
       }
    }
    controla.setLed();
    controlb.setLed();
    controlc.setLed();
    controld.setLed();
    //ButtonDebounce();
  }  
}
//function
bool longPress()
{
  static int lastPress = 0;
  if (millis() - lastPress > 3000 && digitalRead(PIN_BUTTON) == 0) {
    return true;
  } else if (digitalRead(PIN_BUTTON) == 1) {
    lastPress = millis();
  }
  return false;
}

void tick()
{
  //toggle state
  int state = digitalRead(PIN_LED);  
  digitalWrite(PIN_LED, !state);     
}

void enter_smartconfig()
{
  if (in_smartconfig == false) {
    in_smartconfig = true;
    ticker.attach(0.1, tick);
    WiFi.beginSmartConfig();
  }
}

void exit_smart()
{
  ticker.detach();
  LED_ON();
  in_smartconfig = false;
}
void ButtonDebounce(void)
{
    static byte buttonState[2]     = {LOW, LOW};   
    static byte lastButtonState[2] = {LOW, LOW};  
    static long lastDebounceTime[2] = {0}; 
    long debounceDelay = 50;        
    byte reading[2];
    reading[0] = digitalRead(14);
    reading[1] = digitalRead(13);
    for (int i = 0; i < 2; i++) {
        if (reading[i] != lastButtonState[i]) {
            lastDebounceTime[i] = millis();
        }
      
        if ((millis() - lastDebounceTime[i]) > debounceDelay) {
            if (reading[i] != buttonState[i]) {
                buttonState[i] = reading[i];
                if (buttonState[i] == HIGH) {
                    LED_state[i] = !LED_state[i];
                }
            }
        }
    }
    digitalWrite(5, LED_state[0]);
    digitalWrite(0, LED_state[1]);
    lastButtonState[0] = reading[0];
    lastButtonState[1] = reading[1];
}
