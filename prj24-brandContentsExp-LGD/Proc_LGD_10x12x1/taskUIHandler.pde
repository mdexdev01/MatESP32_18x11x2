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

  bgi_logo_120ch = loadImage("./res/logo-Title.jpg");
  image(bgi_logo_120ch, window_size_x - 220, 15);

  bgi_logo_mdex = loadImage("./res/logo_mdex(kor).png");
  image(bgi_logo_mdex, window_size_x - 200, window_size_y - 50);

  setup_cp5();
}

void setup_cp5() {
  cp5 = new ControlP5(this);
  
  PFont font_small = createFont("TAHOMA",14);
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
                      .setPosition(pos_x, pos_y + 6)
                      //.setColorValue(0xff555555)
                      .setColorValue(0xEE0000BF)
                      .setFont(createFont("Tahoma",14))
                      ;
                      //.setColorBackground(0xEE000000)
                      //.setColorForeground(0xff222222)
                      //.setColorValue(0xFFEEFFEE)
                      //.setColorLabel(0xEE0000BF)    

    cp5.addTextfield(SerialNoteLabel)
                      .setPosition(pos_x + 80, pos_y)
                      .setSize(25,25)
                      .setText(userPref.getValue(userPref.itemPort1))
                      .setFont(createFont("Tahoma",12))
                      .setColorBackground(0xEE000000)
                      .setColorForeground(0xff222222)
                      .setColorValue(0xFFEEFFEE)
                      .setColorLabel(0xFFFFFFFF)
                      //.setColor(hsb_cyan)
                      ;

    // cp5.addTextfield(".")
    //     .setPosition(pos_x + 110, pos_y)
    //     .setSize(25,25)
    //     .setText(userPref.getValue(userPref.itemPort2))
    //     .setFont(createFont("Arial",12))
    //     .setColor(hsb_cyan);

    cp5.addButton("OPEN SERIAL").setPosition(pos_x + 140, pos_y).setSize(55, 25).setLabel("Open").setFont(createFont("Arial",14));

  }
  
  //----------------------------------------
  //  SCREEN SHOT, SAVE CSV
  {
    pos_x = window_size_x - 230;
    pos_y +=  150;
    int button_height = 40;
    cp5.addButton("Screen shot").setPosition(pos_x, pos_y).setSize(200, button_height)
                    .setLabel("Screen shot").setFont(font_mid);

    pos_y += (button_height + 20);
    cp5.addButton(CSV_BTN_TAG).setPosition(pos_x, pos_y).setSize(200, button_height)
                    .setLabel(CSV_BTN_FALSE).setFont(font_mid);

    csv_btn_state = false;
  }
  
  //----------------------------------------
  //  FILE NAME
  {
    pos_x = window_size_x - 230;
    pos_y += 80;

    int control_width = 0;

    control_width = 90;
    cp5.addTextlabel("tag_FILENAME")
                      .setText("SAVE FILE NAME (CSV/PNG) :")
                      .setPosition(pos_x, pos_y)
                      //.setColorValue(0xff0060ff)
                      .setColorValue(0xff000088)
                      //.setColorValue(0xffff60ff)
                      .setFont(font_small)
                      ;

    pos_y += 20;
    control_width = 200;

    erFileName = new EditRect(pos_x + 0, pos_y, control_width, 19);
    erFileName.setFontSize(13);
    erFileName.setTextLeftMargin(2);
    erFileName.setTextTopMargin(13);
    erFileName.render();


    pos_x += 6;
    pos_y += 30;
    control_width = 70;
    cp5.addTextfield("NAME")
        .setPosition(pos_x, pos_y).setSize(control_width, 25)
        .setText("LGD_Line3")
        .setFont(font_small)
        .setColorBackground(0xEE000000)
        .setColorForeground(0xff222222)
        .setColorValue(0xFFEEFFEE)
        .setColorLabel(0xEE0000BF)
        ;
    
    pos_x += (control_width + 20);
    //pos_y += 50;
    cp5.addTextfield("MEMO")
        .setPosition(pos_x, pos_y).setSize(control_width, 25)
        .setText("Pos2")
        .setFont(font_small)
        .setColorBackground(0xEE000000)
        .setColorForeground(0xff222222)
        .setColorValue(0xFFEEFFEE)
        .setColorLabel(0xEE0000BF)
        ;
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
    }
    println("ui handler ended...");
  }
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

    fill(0xffdddddd); // HUE, SATURATE, BRIGHTNESS : black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    erFileName.render();
    erFileName.showString(strFileInfo + ".csv (OR .png)");

    pop();

    update_count++;
}
