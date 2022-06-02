/*
 * Clock generator for modular synthesiser
 * It provides clock pulses to drive sequencers and stuff
 * tested on Arduino pro mini (328P)
 */

// Pins
const int num_clks = 2;
#define ADC_MCLK_PIN A0
uint8_t adc_clk_div_pin[num_clks] = {1, 2}; //A1, A2
uint8_t adc_rst_div_pin[num_clks] = {4, 3}; //A4, A3

#define MCLK_LED_PIN 2
int clk_led_pin[num_clks] = {3, 5};
int rst_led_pin[num_clks] = {4, 6};
int clk_out_pin[num_clks] = {7, 11};
int rst_out_pin[num_clks] = {8, 12};

uint16_t mclkTempo;
uint8_t clkDiv[num_clks];
uint8_t rstDiv[num_clks];
unsigned long currentMillis;
unsigned long previousMillis = 0;
bool mclk = false;

void setup() {
  pinMode(MCLK_LED_PIN, OUTPUT);
  for (int i=0; i<2; i++) {
    pinMode(clk_led_pin[i], OUTPUT);
    pinMode(clk_out_pin[i], OUTPUT);
    pinMode(rst_led_pin[i], OUTPUT);
    pinMode(rst_out_pin[i], OUTPUT);
  }
}

uint16_t getMclkTempo() {
  // read master clock control and return an integer that is half the master clock period, in ms
  return (1023 - analogRead(ADC_MCLK_PIN)) + 50;
}

void setMclk(bool val) {
  // set the mclk indicator LED
  if (val) {
    digitalWrite(MCLK_LED_PIN, HIGH);
  } else {
    digitalWrite(MCLK_LED_PIN, LOW);
  }
}

uint8_t getClkDiv(uint8_t chan) {
  // read the clock divider control and return the divider value
  return analogRead(adc_clk_div_pin[chan]) >> 7;
}

void setClk(uint8_t chan, bool val) {
  // set the clk output and LED
  if (val) {
    digitalWrite(clk_led_pin[chan],HIGH);
    digitalWrite(clk_out_pin[chan], HIGH);
  } else {
    digitalWrite(clk_led_pin[chan],LOW);
    digitalWrite(clk_out_pin[chan], LOW);
  }
}

uint8_t getRstDiv(uint8_t chan) {
  // read the reset divider control and return the divider value
  return analogRead(adc_rst_div_pin[chan]) >> 7;
}

void setRst(uint8_t chan, bool val) {
  // set the reset output (LED done in its own function)
  if (val) {
    digitalWrite(rst_out_pin[chan], HIGH);
  } else {
    digitalWrite(rst_out_pin[chan], LOW);
  }
}

void setRstLed(uint8_t chan, bool val) {
  // set the reset LED
  if (val) {
    digitalWrite(rst_led_pin[chan], HIGH);
  } else {
    digitalWrite(rst_led_pin[chan], LOW);
  }
}

void proc_clk(uint8_t chan) {
  // process one clk / reset channel. Called on both rising and falling edges of master clock
  static uint8_t clkDivCnt[num_clks] = {0};
  static uint8_t rstDivCnt[num_clks] = {0};
  static bool clk[num_clks] = {false};
  if (clkDivCnt[chan] >= clkDiv[chan]) {
    if ((clkDiv[chan] != 0 ) || (clk[chan] != mclk)) {
      clk[chan] = not clk[chan]; // toggle clock
      setClk(chan, clk[chan]);
      clkDivCnt[chan] = 0;
      if (clk[chan] == false) {
        // falling edge
        if (rstDivCnt[chan] >= rstDiv[chan]) {
          setRst(chan, true);
          rstDivCnt[chan] = 0;
        } else {
          setRst(chan, false);
          setRstLed(chan, false);
          rstDivCnt[chan] += 1;
        }
      } else {
        // rising edge
        if (rstDivCnt[chan] == 0) {
          // turn on reset led
          setRstLed(chan, true);
        }
      }
    }
  } else {
    clkDivCnt[chan] += 1;
  }  
}

void loop() {
  mclkTempo = getMclkTempo();
  for (int i=0; i< num_clks; i++) {
    clkDiv[i] = getClkDiv(i);
    rstDiv[i] = getRstDiv(i);
  }
  currentMillis = millis();
  if (currentMillis - previousMillis > mclkTempo) {
    previousMillis = currentMillis;
    mclk = not mclk;
    setMclk(mclk);
    for (int i=0; i< num_clks; i++) {
      proc_clk(i);
    }
  }
}
