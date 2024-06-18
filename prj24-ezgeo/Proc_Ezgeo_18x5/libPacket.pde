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

libMatrixView linkedMV;


//------------------------------------------------
//  Configuration
void configPacket(int num_col, int num_row) {
  int num_cells = num_row * num_col;

  PACKET_LEN_TYPE0     = PACKET_HEADER_LEN + PACKET_TAIL_LEN + num_cells;

  PacketRawData = new byte [num_cells * SINGLE_DATA_LEN * 6 + 400 ]; // 400 : in case of overflow
  PacketData = new byte [PACKET_LEN_TYPE0];
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

byte intToByte(int num32) {
  byte num8 = 0;
  if (127 < num32) {
    num8 = (byte)(num32 - 256);
  }
  else 
    num8 = (byte)num32;
  
  return num8;
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

  parseData_BottomLeftStart(read_data, board_id);
  
  return 0;

}


int parseData_BottomLeftStart(byte[] read_data, int board_index) {
  int offset = 0; 

  for(int row = 0 ; row < matView_A.numOfRow ; row++ ){ //  ROW = 20
    for(int x = 0 ; x < matView_A.numOfCol ; x++) {   //  COL = 16

      int xy_pos = matView_A.numOfCol * row + x;

      matView_A.cellsValue[xy_pos] = byteToInt(read_data[PACKET_HEADER_LEN + xy_pos]);

    }
  }

  return 0;
}
