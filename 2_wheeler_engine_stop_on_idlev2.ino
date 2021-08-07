#include <TimerOne.h>

int timer = 20000; //20s timer

int led_on = 11; //green led showing device on state - on pin 11
int led_off = 12; //red led showing device off state - on pin 12
int led_transition = 10; //yellow led showing transition state - on pin 10

int sound_level = 15; //difference between initial sound and final sound in db - to be considered significant sound change

int on_signal = 9; //relay/engine pin on pin 9

int on_switch = 3; //device on switch on pin 3

const int IRSensorPin = 2;

int inputState;
int lastInputState = LOW;
long lastDebounceTime = 0;
long debounceDelay = 5;

long time;
long endTime;
long startTime;
int RPM = 0;
double trip = 0;
double kkbanspd = 0.00223;
float lnTime = 0;

void setup()
{
  pinMode(on_switch, INPUT);
  pinMode(led_on, OUTPUT);
  pinMode(led_off, OUTPUT);
  pinMode(led_transition, OUTPUT);
  pinMode(on_signal, OUTPUT);

  pinMode(IRSensorPin, INPUT);
  endTime = 0;
  Timer1.initialize(1000000);  // Set the timer to 60 rpm, 1,000,000 microseconds (1 second)

}


void loop()
{
  while (digitalRead(on_switch) == 1) //if device switch is on
  {
    digitalWrite(led_on, HIGH); //turn green led on indicating machine is on
    digitalWrite(led_off, LOW);
    digitalWrite(led_transition, LOW);

    //check speed
    Speed();
    double speed = RPM * kkbanspd;

    float initial_sound_sum = 0;
    for (int i = 0; i < 15; i++)
    {
      //measuring sound every second and summing the values
      initial_sound_sum += sound();

      //wait for 1s
      delay(1000);

    }

    //taking average of first 15s
    float initial_sound = initial_sound_sum / 15;

    if (speed < 1) //if speed is less than 1km/hr
    {
      digitalWrite(led_on, LOW);
      digitalWrite(led_transition, HIGH); //turn on transition led

      float final_sound_sum = 0;
      for (int i = 0; i < 5; i++)
      {
        //measuring sound every second and summing the values
        final_sound_sum += sound();

        //wait for 1s
        delay(1000);

      }

      //taking average of last 5s
      float final_sound = final_sound_sum / 15;

      //check speed again after 20s
      Speed();
      speed = (RPM * kkbanspd);



      if ((final_sound - initial_sound) > sound_level) //keep engine on if the sound level change is significant from initial condition after 20s
      {
        digitalWrite(led_transition, LOW);
        digitalWrite(led_on, HIGH); //turn green led on indicating machine is on
        digitalWrite(on_signal, HIGH); //keep engine on
      }

      else if (speed < 1)
      {
        digitalWrite(led_transition, LOW);
        digitalWrite(led_off, HIGH); //turn red led on to indicate engine is off
        digitalWrite(on_signal, LOW); //keep engine off

        delay(10);
        digitalWrite(on_signal, HIGH); //reset the relay, to allow driver to turn on by ignition
      }

    }

    else {
      digitalWrite(on_signal, HIGH); //reset the relay, to allow driver to turn on by ignition
    }
  }

}

void Speed()
{
  time = millis();
  int currentSwitchState = digitalRead(IRSensorPin);

  if (currentSwitchState != lastInputState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentSwitchState != inputState) {
      inputState = currentSwitchState;
      if (inputState == LOW) {
        calculateRPM(); // Real RPM from sensor
      }
    }
  }
  lastInputState = currentSwitchState;
}

void calculateRPM() {
  startTime = lastDebounceTime;
  lnTime = startTime - endTime;
  RPM = 60000 / (startTime - endTime);
  endTime = startTime;
  trip++;
}

float sound() {
  const int sampleWindow = 50;                              // Sample window width in mS (50 mS = 20Hz)
  unsigned int sample;
  unsigned long startMillis = millis();                  // Start of sample window
  float peakToPeak = 0;                                  // peak-to-peak level

  unsigned int signalMax = 0;                            //minimum value
  unsigned int signalMin = 1024;                         //maximum value

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow)
  {
    //microphone connected at A0
    sample = analogRead(0);                             //get reading from microphone
    if (sample < 1024)                                  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;                           // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;                           // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;                    // max - min = peak-peak amplitude
  float db = map(peakToPeak, 20, 900, 49.5, 90);         //calibrate for deciBels

  return db;
}
