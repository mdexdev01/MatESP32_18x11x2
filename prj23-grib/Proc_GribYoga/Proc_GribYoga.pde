/*
  Author : Marveldex Inc. mdex-shop.com
 Contact : sales@mdex.co.kr
 Customer : Grib
 Source Ver. : 2023-0208a
 Release : NOT YET
 
 TODO :
 Open serial port manually
 Save (Not replay. )
 Trigger mode ==> Like osciloscope, determination with summation / max cell value / COM Pos (Center of Mass)
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
boolean isPortOpened = false;


int NUM_BLOCK = 2;
int NUM_CELLS_ROW = 16;
int NUM_CELLS_COL = 18;
int NUM_CH_IN_BLOCK = NUM_CELLS_ROW * NUM_CELLS_COL;
int NUM_CH_IN_DIV3_PACKET = NUM_CELLS_ROW * NUM_CELLS_COL / 3;


int PACKET_HEADER_LEN    = 8;
int PACKET_TAIL_LEN      = 2;

int PACKET_LEN_TYPE0     = PACKET_HEADER_LEN + PACKET_TAIL_LEN + NUM_CH_IN_BLOCK;
int PACKET_LEN_TYPE1     = PACKET_HEADER_LEN + PACKET_TAIL_LEN + NUM_CH_IN_BLOCK / 3; // 6 COL OF 18 COLS

int SINGLE_DATA_LEN = 1; // 1: byte, 2: short, 4: int

byte sub_header = 0; // F0 = Left, F1 = Right
byte[] PacketRawData = new byte [NUM_CH_IN_BLOCK * SINGLE_DATA_LEN * 4 + 400 ]; // 400 : in case of overflow
byte[] PacketData = new byte [NUM_CH_IN_BLOCK * 2 ]; // 400 : in case of overflow


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

int byteToInt(byte num8) {
  int num32 = 0;
  if(num8 < 0) {
    num32 = 256 + num8;
  }
  else {
    num32 = num8;
  }
  return num32;
}

int intToByte(int num32) {
  byte num8 = 0;
  if (127 < num32) {
    num8 = (byte)(num32 - 256);
  }
  else 
    num8 = (byte)num32;
  
  return num8;
}

void setup() {
  // size(1920, 1080);
  size(880, 1000);
  window_size_x = 880;
  window_size_y = 1000;

  surface.setTitle("Smart Yoga Mat Ver.1.0 - Grib");
  surface.setResizable(true);
  surface.setLocation(0, 0);

  colorMode(HSB, 360, 100, 100); //  https://codepen.io/HunorMarton/details/eWvewo
  //background(220, 80, 15); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  background(0, 0, 85); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  //background(220, 80, 15);// : dark blue, (0, 0, 100) : white

  textLog = new EditRect(1, 1, 300, 20);
  textLog.setTextLeftMargin(3);
  textLog.showString("Open serial port please....");


  setup_Pref();

  setup_EditView();
  setup_Save();
  setup_UI();

  //========================================
  //  THREAD - UART
  thread("readData_Grib");


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


int sensor_frame_count = 0;

void draw2() {
  boolean result_read = readData_Grib();
  
  if (result_read == true) {
    parseData_GRIB(PacketData);
    sensor_frame_count++;

    //calc_copi();
  }
  else {
    // println("parse error");
  }

  if( VB_Filled_Board0 == true) {
    update_EditView();
    VB_Filled_Board0 = false;
  }
  
  // update_UI();

  if (result_read == true) {
    update_Save();
  }
}

void draw() {

  if(flag_read == true) {
    sensor_frame_count++;

    update_Save();

    flag_read = false;
  }
  update_EditView();

}


//  Serial example
//    ==> https://processing.org/reference/libraries/serial/Serial_readStringUntil_.html
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

  strBuffer = " " + portName + " port is opened...";
  drawTextLog(strBuffer);
  println(strBuffer);
  delay(50);

  return true;
}


int countNull = 0;

boolean flag_read = false;

boolean readData_Grib() {
  while(true) {
    if ( isPortOpened == false ) {
      // println("port not opened");
      delay(1);
      continue;
    }

    int available_len = myPort.available();
    if ( available_len < 2) { //If receiving buffer is filled under 2 bytes.
      if (available_len == 0) {
        countNull++;
      }
      if (100 < countNull)
        drawTextLog("Serial is not connected");

      // println("available only " + available_len);

      delay(1);
      continue;
    }


    // int read_len = myPort.readBytesUntil(0xFE, PacketData); // read 1 whole buffer.
    int read_len = myPort.readBytesUntil(0xFE, PacketRawData); // read 1 whole buffer.

    boolean good_packet = false;
    if(read_len == PACKET_LEN_TYPE0) {
        System.arraycopy(PacketRawData, 0, PacketData, 0, PACKET_LEN_TYPE0);
        good_packet = true;
    }
    else if(read_len == PACKET_LEN_TYPE1) {
        System.arraycopy(PacketRawData, 0, PacketData, 0, PACKET_LEN_TYPE1);
        good_packet = true;
    }
    else {  //  else a 
      if(read_len < PACKET_LEN_TYPE1) {
        // println("lack of read :" + read_len);
        // println(PacketData);
        delay(1);
        continue;
      }
      else if(read_len < PACKET_LEN_TYPE0) {
        int offset1 = (read_len - PACKET_LEN_TYPE1);

        if( PacketRawData[read_len-2] == 16   && 
            PacketRawData[offset1] == intToByte(0xFF) && PacketRawData[offset1+1] == intToByte(0xFF) && 
            PacketRawData[read_len-1] == intToByte(0xFE)) { // board 1
          System.arraycopy(PacketRawData, (read_len - PACKET_LEN_TYPE1), PacketData, 0, PACKET_LEN_TYPE1);
          good_packet = true;
        }
      }
      else {//  else b
        int offset0 = (read_len - PACKET_LEN_TYPE0);
        int offset1 = (read_len - PACKET_LEN_TYPE1);

        if( PacketRawData[read_len-2] == 0 && 
            PacketRawData[offset0] == intToByte(0xFF) && PacketRawData[offset0+1] == intToByte(0xFF) && 
            PacketRawData[read_len-1] == intToByte(0xFE)) { // board 0 
          System.arraycopy(PacketRawData, (read_len - PACKET_LEN_TYPE0), PacketData, 0, PACKET_LEN_TYPE0);
          good_packet = true;
        }
        else if(  PacketRawData[read_len-2] == 16  && 
                  PacketRawData[offset1] == intToByte(0xFF) && PacketRawData[offset1+1] == intToByte(0xFF) && 
                  PacketRawData[read_len-1] == intToByte(0xFE)) { // board 1
          System.arraycopy(PacketRawData, (read_len - PACKET_LEN_TYPE1), PacketData, 0, PACKET_LEN_TYPE1);
          good_packet = true;
        }

      } //  else b

    }// else a

    if( good_packet == false ) {
      println("read_len = " + read_len + " of " + available_len
                + "[0]" + PacketRawData[0] + "[read_len-2]" + PacketRawData[read_len-2] + "[read_len-1]" + PacketRawData[read_len-1]);
    }


  /*
    println("matrix) read length = " + read_len);
    println("header = " + PacketData[0]);

    for (int k = 0; k < NUM_BLOCK; k++) {
      for (int col = 0; col < NUM_CELLS_COL; col++) {
        for (int row = 0; row < NUM_CELLS_ROW; row++ ) {
          int xy_pos = (row + NUM_CELLS_ROW * col)  * SINGLE_DATA_LEN;

          int value = byteToInt(PacketData[HEADER_LEN + xy_pos]);

          print(value + ", ");
        }
        println("*");
      }
      println("=================");
    }
  */
    // if (PacketData[5] == 0) {
    //   for (int i = PACKET_HEADER_LEN ; i < (PACKET_LEN_TYPE0 - PACKET_TAIL_LEN) ; i++) {
    //     PacketData[i] = (byte)(10 + (i - PACKET_HEADER_LEN) / 2);
    //   }
    // }
    // else if ( (PacketData[5] == 1) ) {
    //   for (int i = PACKET_HEADER_LEN ; i < (PACKET_LEN_TYPE1 - PACKET_TAIL_LEN) ; i++) {
    //     PacketData[i] = (byte)(60 + (i - PACKET_HEADER_LEN) / 4);
    //   }
    //   // println("start index = " + PacketData[6]);
    // }

    parseData_GRIB(PacketData);

    countNull = 0;

    flag_read = true;
  } // while

}

boolean readData_Grib2() {
  if ( isPortOpened == false ) {
    println("port not opened");
    return false;
  }

  int available_len = myPort.available();
  if ( available_len < 2) { //If receiving buffer is filled under 2 bytes.
    if (available_len == 0) {
      countNull++;
    }
    if (100 < countNull)
      drawTextLog("Serial is not connected");

    // println("available only " + available_len);

    return false;
  }


  int read_len = myPort.readBytesUntil(0xFE, PacketData); // read 1 whole buffer.
  // println("read_len = " + read_len);

  if (read_len < NUM_CH_IN_DIV3_PACKET) {
    print("lack of read :");
    println(read_len);
    // println(PacketData);
    return false;
  }

/*
  println("matrix) read length = " + read_len);
  println("header = " + PacketData[0]);

  for (int k = 0; k < NUM_BLOCK; k++) {
    for (int col = 0; col < NUM_CELLS_COL; col++) {
      for (int row = 0; row < NUM_CELLS_ROW; row++ ) {
        int xy_pos = (row + NUM_CELLS_ROW * col)  * SINGLE_DATA_LEN;

        int value = byteToInt(PacketData[HEADER_LEN + xy_pos]);

        print(value + ", ");
      }
      println("*");
    }
    println("=================");
  }
*/

  countNull = 0;

  flag_read = true;

  return true;
}

