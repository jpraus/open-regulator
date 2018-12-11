#include "uplink.h"

#define UPLINK_INTERVAL 60000 // uplink every 60s
#define RETRY_INTERVAL 10000 // retry 10 after failure

// Ethernet module connection settings
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 8, 107);

// Uplink server endpoint
char server[] = "tranquil-scrubland-26891.herokuapp.com";
char endpoint[] = "/rest/v1/heating";

void UPLINK::setup() {
  /*Serial.println("Trying to get an IP address using DHCP");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);
  }*/

  Ethernet.begin(mac, ip);

  Serial.print("[UP] Using IP address ");
  Serial.println(Ethernet.localIP());

  _timer = UPLINK_INTERVAL - RETRY_INTERVAL; // shorten first upload interval
}

void UPLINK::update(int deltaMs) {
  _timer += deltaMs;

  // read response if awaiting one
  if (_awaitingResponse) {
    while (_client.available()) {
      _client.read(); // TODO: parse and verify success response
    }
    if (!_client.connected()) { // close connection, all data received
      _awaitingResponse = false;
      _client.stop();
    }
  }
  // send data
  else if (!_client.connected() && _timer >= UPLINK_INTERVAL) {
    if (_send()) {
      Serial.println("[UP] data uploaded");
      _timer = 0; // reset timer if successfull
    }
    else {
      _timer = _timer - RETRY_INTERVAL;
    }
  }
}

bool UPLINK::_send() {
  // if you get a connection, report back via serial:
  if (_client.connect(server, 80)) {
    Serial.print("[UP] sending data ");

    String data = _createStateData();
    Serial.print(data.length());
    Serial.println("B");
    Serial.println(data);

    _client.println("POST /rest/v1/heating HTTP/1.1");
    _client.println("Host: tranquil-scrubland-26891.herokuapp.com");
    _client.println("Connection: close");
    _client.println("Content-Type: application/json");
    _client.print("Content-Length: "); _client.println(data.length());
    _client.println();
    _client.println(data);

    _awaitingResponse = true;
    return true;
  } 
  else {
    // if you didn't get a connection to the server:
    Serial.println("[UP] connection failed");
    return false;
  }
}

String UPLINK::_createStateData() {
  String buffer = "{";

  buffer += "\"v\":2,"; // protocol version
  buffer += "\"d\":["; // protocol data (key is position in array)

  // MEMORY: add more data when more memory
  // boiler data
  //_printJsonBool(buffer, _state.boiler.fault); buffer += ",";
  _printJsonBool(buffer, _state.boiler.chOn); buffer += ",";
  _printJsonBool(buffer, _state.boiler.dhwOn); buffer += ",";
  //_printJsonBool(buffer, _state.boiler.flameOn); buffer += ",";
  //_printJsonFloat(buffer, _state.boiler.modulationLvl); buffer += ",";
  _printJsonFloat(buffer, _state.boiler.feedTemp); buffer += ",";
  //_printJsonFloat(buffer, _state.boiler.returnTemp); buffer += ",";

  // thermostat data
  _printJsonBool(buffer, _state.thermostat.chOn); buffer += ",";
  _printJsonBool(buffer, _state.thermostat.dhwOn); buffer += ",";
  _printJsonFloat(buffer, _state.thermostat.outsideTemp); buffer += ",";
  _printJsonFloat(buffer, _state.thermostat.roomTemp); buffer += ",";
  _printJsonFloat(buffer, _state.thermostat.dhwTemp); buffer += ",";
  _printJsonFloat(buffer, _state.thermostat.chSetpoint); buffer += ",";
  _printJsonFloat(buffer, _state.thermostat.dhwSetpoint); buffer += ",";

  // accumulator data
  _printJsonBool(buffer, _state.accumulator.chOn); buffer += ",";
  _printJsonBool(buffer, _state.accumulator.dhwOn); buffer += ",";
  _printJsonFloat(buffer, _state.accumulator.topTemp); buffer += ",";
  _printJsonFloat(buffer, _state.accumulator.bottomTemp); buffer += ",";
  _printJsonFloat(buffer, _state.accumulator.chTemp); buffer += ",";
  //_printJsonFloat(buffer, _state.accumulator.returnTemp); buffer += ",";
  _printJsonFloat(buffer, _state.accumulator.valveAngle); //buffer += ",";
  //_printJsonFloat(buffer, _state.accumulator.pressure); buffer += ",";
  //buffer += _state.accumulator.mode;

  buffer += "]}";

  return buffer;
}

void UPLINK::_printJsonFloat(String &buffer, float value) {
  buffer += String(value, 1); // MEMORY: better precision when more memory
}

void UPLINK::_printJsonBool(String &buffer, bool value) {
  buffer += value ? "1" : "0";
}

