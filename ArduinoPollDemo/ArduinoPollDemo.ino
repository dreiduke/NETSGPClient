#define prog_pin 4
#define RXD2 16
#define TXD2 17

uint8_t rx_buffer[50] = {0};

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(prog_pin, OUTPUT);
  digitalWrite(prog_pin, HIGH);
  delay(1000);
  Serial.println("Welcome to Micro Inverter Interface By ATCnetz.de");
}

long last_send = 0;
void loop() {

  if (millis() - last_send > 2000) {
    last_send = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("");
    Serial.println("Sending request now");

    if (wait_for_answer(send_cmd(0x11002793, 0xC0, 0x00))) {
      if (rx_buffer[1] == 0xc0) { // Got Status back

        uint32_t device_id = rx_buffer[6] << 24 | rx_buffer[7] << 16 | rx_buffer[8] << 8 | rx_buffer[9] & 0xff;

        int state = 0;// not reversed
        int power_grade = 0;// not reversed

        float dc_voltage = (rx_buffer[15] << 8 | rx_buffer[16]) / 100;
        float dc_current = (rx_buffer[17] << 8 | rx_buffer[18]) / 100;
        float dc_power = dc_voltage * dc_current;

        float ac_voltage = (rx_buffer[19] << 8 | rx_buffer[20]) / 100;
        float ac_current = (rx_buffer[21] << 8 | rx_buffer[22]) / 100;
        float ac_power = ac_voltage * ac_current;

        int power_gen_total = 0;// not reversed
        int temperature = rx_buffer[26];// not reversed

        Serial.println("*********************************************");
        Serial.println("Received Inverter Status");
        Serial.print("Device: ");
        Serial.println(device_id, HEX);
        Serial.println("Status: "  + String(state));
        Serial.println("PowerGrade: "  + String(power_grade));
        Serial.println("DC_Voltage: "  + String(dc_voltage) + "V");
        Serial.println("DC_Current: "  + String(dc_current) + "A");
        Serial.println("DC_Power: "  + String(dc_power) + "W");
        Serial.println("AC_Voltage: "  + String(ac_voltage) + "V");
        Serial.println("AC_Current: "  + String(ac_current) + "A");
        Serial.println("AC_Power: "  + String(ac_power) + "W");
        Serial.println("Power gen total: "  + String(power_gen_total));
        Serial.println("Temperature: "  + String(temperature));
      }
    }
  }
}

uint8_t send_cmd(uint32_t device_id, uint8_t cmd, uint8_t mode) {
  uint8_t crc = 0x43;
  Serial2.write(0x43);
  Serial2.write(cmd);
  crc += cmd;

  Serial2.write(0x00);
  Serial2.write(0x00);
  Serial2.write(0x00);
  Serial2.write(0x00);

  Serial2.write((device_id >> 24) & 0xff);
  Serial2.write((device_id >> 16) & 0xff);
  Serial2.write((device_id >> 8) & 0xff);
  Serial2.write(device_id & 0xff);
  crc += (device_id >> 24) & 0xff;
  crc += (device_id >> 16) & 0xff;
  crc += (device_id >> 8) & 0xff;
  crc += device_id & 0xff;

  Serial2.write(0x00);
  Serial2.write(0x00);
  Serial2.write(0x00);

  Serial2.write(mode);
  crc += mode;
  Serial2.write(crc);
  return cmd;
}

bool wait_for_answer(uint8_t cmd) {
  uint8_t expected_len = (cmd == 0xC0) ? 26 : 14;
  long start_time = millis();
  int rx_pos = 0;
  while (millis() - start_time < 100 && rx_pos <= expected_len) {
    if (Serial2.available()) {
      rx_buffer[rx_pos++] = Serial2.read();
     /* Serial.print("Received one pos:");
      Serial.print(rx_pos - 1);
      Serial.print(" buff 0x");
      Serial.println(rx_buffer[rx_pos - 1], HEX);*/
      if (rx_pos > expected_len) {
        while (Serial2.available())Serial2.read();
        return true;
      }
    }
  }
  while (Serial2.available())Serial2.read();
  return false;
}
