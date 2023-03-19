

ControlP5 cp5; // https://sojamo.de/libraries/controlP5/

//---------------------------------------------
//  UI coordinate
int window_width = 1400;
int window_height = 900;

int division_r_w = 210;
int division_b_h = 100;

int offset_x = window_width - division_r_w;
int offset_y = 30;
int margin_w = 15;
int margin_h = 35;


EditRect  erCopi_L;
EditRect  erCopi_R;


String strPathCSV = "CSV_Data/";
String strPathShot = "ScreenShot/";

String strFooterCSV = ".csv";
String strFooterShot = ".png";

String strFileInfo;
EditRect  erFileName;


Textlabel    textTimer;

ControlTimer lapTimer;
long         tickOrigin;
long last_grant_tick = 0;

void setup_Timer() {
  lapTimer = new ControlTimer();
  lapTimer.setSpeedOfTime(1); // 1 : 1ms

  tickOrigin = lapTimer.time();
  last_grant_tick = 0;
}

void reset_Timer() {
  tickOrigin = lapTimer.time();
  last_grant_tick = 0;
}

void updateGrant_Timer(long grant_tick) {
  last_grant_tick = grant_tick;
}

long getLastGrant_Timer() {
  return last_grant_tick;
}

long getElapsedTick_Timer() {
  return (lapTimer.time() - tickOrigin);
}

String getCurTimeString() {
  int d = day();    // Values from 1 - 31
  int mo = month();  // Values from 1 - 12
  int y = year();   // 2003, 2004, 2005, etc.

  int s = second();  // Values from 0 - 59
  int m = minute();  // Values from 0 - 59
  int h = hour();    // Values from 0 - 23
  
  String time_string = String.format("%4d%02d%02d-%02d%02d%02d", y, mo, d, h, m, s);
  
  return time_string;  
}


PImage    bgi_logo_120ch; // Background image
PImage    bgi_logo_mdex; // Background image

void setup_UI() {

  bgi_logo_120ch = loadImage("./res/logo_Yoga.png");
  //image(bgi_logo_120ch, window_size_x - 100, 3, 70, 70);
  image(bgi_logo_120ch, window_size_x - 350, -25);
  // image(bgi_logo_120ch, 600, 15);


  bgi_logo_mdex = loadImage("./res/logo_mdex(kor).png");
  //image(bgi_logo_mdex, window_size_x - 100, 3 + 60, 70, 70);
  image(bgi_logo_mdex, window_size_x - 180, 60);

  setup_Timer();
  setup_cp5();
}

void setup_cp5() {
  //----------------------------------------
  //  ControlP5
  //  https://www.sojamo.de/libraries/controlP5/#examples
  cp5 = new ControlP5(this);
  
  PFont font_small = createFont("arial",14);
  PFont font_mid   = createFont("arial",24);
  PFont font_large = createFont("arial",40);
  //  align controls
  int pos_x = 220;
  int pos_y = 500;
  

  pos_y += margin_h;
  textTimer = new Textlabel(cp5,"--",100,100);
  textTimer.setPosition(pos_x, pos_y);

  //----------------------------------------
  //  SERIAL PORT
  {
    // pos_x = window_width / 2 - 180;
    pos_x = 310;
    pos_y = 1;

    cp5.addTextlabel("label_Serial")
                      .setText("Serial Port :")
                      .setPosition(pos_x, pos_y)
                      .setColorValue(0xff555555)
                      .setFont(createFont("Tahoma",14))
                      //.setFont(font_small)
                      ;
    
    // pos_y += 8;
    cp5.addTextfield(SerialNoteLabel)
        .setPosition(pos_x + 90, pos_y)
        .setSize(30,25)
        .setText(userPref.getValue(userPref.itemPort))
        .setFont(createFont("Arial",12))
        //.setFont(font)
        .setFocus(true)
        .setColor(color(0, 0, 100))
        ;

    cp5.addButton("OPEN SERIAL").setPosition(pos_x + 130, pos_y).setSize(55, 25).setLabel("Open").setFont(createFont("Arial",14));

  }
  
  //----------------------------------------
  //  SCREEN SHOT, SAVE CSV
  {
    // pos_x = window_size_x - 650;
    pos_x = 140;
    pos_y = window_size_y - 180;
    cp5.addButton("Screen shot").setPosition(pos_x, pos_y).setSize(200, 70)
                    .setLabel("Screen shot").setFont(font_mid);

    pos_x += 300;
    cp5.addButton(CSV_BTN_TAG).setPosition(pos_x, pos_y).setSize(300, 70)
                    .setLabel(CSV_BTN_FALSE).setFont(font_mid);

    csv_btn_state = false;

  }


  //----------------------------------------
  //  FILE NAME
  {
    pos_x = 20;
    pos_y = window_size_y - 70;

    int control_width = 0;
    
    control_width = 100;
    cp5.addTextlabel("tag_FILENAME")
                      .setText("File Name (Eng) : ")
                      .setPosition(pos_x, pos_y)
                      .setColorValue(0xff0060ff)
                      .setFont(font_small)
                      ;

    pos_x += control_width + 30;
    control_width = 190;
    cp5.addTextfield("NAME")
        .setPosition(pos_x, pos_y).setSize(control_width-50, 25)
        .setText("N")
        .setFont(font_small)
        .setColor(color(0, 0, 100))
        ;
    
    pos_x += control_width - 35;
    control_width = 100;
    cp5.addTextfield("GENDER")
        .setPosition(pos_x, pos_y).setSize(control_width, 25)
        .setText("FEMALE")
        .setFont(font_small)
        .setColor(color(0, 0, 100))
        ;

    pos_x += control_width + 20;
    control_width = 50;
    cp5.addTextfield("AGE")
        .setPosition(pos_x, pos_y).setSize(control_width, 25)
        .setText("1")
        .setFont(font_small)
        .setColor(color(0, 0, 100))
        ;

    pos_x += control_width + 10;
    control_width = 60;
    cp5.addTextfield("WEIGHT (KG)")
        .setPosition(pos_x, pos_y).setSize(control_width, 25)
        .setText("1")
        .setFont(font_small)
        .setColor(color(0, 0, 100))
        ;

    pos_x += control_width + 30;
    control_width = 80;
    cp5.addTextfield("FOOT SIZE (MM)")
        .setPosition(pos_x, pos_y).setSize(control_width, 25)
        .setText("1")
        .setFont(font_small)
        .setColor(color(0, 0, 100))
        ;
    control_width = 230;

    pos_x = 100 + 250;
    pos_y += 70;
    // cp5.addTextlabel("_SaveFileName")
    //                  .setText("TIME_NAME_GENDER_AGE_WEIGHT_FOOTLENGTH.csv")
    //                  .setPosition(pos_x, pos_y)
    //                  .setColorValue(0xff606060)
    //                  .setFont(font_small)
    //                  ;

    control_width = 650;

    erFileName = new EditRect(pos_x, pos_y, control_width, 22);
    erFileName.setFontSize(20);
    erFileName.setTextLeftMargin(2);
    erFileName.setTextTopMargin(16);

  }


}

boolean csv_btn_state = false;
//String SerialNoteLabel = "Serial Port (Number only)";
String SerialNoteLabel = "(Number only)";

String CSV_BTN_TAG    = "CSV_START_STOP";
String CSV_BTN_FALSE  = "SAVE DATA";
String CSV_BTN_TRUE   = "STOP SAVING";



///////////////////////////////////////////////////////////////////////////
//    EVENT HANDLER
///////////////////////////////////////////////////////////////////////////

void controlEvent(CallbackEvent event) {
  String strBuf;
  boolean result;
  int value_i;
  float value_f;

  if (event.getAction() == ControlP5.ACTION_CLICK) {
    switch(event.getController().getAddress()) {
      case "/OPEN SERIAL":
        strBuf = cp5.get(Textfield.class, SerialNoteLabel).getText();
        println("Button open Pressed : " + strBuf);
        
        String comNames = String.format("COM%s", strBuf);
        println("Port Name : " + comNames);
        
        openSerialPort(comNames);

        //sunixStartPort = Integer.parseInt(strBuf);;
        //open_Serial(sunixStartPort);
        
        userPref.setValue(userPref.itemPort, strBuf);
        userPref.saveTable_CSV();
        
        cp5.get(Textfield.class, SerialNoteLabel).setLock(true);
        event.getController().setColorValue(0x5e5e5e);
        //event.getController().setLock(true);

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
          
          reset_Timer();
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

        //String temp_str = "./capture/bebe_V01_" + getCurTimeString() + ".png";
        String temp_str = strPathShot + "/" + text_field + "/" + getCurTimeString() + "_" + strFileInfo + ".png";
        println(temp_str);
        saveFrame(temp_str);
        //cp5.getProperties().saveSnapshot("hello1");
        break;
        
      case "/tag_COPI_L":
        float value = event.getController().getValue();

        println("tag_COPI_L --> " + value);
        break;
    }
  }
}


int update_count = 0;
int COPI_DIV_L = 0;
int COPI_DIV_R = 0;

void update_UI() {
    push();

    //stroke(250);
    ////line(950, 0, 950, window_height);
    //line(dashboard_x, 0, dashboard_x, window_height);

    String text_field;
    
    strFileInfo = "";
    
    text_field = cp5.get(Textfield.class, "NAME").getText();
    strFileInfo += text_field;
    
    text_field = "_" + cp5.get(Textfield.class, "GENDER").getText();
    strFileInfo += text_field;
    
    text_field = "_A" + cp5.get(Textfield.class, "AGE").getText();
    strFileInfo += text_field;
    
    text_field = "_W" + cp5.get(Textfield.class, "WEIGHT (KG)").getText();
    strFileInfo += text_field;
    
    text_field = "_L" + cp5.get(Textfield.class, "FOOT LENGTH (MM)").getText();
    strFileInfo += text_field;
    
    
    erFileName.render();
    erFileName.showString(strFileInfo);


    pop();

    update_count++;
}
