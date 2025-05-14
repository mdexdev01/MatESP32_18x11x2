ControlP5 cp5; // https://sojamo.de/libraries/controlP5/

//---------------------------------------------
//  UI coordinate
String strPathCSV = "Save_CSV/";
String strPathShot = "Save_ScreenShot/";

String strFooterCSV = ".csv";
String strFooterShot = ".png";

String strFileInfo;
EditRect  erFileName;

Textlabel    textTimer;

PImage    bgi_logo_120ch; // Background image
PImage    bgi_logo_mdex; // Background image


//---------------------------------------------
//  FUNCTION DEFINITION
//---------------------------------------------
void drawTextLog(String strText) {
  fill(0, 0, 90); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100)
  textLog.render();
  textLog.showString(strText);

  println(strText);
}


void setup_UI() {

  bgi_logo_120ch = loadImage("./res/logo-Title.png");
  image(bgi_logo_120ch, window_size_x - 220, 30);

  bgi_logo_mdex = loadImage("./res/logo_mdex(kor).png");
  image(bgi_logo_mdex, window_size_x - 200, window_size_y - 50);

  setup_cp5();
}

void setup_cp5() {
  cp5 = new ControlP5(this);
  
  PFont font_small = createFont("arial",14);
  PFont font_mid   = createFont("arial",24);
  PFont font_large = createFont("arial",40);
 
  color hsb_cyan = color(180, 50, 100);
  color hsb_yellow = color(60, 100, 100);
  color hsb_white = color(0, 0, 100);

 //  align controls
  int pos_x = 120;
  int pos_y = 100;

  textTimer = new Textlabel(cp5,"--",100,100);
  textTimer.setPosition(pos_x, pos_y);


  pos_x = window_size_x - 250;
  pos_y = 20;
  EditRect lineRect = new EditRect(pos_x, pos_y, 2, window_size_y - (pos_y + 10));
  lineRect.render();


  {
    pos_x = window_size_x - 230;
    pos_y = 120;

    textLog = new EditRect(pos_x, pos_y, 220, 20);
    textLog.setTextLeftMargin(3);
    // textLog.showString("Open serial port please....");

    String portInfo = "USB Port : ";
    //  "Silicon Labs CP210x USB to UART Bridge"
    for (int i = 0; i < Serial.list().length; i++) {
      println("Found : " + Serial.list()[i]);
      portInfo += Serial.list()[i];
      portInfo += ", ";
    }
    textLog.showString(portInfo);

  }


  //----------------------------------------
  //  SERIAL PORT
  {
    pos_y = 150;
    cp5.addTextlabel("label_Serial")
                      .setText("Serial Port :")
                      .setPosition(pos_x, pos_y)
                      .setColorValue(0xff555555)
                      .setFont(createFont("Arial",14));
    
    cp5.addTextfield(SerialNoteLabel)
                      .setPosition(pos_x + 80, pos_y)
                      .setSize(25,25)
                      .setText(userPref.getValue(userPref.itemPort1))
                      .setFont(createFont("Arial",12))
                      .setColor(hsb_cyan);

    // cp5.addTextfield(".")
    //     .setPosition(pos_x + 110, pos_y)
    //     .setSize(25,25)
    //     .setText(userPref.getValue(userPref.itemPort2))
    //     .setFont(createFont("Arial",12))
    //     .setColor(hsb_cyan);

    cp5.addButton("OPEN SERIAL").setPosition(pos_x + 140, pos_y).setSize(55, 25).setLabel("Open").setFont(createFont("Arial",14));

  }
  
  {
    pos_y += 50;
    //cp5.addTextfield(SerialNoteLabel)
    //                  .setPosition(pos_x + 80, pos_y)
    //                  .setSize(25,25)
    //                  .setText(userPref.getValue(userPref.itemPort1))
    //                  .setFont(createFont("Arial",12))
    //                  .setColor(hsb_cyan);


    cp5.addButton("tagSEND").setPosition(pos_x + 140, pos_y).setSize(55, 25).setLabel("Send").setFont(createFont("Arial",14));
  }
  
  //----------------------------------------
  //  FILE NAME
  {
    // pos_x = 5;
    pos_y = window_size_y - 450;

    int control_width = 0;

    
    control_width = 90;
    cp5.addTextlabel("tag_FILENAME")
                      .setText("File Name(Eng) :")
                      .setPosition(pos_x, pos_y)
                      .setColorValue(0xff0060ff)
                      .setFont(font_small)
                      ;

    pos_y += 30;
    control_width = 180;
    cp5.addTextfield("NAME")
        .setPosition(pos_x + 10, pos_y).setSize(control_width, 25)
        .setText("LUXTEP-Mat")
        .setFont(font_small)
        .setColor(hsb_cyan)
        ;
    
    pos_y += 50;
    cp5.addTextfield("MEMO")
        .setPosition(pos_x + 10, pos_y).setSize(control_width, 25)
        .setText("V01")
        .setFont(font_small)
        .setColor(hsb_cyan)
        ;

    control_width = 230;

    // pos_x = 100 + 250;
    pos_y += 70;
    control_width = 200;

    erFileName = new EditRect(pos_x, pos_y, control_width, 22);
    erFileName.setFontSize(13);
    erFileName.setTextLeftMargin(2);
    erFileName.setTextTopMargin(13);
  }

  //----------------------------------------
  //  SCREEN SHOT, SAVE CSV
  {
    // pos_x = 15;
    pos_y +=  40;
    cp5.addButton("Screen shot").setPosition(pos_x, pos_y).setSize(200, 70)
                    .setLabel("Screen shot").setFont(font_mid);

    pos_y += 80;
    cp5.addButton(CSV_BTN_TAG).setPosition(pos_x, pos_y).setSize(200, 70)
                    .setLabel(CSV_BTN_FALSE).setFont(font_mid);

    csv_btn_state = false;
  }
}

boolean csv_btn_state = false;
String SerialNoteLabel = "(Number only)";

String CSV_BTN_TAG    = "CSV_START_STOP";
String CSV_BTN_FALSE  = "SAVE DATA";
String CSV_BTN_TRUE   = "STOP";

///////////////////////////////////////////////////////////////////////////
//    EVENT HANDLER
///////////////////////////////////////////////////////////////////////////

// combo function : selectInput("Select a file to process:", "fileSelected");
void fileSelected(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
  } else {
    println("User selected " + selection.getAbsolutePath());
  }
}

int tx_count = 0;

void controlEvent(CallbackEvent event) {
  String strBuf, strBuf2;
  boolean result;
  int value_i;
  float value_f;

  if (event.getAction() == ControlP5.ACTION_CLICK) {
    println("\t> ACTION_CLICK : " + event.getController().getAddress());

    switch(event.getController().getAddress()) {
      case "/OPEN SERIAL":
        strBuf = cp5.get(Textfield.class, SerialNoteLabel).getText();
        println("Button open Pressed : " + strBuf);
        // strBuf2= cp5.get(Textfield.class, ".").getText();
        // println("Button open Pressed : " + strBuf + ", " + strBuf2);
        
        //  1st Serial Port
        openSerialPort(String.format("COM%s", strBuf));

        //  2nd Serial Port
        // {
        //   // String com2Names = String.format("COM%s", userPref.getValue(userPref.itemPort2));
        //   String com2Names = String.format("COM%s", strBuf2);
        //   println("Port2 Name : " + com2Names);
        //   openSerialPort2(com2Names);
        // }

        //  save preference
        {
          userPref.setValue(userPref.itemPort1, strBuf);
          // userPref.setValue(userPref.itemPort2, strBuf2);
          userPref.saveTable_CSV();
          
          cp5.get(Textfield.class, SerialNoteLabel).setLock(true);
          event.getController().setColorValue(0x5e5e5e);
          //event.getController().setLock(true);
        }
        break;

      case "/CSV_START_STOP":
        if(false == isPortOpened) {
          drawTextLog("Can't save. Please open serial port");
          break;
        }

        //--------------------------------------------
        //  toggle button state and text
        if(false == csv_btn_state) {   //  SAVE STARTS
          csv_btn_state = true;
          event.getController().setLabel(CSV_BTN_TRUE);

          MATRIX_SAVE_CSV_open(matView_A[0]);          
          timerSaveCSV.reset();
          timerSaveCSV.cycleStart(SAVE_INTERVAL_MS);
        }
        else {    //  SAVE ENDS
          csv_btn_state = false;
          event.getController().setLabel(CSV_BTN_FALSE);

          MATRIX_SAVE_CSV_close();
        }
        break;

      case "/Screen shot":
        String text_field = cp5.get(Textfield.class, "NAME").getText();

        String temp_str = strPathShot + "/" + text_field + "/" + getCurTimeString() + "_" + strFileInfo + ".png";
        println(temp_str);
        saveFrame(temp_str);
        break;
      case "/tagSEND":
        println("Send data");
        for(int j = 0 ; j < 20 ; j++) {
          test_OSD();
          tx_count++;
          delay(100);
          //if( (tx_count % 2) == 0)
          //  delay(40);
        }
        break;
    }
    println("ui handler ended...");
  }
}

byte[] packetOSD = new byte[8*56*(34+1)];

void test_OSD() {
  int packet_head_len = 16;
  int packet_tail_len = 2;
  
  int osd_start_x = 0 + (tx_count % 16);
  int osd_start_y = 0 + (tx_count % 27);
  int osd_width = 12;//6; // max : 28
  int osd_height = 8;//12; // max : 35

  int packet_max_size = packet_head_len + osd_width * osd_height + packet_tail_len;

  byte [] osd_packet = new byte[packet_max_size];

  buildOSDPacket(osd_packet, osd_start_x, osd_start_y, osd_width, osd_height);
  
  //  send to uart
  //for(int i = 0 ; i < (packet_head_len + osd_size) ; i++) {
    myPort.write(osd_packet); // read 1 whole buffer.
  //}

}

void buildOSDPacket(byte [] packetBuf, int start_x, int start_y, int osd_width, int osd_height) {
  int header_len = 8;
  int sub_header_len = 8;
  int tail_len = 2;
  int data_len = osd_width * osd_height;
  int packet_body_len = sub_header_len + data_len + tail_len;  
  
  packetBuf[0] = -1; // 0xFF
  packetBuf[1] = -1; // 0xFF
  packetBuf[2] = byte(tx_count % 100); // version major
  packetBuf[3] = 0; // tx board id  ㅑㅜㄴㅇ
  
  packetBuf[4] = 3; // group id, 3:G_OSD_COMMAND
  
  byte rx_board_id = byte(tx_count % 2);
  //  [5] : (osd_id << 4) | rx_board_id 
  //data[5] = byte((0 << 4) | (tx_count % 2)); // rx osd id, 0: board 0, 1: board 1
  packetBuf[5] = byte((0 << 4) | rx_board_id); // rx osd id, 0: board 0, 1: board 1
  
  packetBuf[6] = byte(packet_body_len / 100); // data len, over 100
  packetBuf[7] = byte(packet_body_len % 100); // data len, under 100
  
  packetBuf[8] = (byte)start_x;
  packetBuf[9] = (byte)start_y;
  packetBuf[10] = (byte)osd_width;
  packetBuf[11] = (byte)osd_height;
  packetBuf[12] = -17; // duration, 100ms. Doesn't work yet
  packetBuf[13] = 100; // brightness. Doesn't work yet
  packetBuf[14] = 0; // res 0
  packetBuf[15] = 0; // res 1

  
  //int offset = header_len + sub_header_len;
  //int number = 0;
  int index = header_len + sub_header_len;

  for(int y = 0 ; y < osd_height ; y++) {
    for(int x = 0 ; x < osd_width ; x++) {
      int position = header_len + sub_header_len + y * osd_width + x;

      packetBuf[index] = byte(5+x + tx_count % 80) ;//0xEF; Transparent

      //if( (17 < x) && (17 < x) ) {
      //  packetBuf[index] = -17;//0xEF; Transparent
      //}
      
      //if(((tx_count % 8) == 6) || ((tx_count % 8) == 7)) {
      //  packetBuf[index] = -17;//0xEF; Transparent
      //}
      
      index++;
    }
  }
  
  packetBuf[header_len + sub_header_len + data_len] = byte(tx_count % 100);
  packetBuf[header_len + sub_header_len + data_len + 1] = -2; // 0xFE

}


int update_count = 0;

void update_UI() {
    push();

    String text_field;
    
    strFileInfo = "";
    
    text_field = cp5.get(Textfield.class, "NAME").getText();
    strFileInfo += text_field;
    
    text_field = "_" + cp5.get(Textfield.class, "MEMO").getText();
    strFileInfo += text_field;

    fill(0, 0, 100);// black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100)   
    erFileName.render();
    erFileName.showString(strFileInfo + ".csv(or .png)");

    pop();

    update_count++;
}
