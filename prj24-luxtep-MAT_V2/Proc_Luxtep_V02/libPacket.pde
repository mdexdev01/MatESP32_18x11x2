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
int parse_count = 0;
int board_count[] = new int [3];

int parseData_Luxtep(byte[] read_data) {
  int parse_error = 0;

  if (false == (  (byteToInt(read_data[0]) == HEADER_SYNC) && (byteToInt(read_data[1]) == HEADER_SYNC) )) {
    println("bad packet");
    parse_error = -1;
    return parse_error;
  }

  int version   = byteToInt(read_data[IDX_VERSION]);
  int board_id    = byteToInt(read_data[IDX_TX_BOARD_ID]);
  
  int group_id    = byteToInt(read_data[IDX_GROUP_ID]);
  int msg_id      = byteToInt(read_data[IDX_MSG_ID]);

  int num_row     = byteToInt(read_data[IDX_HEIGHT]);
  int num_col     = byteToInt(read_data[IDX_WIDTH]);


  int packet_len = num_row * num_col;

  switch(board_id) {
  case 0:
    board_count[0]++;
    break;
  case 1:
    board_count[1]++;
    break;
  case 2:
    board_count[2]++;
    break;
  case 8:
    board_count[0]++;
    board_count[1]++;
    break;
  //default :
  //  return 0;
  }

  //matView_A[board_id].fillPacketBytes(read_data, PACKET_HEADER_LEN);
  matView_A[0].fillPacketBytes(read_data, PACKET_HEADER_LEN);

   println("[" + parse_count + "]board = " + board_id + ", version:" + version + ", len:" + packet_len + ", start:" + byteToInt(read_data[0]) + ", group_id:" + group_id);

  if( (board_count[0] % 100) == 0)
    println("COUNT : board[0]:" + board_count[0] + " board[1]:" + board_count[1] + "(" + (board_count[1]-board_count[0]) + ")" + " board[2]:" + board_count[2] + "(" + (board_count[2]-board_count[0])+ ")");

  parse_count++;

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

//  NOT USED
void openSerialPort2(String portName) {
  try {
    myPort2 = new Serial(this, portName, 230400); // 115200 : should be same to the setting in Arduino code.
    // myPort2 = new Serial(this, portName, 1000000); // 115200 : should be same to the setting in Arduino code.
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

  // myPort = new Serial(this, portName, 230400); // 115200 : should be same to the setting in Arduino code.
  myPort = new Serial(this, portName, 921600); // 115200 : should be same to the setting in Arduino code.

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

    if ( available_len < PACKET_LEN_TYPE0) { //If receiving buffer is filled under 2 bytes.
      if (available_len == 0) {
        countNull++;
        delay(1);
      }

      if ((countNull % 10000) == 1 ) {
        //drawTextLog("Serial seems not connected");
        //println("Serial seems not connected");
      }

      delay(1);
      continue;
    }


    //  ==> https://processing.org/reference/libraries/serial/Serial_readStringUntil_.html
    read_len = myPort.readBytesUntil(TAIL_SYNC, PacketRawData); // read 1 whole buffer.

    if(read_len < (PACKET_LEN_TYPE0 - 1)) {
      continue;
      
      
    }
    else if(read_len == PACKET_LEN_TYPE0) {
      System.arraycopy(PacketRawData, 0, PacketData, 0, PACKET_LEN_TYPE0);
       println("packet 0 good");
    }
    else {
      if(PACKET_LEN_TYPE0 < read_len) {
        int jitter = read_len - PACKET_LEN_TYPE0; 
        println("read_len = " + read_len + " of " + available_len 
                + ", Jitter:" + jitter
                + ", PACK:" + PACKET_LEN_TYPE0
                + "\t [0]" + PacketRawData[jitter] + " [1]" + PacketRawData[jitter+1] 
                + " [last-2]" + PacketRawData[PACKET_LEN_TYPE0 + jitter -2] 
                + " [last-1]" + PacketRawData[PACKET_LEN_TYPE0 + jitter -1]);
        
        if( (PacketRawData[jitter] == -1) && (PacketRawData[jitter + 1] == -1) && (PacketRawData[read_len-1] == -2) ) {
          //println("save");
          System.arraycopy(PacketRawData, jitter, PacketData, 0, PACKET_LEN_TYPE0);
        }
        else {
          continue;
        }
        //println(PacketData);
      }
      //delay(1);
      //continue;
    }

    // parseData_EZGEO(PacketData);
    //parseData_BrandContents(PacketData);
    parseData_Luxtep(PacketData);

    countNull = 0;

    flag_read = true;
  } // while

}
