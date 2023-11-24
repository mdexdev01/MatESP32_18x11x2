ControlP5 cp5; // https://sojamo.de/libraries/controlP5/

//---------------------------------------------
//  UI coordinate
String strPathCSV = "CSV_Data/";
String strPathShot = "ScreenShot/";

String strFooterCSV = ".csv";
String strFooterShot = ".png";

String strFileInfo;
EditRect  erFileName;

Textlabel    textTimer;

PImage    bgi_logo_120ch; // Background image
PImage    bgi_logo_mdex; // Background image

void setup_UI() {

  bgi_logo_120ch = loadImage("./res/logo_Title.png");
  image(bgi_logo_120ch, window_size_x - 300, 0);

  bgi_logo_mdex = loadImage("./res/logo_mdex(kor).png");
  image(bgi_logo_mdex, window_size_x - 220, window_size_y - 50);

  setup_cp5();
}

void setup_cp5() {
  cp5 = new ControlP5(this);
  
  PFont font_small = createFont("arial",14);
  PFont font_mid   = createFont("arial",24);
  PFont font_large = createFont("arial",40);
  //  align controls
  int pos_x = 120;
  int pos_y = 100;

  textTimer = new Textlabel(cp5,"--",100,100);
  textTimer.setPosition(pos_x, pos_y);

  //----------------------------------------
  //  SERIAL PORT
  {
    pos_x = window_size_x - 230;
    pos_y = 80;

    EditRect lineRect = new EditRect(pos_x-10, pos_y, 3, window_size_y - pos_y);
    lineRect.render();


    pos_y = 150;
    cp5.addTextlabel("label_Serial")
                      .setText("Serial Port :")
                      .setPosition(pos_x, pos_y)
                      .setColorValue(0xff555555)
                      .setFont(createFont("Arial",14))
                      //.setFont(font_small)
                      ;
    
    cp5.addTextfield(SerialNoteLabel)
        .setPosition(pos_x + 90, pos_y)
        .setSize(30,25)
        .setText(userPref.getValue(userPref.itemPort1))
        .setFont(createFont("Arial",12))
        //.setFont(font)
        .setFocus(true)
        .setColor(color(0, 0, 100))
        ;

    cp5.addButton("OPEN SERIAL").setPosition(pos_x + 130, pos_y).setSize(55, 25).setLabel("Open").setFont(createFont("Arial",14));

  }
  
  //----------------------------------------
  //  FILE NAME
  {
    // pos_x = 5;
    pos_y = window_size_x - 350;

    int control_width = 0;
    
    control_width = 90;
    cp5.addTextlabel("tag_FILENAME")
                      .setText("File Name(Eng) :")
                      .setPosition(pos_x, pos_y)
                      .setColorValue(0xff0060ff)
                      .setFont(font_small)
                      ;

    // pos_x += control_width + 25;
    pos_y += 30;
    control_width = 180;
    cp5.addTextfield("NAME")
        .setPosition(pos_x, pos_y).setSize(control_width-50, 25)
        .setText("John Doe")
        .setFont(font_small)
        .setColor(color(0, 0, 100))
        ;
    
    // pos_x += control_width - 35;
    pos_y += 50;
    control_width = 70;
    cp5.addTextfield("MEMO")
        .setPosition(pos_x, pos_y).setSize(control_width, 25)
        .setText("MEMO")
        .setFont(font_small)
        .setColor(color(0, 0, 100))
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
void fileSelected(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
  } else {
    println("User selected " + selection.getAbsolutePath());
  }
}


void controlEvent(CallbackEvent event) {
  String strBuf;
  boolean result;
  int value_i;
  float value_f;

  if (event.getAction() == ControlP5.ACTION_CLICK) {
    switch(event.getController().getAddress()) {
      case "/OPEN SERIAL":
        println("OPEN SERIAL");
        strBuf = cp5.get(Textfield.class, SerialNoteLabel).getText();
        println("Button open Pressed : " + strBuf);
        
        //  1st Serial Port
        {
          String comNames = String.format("COM%s", strBuf);
          println("Port Name : " + comNames);
          openSerialPort(comNames);
        }

        //  2nd Serial Port
        {
          String com2Names = String.format("COM%s", userPref.getValue(userPref.itemPort2));
          // String com2Names = String.format("COM%s", "33");
          println("Port2 Name : " + com2Names);
          openSerialPort2(com2Names);
        }

        //  save preference
        {
          userPref.setValue(userPref.itemPort1, strBuf);
          userPref.saveTable_CSV();
          
          cp5.get(Textfield.class, SerialNoteLabel).setLock(true);
          event.getController().setColorValue(0x5e5e5e);
          //event.getController().setLock(true);
        }
        break;

      case "/CSV_START_STOP":
        println("CSV_START_STOP");
        if(false == isPortOpened) {
          strBuffer = "Can't save. Please open serial port";

          drawTextLog(strBuffer);
          println(strBuffer);
          
          break;
        }

        //  toggle button state and text
        if(false == csv_btn_state) {
          //  SAVE ON
          csv_btn_state = true;
          event.getController().setLabel(CSV_BTN_TRUE);
          saveCSV_open();
          
          timerSaveCSV.reset();
        }
        else {
          //  SAVE OFF
          csv_btn_state = false;
          event.getController().setLabel(CSV_BTN_FALSE);
          
        }
        break;

      case "/Screen shot":
        println("Screen shot");
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

    erFileName.render();
    erFileName.showString(strFileInfo + ".csv(or .png)");

    pop();

    update_count++;
}
