/*
 Author : Marveldex Inc. mdex.co.kr
 Contact : area@mdex.co.kr
 Customer : EZGEO
 Source Ver. : 2024-0608a
 Release : NOT YET
 
 TODO :
  1. 프로토콜 변경 통보 : 18 MUX ==> 20 MUX, 테일에 0XFE 두개가 아닌, 1개.
  2. libPacket 함수들을 libMatrixView 클래스에 fillData함수로 편입.
  3. matView_A를 레퍼런스로 참조.
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

//==========================================================
//  UTILITY FUNCTIONS


void setup() {
  // size(1920, 1080);
  size(880, 1000);
  window_size_x = 880;
  window_size_y = 1000;

  surface.setTitle("EZGEO-Air Mat Sensor");
  surface.setResizable(true);
  surface.setLocation(0, 0);

  colorMode(HSB, 360, 100, 100); //  https://codepen.io/HunorMarton/details/eWvewo
  //background(220, 80, 15); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  background(0, 0, 85); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  //background(220, 80, 15);// : dark blue, (0, 0, 100) : white


  setup_Pref();

  if(false == setup_MatrixView()) {
    textLog.showString("[ERROR] Position CSV file error....");
  }
  delay(100);

  setup_UI();
  setup_Save();

  //========================================
  //  THREAD - UART
  thread("taskSerial0xFE_01");


  println(String.format("App launching) %d-%d-%d / %02d:%02d-%02d", year(), month(), day(), hour(), minute(), second() ));
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

    delay(200);
    //    exit();
    return false;
  }

  //  "Silicon Labs CP210x USB to UART Bridge"
  for (int i = 0; i < Serial.list().length; i++) {
    println(Serial.list()[i]);
  }

  //  "Silicon Labs CP210x USB to UART Bridge"
  String portName = PORT_NAME; // "COM17"

  myPort = new Serial(this, portName, 230400); // 115200 : should be same to the setting in Arduino code.

  strBuffer = portName + " port is opened...";

  drawTextLog(strBuffer);
  println(strBuffer);

  return true;
}


int countNull = 0;

boolean flag_read = false;

boolean taskSerial0xFE_01() {
  int available_len = 0;
  int read_len = 0;

  while(true) {
    if ( isPortOpened == false ) {
      // println("port not opened 1");
      delay(500);
      continue;
    }

    available_len = myPort.available();

    if ( available_len < 2) { //If receiving buffer is filled under 2 bytes.
      if (available_len == 0) {
        countNull++;
      }

      if (200 < countNull) {
        drawTextLog("Serial is not connected");
        println("Serial is not connected");
      }

      delay(20);
      continue;
    }


    //  ==> https://processing.org/reference/libraries/serial/Serial_readStringUntil_.html
    read_len = myPort.readBytesUntil(TAIL_SYNC, PacketRawData); // read 1 whole buffer.

    if(read_len == PACKET_LEN_TYPE0) {
      System.arraycopy(PacketRawData, 0, PacketData, 0, PACKET_LEN_TYPE0);
      // println("packet 0 good");
    }
    else {
      // println("read_len = " + read_len + " of " + available_len
      //           + "[0]" + PacketRawData[0] + "[read_len-2]" + PacketRawData[read_len-2] + "[read_len-1]" + PacketRawData[read_len-1]);
      //println(PacketData);
      delay(1);
      continue;
    }

    parseData_EZGEO(PacketData);

    countNull = 0;

    flag_read = true;
  } // while

}
