#include <bluefruit.h>

//Set up gyro stuff
#include <Adafruit_LSM6DS33.h>
Adafruit_LSM6DS33 lsm6ds33; // accelerometer, gyroscope
float gyro_x, gyro_y, gyro_z;



//set up neopixel stuff
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define PIN            13
#define NUMPIXELS      11
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int delayval = 50;
//end of neopixel setup


BLEUart bleuart; //peripheral uart service
BLEDfu bledfu; //Over the air device firmware update
BLEClientUart clientUart; // Cenrtal uart client

void setup() {
  pixels.begin();//start neopixels
  lsm6ds33.begin_I2C();// start gyroscope
  Serial.begin(115200);
  Bluefruit.begin(1,1); //start bluefruit with 1 available peripheral and central connections
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Bracelet");
  
  Bluefruit.Central.setConnectCallback(cent_connect_callback); 
  Bluefruit.Periph.setConnectCallback(prph_connect_callback);
//no other callbacks needed as we only care about rssi values once connected???
  bledfu.begin(); 
  bleuart.begin();
  clientUart.begin(); //start BLE central uart Service

  /* Start Central Scanning for other bracelet
   * - Enable auto scan if disconnected
   * - Interval = 100 ms, window = 80 ms
   * - Filter only accept bleuart service (IE other bracelet
   * - Don't use active scan
   * - Start(timeout) with timeout = 0 will scan forever (until connected)
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  Bluefruit.Scanner.filterUuid(bleuart.uuid);
  Bluefruit.Scanner.useActiveScan(true);
  Bluefruit.Scanner.start(0);                   // 0 = Don't stop scanning after n seconds

  startAdv(); //start advertising as peripheral to other bracelet

}

void startAdv(void) // describes how to advertise prior to connecting
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}
void bracelet_alone(){

  /*for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(250,10,50)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(delayval); // Delay for a period of time (in milliseconds).
  }*/
 // pixels.setPixelColor(0,pixels.Color(0,0,0));
  //pixels.show();
  lsm6ds33.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  for (int n=0; n<3; n++){
  lsm6ds33.getEvent(&accel, &gyro, &temp);
  gyro_y = gyro.gyro.y*57.29;
    if(gyro_y>700){
      for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        pixels.setPixelColor(i, pixels.Color((5+10*n)*i,10,20+i)); // Moderately bright green color.

       pixels.show(); // This sends the updated pixel color to the hardware.

       delay(delayval-10*n); // Delay for a period of time (in milliseconds).

  }
        for(int i=NUMPIXELS;i>=0;i=i-1){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        pixels.setPixelColor(i, pixels.Color(0,0,0)); // Moderately bright green color.

        pixels.show(); // This sends the updated pixel color to the hardware.

        delay(delayval-10*n); // Delay for a period of time (in milliseconds).

  }
 pixels.clear();
  
    
    }
  }
  

}

void bracelet_together(){
  for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(3,10*i,2+2*i)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(delayval); // Delay for a period of time (in milliseconds).
}
  for(int i=NUMPIXELS;i>=0;i=i-1){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,0,0)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(delayval); // Delay for a period of time (in milliseconds).

  }
 pixels.clear();
  
}




void loop() {
  
  if ( Bluefruit.connected() )
  {
    uint16_t conn_hdl = Bluefruit.connHandle();

    // Get the reference to current connected connection 
    BLEConnection* connection = Bluefruit.Connection(conn_hdl);

    // get the RSSI value of this connection
    // monitorRssi() must be called previously (in connect callback)
    int8_t rssi = connection->getRssi();
    
    Serial.printf("Rssi = %d", rssi);
    Serial.println();

    if(rssi>=-40){
      bracelet_together();
    }
    if(rssi<-40){
      bracelet_alone();
    }
  }
  if (!Bluefruit.connected()){
  bracelet_alone();
  }
  
}

void cent_connect_callback(uint16_t conn_handle) //invoked once a valid peripheral (other bracelet) is found, this happens on both bracelets
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);
  connection->monitorRssi(10); //to be used in loop to retrieve rssi value of connection

  char peer_name[32] = { 0 };
  connection->getPeerName(peer_name, sizeof(peer_name));

  Serial.print("[Cent] Connected to "); //for debugging
  Serial.println(peer_name);;

  if ( clientUart.discover(conn_handle) )
  {
    // Enable TXD's notify
    clientUart.enableTXD();
  }else
  {
    // disconnect since we couldn't find bleuart service
    Bluefruit.disconnect(conn_handle);
  }  
}

void prph_connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char peer_name[32] = { 0 };
  connection->getPeerName(peer_name, sizeof(peer_name));
  connection->monitorRssi(10); //to be used in loop to retrieve rssi value of connection

  Serial.print("[Prph] Connected to ");
  Serial.println(peer_name);


  // Start monitoring rssi of this connection
  // This function should be called in connect callback
  // Input argument is value difference (to current rssi) that triggers callback
 
}

void scan_callback(ble_gap_evt_adv_report_t* report)
{
  // Since we configure the scanner with filterUuid()
  // Scan callback only invoked for device with bleuart service advertised  
  // Connect to the device with bleuart service in advertising packet  
  Bluefruit.Central.connect(report);
  
}
