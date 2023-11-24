/*
  Author : Marveldex Inc. mdex.co.kr
 Contact : area@mdex.co.kr
 Customer : KRRI
 Source Ver. : 2023-1108a
 Release : NOT YET
 
 TODO :
 */

import processing.serial.*;
import controlP5.*; // import controlP5 library


String  PORT_NAME = "COM17"; // "Silicon Labs CP210x USB to UART Bridge"

//==========================================================
//  COLORMAP RANGE
int max_cell_val = 200;
int colorMap(int cell_value) {
  /*
   cell range : about 0~240. usually 0~150
   
   hue range : use only 180~0
   color table of HUE, SAT, BRI : https://codepen.io/HunorMarton/details/eWvewo
   
   map function : https://processing.org/reference/map_.html
   map(current value, source min, source max, target min, target max)
   */

  float hue_val = map(cell_value, 0, 140, 180, 0); // if value is over 140, then it's ceiled by 140. if 141 or 150 or 240, then it's ceiled to 140.
  int hue = (int)hue_val;

  if (max_cell_val < cell_value)
    hue = 0;

  return hue;
}


//==========================================================
//  SERIAL AND PACKET BUFFER STRUCTURE
Serial myPort;
Serial myPort2;
boolean isPortOpened = false;
boolean isPortOpened2 = false;


//==========================================================
//  FOR DEBUG and LOG
// int window_size_x = 1920;
// int window_size_y = 1080;
int window_size_x = 1220;
int window_size_y = 1080;

int dashboard_x = 1150;
int dashboard_y = window_size_y - 120;

//==========================================================
//  FOR DEBUG and LOG
String    strBuffer;
EditRect  textLog;

boolean isDebug = false;


//==========================================================
//  UTILITY FUNCTIONS
void drawTextLog(String strText) {
  fill(0, 0, 90); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100)
  textLog.render();
  textLog.showString(strText);
}


void setup() {
  // size(1920, 1080);
  size(880, 1000);
  window_size_x = 880;
  window_size_y = 1000;

  surface.setTitle("Highpass mat Ver.1.0 - KRRI");
  surface.setResizable(true);
  surface.setLocation(0, 0);

  colorMode(HSB, 360, 100, 100); //  https://codepen.io/HunorMarton/details/eWvewo
  //background(220, 80, 15); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  background(0, 0, 85); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  //background(220, 80, 15);// : dark blue, (0, 0, 100) : white

  textLog = new EditRect(1, 40, 250, 20);
  textLog.setTextLeftMargin(3);
  textLog.showString("Open serial port please....");


  setup_Pref();

  if(false == setup_MatrixView()) {
    textLog.showString("[ERROR] Position CSV file error....");
  }
  delay(100);

  setup_UI();
  setup_Save();

  //========================================
  //  THREAD - UART
  thread("readData_Grib_A");
  thread("readData_Grib_C");


  //========================================
  {
    int d = day();    // Values from 1 - 31
    int mo = month();  // Values from 1 - 12
    int y = year();   // 2003, 2004, 2005, etc.

    int s = second();  // Values from 0 - 59
    int m = minute();  // Values from 0 - 59
    int h = hour();    // Values from 0 - 23

    //String time_string = String.format("%4d%02d%02d-%02d%02d%02d", y, mo, d, h, m, s);
    String time_string = String.format("%d-%d-%d---%d-%d-%d", y, mo, d, h, m, s);

    println(time_string);
  }

}

int sensor_frame_count = 0;

void draw() {

  if(flag_read == true) {
    sensor_frame_count++;

    update_UI();
    update_Save();

    flag_read = false;
  }
  update_EditView();

}


//  Serial example
//    ==> https://processing.org/reference/libraries/serial/Serial_readStringUntil_.html
void openSerialPort(String portName) {
  PORT_NAME = portName;

  //  open serial port
  if (false == openSerialOrExit()) {
    println("Failed to connect with Arduino. Please check connection.");
    return;
  } else {
    isPortOpened = true;
  }
}

//  Serial example
//    ==> https://processing.org/reference/libraries/serial/Serial_readStringUntil_.html
void openSerialPort2(String portName) {
  try {
    myPort2 = new Serial(this, portName, 230400); // 115200 : should be same to the setting in Arduino code.
  } catch (Exception e) {
      println(e.toString());
      isPortOpened2 = false;
      return;
  }  

  isPortOpened2 = true;
  strBuffer = portName + " port is opened...";
  println(strBuffer);
}


boolean openSerialOrExit() {
  if (Serial.list().length == 0) {
    strBuffer = "ERROR ! Can't find serial port. Connect arduino and restart.";
    drawTextLog(strBuffer);
    println(strBuffer);

    delay(200);
    //    exit();
    return false;
  }

  //  "Silicon Labs CP210x USB to UART Bridge"
  for (int i = 0; i < Serial.list().length; i++) {
    println(Serial.list()[i]);
  }

  //  "Silicon Labs CP210x USB to UART Bridge"
  //String portName = Serial.list()[0]; // [0] means 0th port in the COM list
  String portName = PORT_NAME; // "COM17"

  myPort = new Serial(this, portName, 230400); // 115200 : should be same to the setting in Arduino code.
  //myPort = new Serial(this, portName, 921600); // 115200 : should be same to the setting in Arduino code.

  strBuffer = portName + " port is opened...";
  drawTextLog(strBuffer);
  println(strBuffer);
  delay(50);

  return true;
}


int countNull = 0;

boolean flag_read = false;

boolean readData_Grib_A() {
  while(true) {
    if ( isPortOpened == false ) {
      // println("port not opened 1");
      delay(500);
      continue;
    }

    int available_len = myPort.available();
    if ( available_len < 2) { //If receiving buffer is filled under 2 bytes.
      if (available_len == 0) {
        countNull++;
      }
      if (100 < countNull) {
        drawTextLog("Serial is not connected");
        println("Serial is not connected");
      }

      delay(20);
      continue;
    }


    int read_len = myPort.readBytesUntil(0xFE, PacketRawData); // read 1 whole buffer.

    boolean good_packet = false;
    if(read_len == PACKET_LEN_TYPE0) {
        System.arraycopy(PacketRawData, 0, PacketData, 0, PACKET_LEN_TYPE0);
        // println("packet 0 good");
        good_packet = true;
    }
    else {  //  else a 
         //println("lack of read :" + read_len);
         //println(PacketData);
        delay(1);
        continue;
    }// else a


    if( good_packet == false ) {
      println("read_len = " + read_len + " of " + available_len
                + "[0]" + PacketRawData[0] + "[read_len-2]" + PacketRawData[read_len-2] + "[read_len-1]" + PacketRawData[read_len-1]);
    }

    parseData_GRIB(PacketData);

    countNull = 0;

    flag_read = true;
  } // while

}

boolean readData_Grib_C() {
  while(true) {
    if ( isPortOpened2 == false ) {
      // println("port not opened 1");
      delay(500);
      continue;
    }

    int available_len = myPort2.available();
    if ( available_len < 2) { //If receiving buffer is filled under 2 bytes.
      if (available_len == 0) {
        countNull++;
      }
      if (100 < countNull) {
        drawTextLog("Serial is not connected");
        println("Serial is not connected");
      }

      delay(20);
      continue;
    }


    int read_len = myPort2.readBytesUntil(0xFE, PacketRawData2); // read 1 whole buffer.

    boolean good_packet = false;
    if(read_len == PACKET_LEN_TYPE0) {
        System.arraycopy(PacketRawData2, 0, PacketData2, 0, PACKET_LEN_TYPE0);
        // println("packet 0 good");
        good_packet = true;
    }
    else {  //  else a 
         //println("lack of read :" + read_len);
         //println(PacketData2);
        delay(1);
        continue;
    }// else a


    if( good_packet == false ) {
      println("read_len = " + read_len + " of " + available_len
                + "[0]" + PacketRawData2[0] + "[read_len-2]" + PacketRawData2[read_len-2] + "[read_len-1]" + PacketRawData2[read_len-1]);
    }

    parseData_GRIB(PacketData2);

    countNull = 0;

    flag_read = true;
  } // while

}
