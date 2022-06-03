/*
 * Clock generator for modular synthesiser
 * It provides clock pulses to drive sequencers and stuff
 * tested on Arduino pro mini (328P)
 * controls consist of 5 pots:
 * MCLK sets the tempo of the master clock
 * CLKDIV sets a divider for each of 2 clock outputs. clk = mclk / N where N is between 1 and 8
 * RSTDIV sets a divider for each of 2 reset outputs. Each reset will give a pulse every N clock cycles where N is between 1 and 8
 */

// Pins
const int NUM_CLKS = 2;
const int MAX_CLK_DIV = 8;
const int MAX_RST_DIV = 8;
#define ADC_MCLK_PIN A0
uint8_t adc_clk_div_pin[NUM_CLKS] = {1, 2}; //A1, A2
uint8_t adc_rst_div_pin[NUM_CLKS] = {4, 3}; //A4, A3

#define MCLK_LED_PIN 2
int clk_led_pin[NUM_CLKS] = {3, 5};
int rst_led_pin[NUM_CLKS] = {4, 6};
int clk_out_pin[NUM_CLKS] = {7, 11};
int rst_out_pin[NUM_CLKS] = {8, 12};

uint16_t mclkTempo;
unsigned long currentMillis;
unsigned long previousMillis = 0;
bool mclk = false;

uint8_t clkCount[MAX_CLK_DIV] = {0}; // array of counters for clock dividers
bool clk[MAX_CLK_DIV] = {false}; // divided clocks

uint8_t rstCount[MAX_CLK_DIV][MAX_RST_DIV] = {0}; // array of counters for reset dividers
bool rst[MAX_CLK_DIV][MAX_RST_DIV]; // array of all possible resets

void setup() {
  pinMode(MCLK_LED_PIN, OUTPUT);
  for (int i=0; i<NUM_CLKS; i++) {
    pinMode(clk_led_pin[i], OUTPUT);
    pinMode(clk_out_pin[i], OUTPUT);
    pinMode(rst_led_pin[i], OUTPUT);
    pinMode(rst_out_pin[i], OUTPUT);
    setRst(i, true);
  }
  for (int i=0; i<MAX_CLK_DIV; i++) {
    for (int j=0; j<MAX_RST_DIV; j++) {
      rst[i][j] = true; // start with reset asserted
    }
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

void incRstCounters(int clkDiv) {
  // increment the reset dividors associated with one divided clock
  for (int i=0; i<MAX_RST_DIV; i++) {
    if (rstCount[clkDiv][i] == i) {
      rstCount[clkDiv][i] = 0;
      rst[clkDiv][i] = true;
    } else {
      rstCount[clkDiv][i] += 1;
      rst[clkDiv][i] = false;
    }
  }
}

void incCounters() {
  // increment the clock divider counters
  for (int i=0; i<MAX_CLK_DIV; i++) {
    if (clkCount[i] == 0) {
      clk[i] = not clk[i]; // toggle
      if (clk[i] == false) {
        incRstCounters(i); 
      }
    }
    if (clkCount[i] == i) {
      clkCount[i] = 0;
    } else {
      clkCount[i] += 1;
    }
  }
}

void setOutputs() {
  // select the clock outputs
  for (int i=0; i< NUM_CLKS; i++) {
    uint8_t divSel = getClkDiv(i);
    bool clkOut = clk[divSel];
    setClk(i, clkOut);
    uint8_t rstSel = getRstDiv(i);
    bool rstOut = rst[divSel][rstSel];
    setRst(i, rstOut);
    // light reset led only when clk is high, for asthetic reasons
    bool ledState = clkOut ? rstOut : false;
    setRstLed(i, ledState);
  }
}

void loop() {
  mclkTempo = getMclkTempo();
  currentMillis = millis();
  if (currentMillis - previousMillis > mclkTempo) {
    previousMillis = currentMillis;
    mclk = not mclk; // toggle
    setMclk(mclk);
    incCounters();
    setOutputs();
  }
}
