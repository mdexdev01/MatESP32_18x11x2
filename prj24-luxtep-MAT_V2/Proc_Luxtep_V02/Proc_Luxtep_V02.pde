
/*
 Author : Marveldex Inc. mdex.co.kr
 Contact : area@mdex.co.kr
 Customer : LUXTEP
 Source Ver. : 2024-0608a
 Release : NOT YET
 
 TODO :
  1. 프로토콜 변경 통보 : 18 MUX ==> 20 MUX, 테일에 0XFE 두개가 아닌, 1개.
  2. libPacket 함수들을 libMatrixView 클래스에 fillData함수로 편입.
  3. matView_A를 레퍼런스로 참조.
  4. SerialPort 객체화
  5. 무게 중심
 */


import processing.serial.*;
import controlP5.*; // import controlP5 library


//==========================================================
//  WINDOW SIZE
int window_size_x = 1420;
int window_size_y = 1080;

//==========================================================
//  FOR DEBUG and LOG
EditRect  textLog;

//==========================================================
//  UTILITY FUNCTIONS


void setup() {
  size(1420,800);
  window_size_x = 1420;
  window_size_y = 800;

  surface.setTitle("LED Pressure Mat - Luxtep, V.0.1");
  surface.setResizable(true);
  surface.setLocation(0, 0);

  colorMode(HSB, 360, 100, 100); //  https://codepen.io/HunorMarton/details/eWvewo

  background(0, 0, 85); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  //background(220, 80, 15); // (220, 80, 15) : dark blue, (0, 0, 100) : white
  //background(220, 80, 15);// : dark blue, (0, 0, 100) : white


  setup_Pref();

  if(false == create_MatrixView()) {
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
  if(isPortOpened == false) {
    update_UI();
    draw_MatrixView();
  }
  if(flag_read == false) 
    return;

  calculate_MatrixView();
  draw_MatrixView();

  update_UI();
  update_Save();

  sensor_frame_count++;
  flag_read = false;
}
