//------------------------------------------------
//  Packet protocol
static final int PACKET_HEADER_LEN    = 8;
static final int PACKET_TAIL_LEN      = 2;

static final int HEADER_SYNC   = (0xFF);
static final int TAIL_SYNC     = (0xFE);
static final int VALID_VAL_MAX = (0xEF);

int PACKET_LEN_TYPE0    = 0;
static final int SINGLE_DATA_LEN     = 1; // 1: byte, 2: short, 4: int

//------------------------------------------------
//  Packet buffer for receiving
byte[] PacketRawData;
byte[] PacketData;

int packetCells = 0;
int packetRows = 0;
int packetColumns = 0;


//==========================================================
//  SERIAL AND PACKET BUFFER STRUCTURE
String  PORT_NAME = "COM17"; // "Silicon Labs CP210x USB to UART Bridge"

Serial myPort;
Serial myPort2;
boolean isPortOpened = false;
boolean isPortOpened2 = false;


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

byte intToByte(int num32) {
  byte num8 = 0;
  if (127 < num32) {
    num8 = (byte)(num32 - 256);
  }
  else 
    num8 = (byte)num32;
  
  return num8;
}


//------------------------------------------------
//  Configuration
void configPacket(int num_col, int num_row) {
  packetRows = num_row;
  packetColumns = num_col;
  
  int packetCells = num_row * num_col;

  PACKET_LEN_TYPE0     = PACKET_HEADER_LEN + PACKET_TAIL_LEN + packetCells;

  PacketRawData = new byte [packetCells * SINGLE_DATA_LEN * 6 + 400 ]; // 400 : in case of overflow
  PacketData = new byte [PACKET_LEN_TYPE0];
}



//-------------------------------------------------------------
//  PARSE PACKET
//-------------------------------------------------------------
int parseData_EZGEO(byte[] read_data) {
  int parse_error = 0;

  if (false == (  (byteToInt(read_data[0]) == HEADER_SYNC) && (byteToInt(read_data[1]) == HEADER_SYNC) )) {
    parse_error = -1;
    return parse_error;
  }

  int major_ver   = byteToInt(read_data[2]);
  int minor_ver   = byteToInt(read_data[3]);

  int packet_len  = byteToInt(read_data[4]);
  int board_id    = byteToInt(read_data[5]);

  int reserved_0  = byteToInt(read_data[6]);
  int reserved_1  = byteToInt(read_data[7]);

  // println("board = " + board_id + ", major:" + major_ver + ", len:" + packet_len + ", start:" + byteToInt(read_data[0]) + ", res1:" + reserved_1);

  parseData_EzgeoReordering(read_data, board_id);
  
  return 0;

}


int parseData_EzgeoReordering(byte[] read_data, int board_index) {

  for(int row = 0 ; row < packetRows ; row++ ){ //  ROW = 20
    for(int x = 0 ; x < packetColumns ; x++) {   //  COL = 5
      int xy_pos = packetColumns * row + x;

      if( (row %2) == 1 ) {
        int offset = packetColumns * row + (packetColumns - 1 - x);
        matView_A.cellsValue[offset] = byteToInt(read_data[PACKET_HEADER_LEN + xy_pos]);

      }
      else {
        matView_A.cellsValue[xy_pos] = byteToInt(read_data[PACKET_HEADER_LEN + xy_pos]);
      }

    }
  }

  // matView_A.fillPacketBytes(read_data, PACKET_HEADER_LEN);

  return 0;
}


///////////////////////////////////////////////////////////////////////////////////
//    SERIAL PORT THREAD (OPEN, READ)
///////////////////////////////////////////////////////////////////////////////////

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
  println(portName + " port is opened...");
}


boolean openSerialOrExit() {
  if (Serial.list().length == 0) {
    drawTextLog("ERROR ! Can't find serial port. Connect arduino and restart.");

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

  drawTextLog(portName + " port is opened...");

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

