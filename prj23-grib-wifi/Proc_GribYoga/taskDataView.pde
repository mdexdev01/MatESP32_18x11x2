//==========================================================
//  CONFIG DATA VIEW

DoubleArrayView foot_DataView;


int [] foot_Data_L = new int [NUM_CELLS_COL * NUM_CELLS_ROW];
int [] foot_Data_R = new int [NUM_CELLS_COL * NUM_CELLS_ROW];  
int [] foot_Data_All = new int [NUM_CELLS_COL * NUM_CELLS_ROW * NUM_BLOCK];  


String csvPath_CellPos_A = "./res/CellPos_Block_A.csv";
String csvPath_CellPos_B = "./res/CellPos_Block_B.csv";

int [] DataBuf_U = new int [NUM_CELLS_COL * NUM_CELLS_ROW];
int [] DataBuf_D = new int [NUM_CELLS_COL * NUM_CELLS_ROW];  

SensorCellView  MatDataView_U;
SensorCellView  MatDataView_D;


void setup_EditView() {
  //  1. DoubleArrayView - rect position and color  
  foot_DataView = new DoubleArrayView(NUM_CELLS_COL, NUM_CELLS_ROW);

  //  load csv file - rect cell position. <left, top, width, height> of 31 rect
  Table     csvRectPos_U;
  Table     csvRectPos_D;

  
  //  Cell position - Up
  MatDataView_U = new SensorCellView(NUM_CELLS_COL, NUM_CELLS_ROW, "mat-up");
  csvRectPos_U = loadTable(csvPath_CellPos_A, "header");

  if(false == MatDataView_U.setCellPos_CSV(csvRectPos_U)) {
    println("Failed to load CSV file. " + csvPath_CellPos_A);
    return;
  }
  else {
    println("Succeeded to load CSV file. " + csvPath_CellPos_A);
  }
  MatDataView_U.createCOMGauge1();
  MatDataView_U.setEditTextSize(12);

  //  Cell position - Down
  MatDataView_D = new SensorCellView(NUM_CELLS_COL, NUM_CELLS_ROW, "mat-down"); 
  csvRectPos_D = loadTable(csvPath_CellPos_B, "header");

  if(false == MatDataView_D.setCellPos_CSV(csvRectPos_D)) {
    println("Failed to load CSV file. " + csvPath_CellPos_B);
    return;
  }
  else {
    println("Succeeded to load CSV file. " + csvPath_CellPos_B);
  }
  MatDataView_D.createCOMGauge1();
  MatDataView_D.setEditTextSize(12);

  //---------------------------------
  for (int i = 0 ; i < (NUM_CELLS_COL * NUM_CELLS_ROW) ; i++) {
    DataBuf_U[i] = i;
  }  

  for (int i = 0 ; i < (NUM_CELLS_COL * NUM_CELLS_ROW) ; i++) {
    DataBuf_D[i] = i;
  }  

}


boolean update_EditView() {
  // if (false == isPortOpened)
  //  return false;

  MatDataView_D.anylizeCellValues(DataBuf_D);
  MatDataView_U.anylizeCellValues(DataBuf_U);
  
  MatDataView_D.drawColorCells();
  MatDataView_U.drawColorCells();


  // MatDataView_U.drawCOMGauge();
  // MatDataView_D.drawCOMGauge();

  //-----------------------------------------------
  //  step 3. draw the x and y gauge of "center of mass"
  PVector   vectorCOM_U = MatDataView_U.getVectorCOM();
  PVector   vectorCOM_D = MatDataView_D.getVectorCOM();

  //strBuffer = String.format("Center Of Mass (X, Y) = LEFT (%2.3f, %2.3f), Right (%2.3f, %2.3f)"
  //                      , vectorCOM_A.x, vectorCOM_A.y, vectorCOM_B.x, vectorCOM_B.y);
  //drawTextLog(strBuffer);
  
  return true;

}

//-------------------------------------------------------------
//  PARSE PACKET
//-------------------------------------------------------------
static final int HEADER_LEN    = 8;
static final int TAIL_LEN      = 2;

static final int HEADER_SYNC   = (0xFF);
static final int TAIL_SYNC     = (0xFE);
static final int VALID_VAL_MAX = (0xEF);

boolean VB_Filled_Board0 = false;
int rx_count_board_0 = 0;
int rx_count_board_1 = 0;
int rx_count_board_1_0 = 0;
int rx_count_board_1_6 = 0;
int rx_count_board_1_12 = 0;

int parseData_GRIB(byte[] read_data) {
  int parse_error = 0;

  if (false == (  (byteToInt(read_data[0]) == HEADER_SYNC) && (byteToInt(read_data[1]) == HEADER_SYNC) )) {
    parse_error = -1;
    return parse_error;
  }

  int major_ver   = read_data[2];
  int minor_ver   = read_data[3];

  int packet_len  = read_data[4];
  int board_id    = read_data[5];

  int reserved_0  = read_data[6];
  int reserved_1  = read_data[7];

  // println("board = " + board_id + ", major:" + major_ver + ", len:" + packet_len + ", start:" + reserved_0 + ", end:" + reserved_1);

  if(board_id == 0) {
    parseData_TopLeftStart(read_data, major_ver, minor_ver, reserved_0, reserved_1);
    VB_Filled_Board0 = true;
    rx_count_board_0++;
  }
  else if(board_id == 1) {
    parseData_BottomLeftStart(read_data, major_ver, minor_ver, reserved_0, reserved_1);
    rx_count_board_1++;
    switch (reserved_0) {
      case 0:
      rx_count_board_1_0++;
      break;
      case 6:
      rx_count_board_1_6++;
      break;
      case 12:
      rx_count_board_1_12++;
      break;
    }
  }

  // println("rx bd[0]:" + rx_count_board_0 + " rx bd[1]:" + rx_count_board_1 + " [" + rx_count_board_1_0 + " : " + rx_count_board_1_6 + " : " + rx_count_board_1_12);
  
  return -1;

}


int parseData_TopLeftStart(byte[] read_data, int major_ver, int minor_ver, int start_x, int end_x) {
  int offset = 0; 

  for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 3~14
    for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16

      int xy_pos = x * NUM_CELLS_ROW + row;
      int inv_pos = x * NUM_CELLS_ROW + revIndex16(row);
      DataBuf_U[inv_pos] = byteToInt(read_data[HEADER_LEN + xy_pos]);

      if(DataBuf_U[inv_pos] < 5)
        DataBuf_U[inv_pos] = 0;
    }
  }

  return -1;
}


int parseData_BottomLeftStart(byte[] read_data, int major_ver, int minor_ver, int start_x, int end_x) {
  int offset = 0; 

  if(major_ver == 0) {
    for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 3~14
      for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
        int xy_pos = x * NUM_CELLS_ROW + row;
        int inv_pos = x * NUM_CELLS_ROW + (NUM_CELLS_ROW - 1 ) - row;

        DataBuf_D[xy_pos] = byteToInt(read_data[HEADER_LEN + xy_pos]);

        if(DataBuf_D[xy_pos] < 5)
          DataBuf_D[xy_pos] = 0;
      }
    }
  }
  else if(major_ver == 1) {
    offset = 0;
    int xy_pos = 0;

    for(int x = start_x ; x < end_x ; x++) {   //  NUM_CELLS_COL = 3~14
      for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
        xy_pos = x * NUM_CELLS_ROW + row;
        
        DataBuf_D[xy_pos] = byteToInt(read_data[HEADER_LEN + offset]);

        if(DataBuf_D[xy_pos] < 5)
          DataBuf_D[xy_pos] = 0;
        offset++;
      }
    }
  }

  return -1;

}


int parseData_BottomLeftStart2(byte[] read_data, int major_ver, int minor_ver, int res_0, int res_1) {
  int offset = 0; 

  if(major_ver == 0) {
    for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 3~14
      for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16

        int xy_pos = x * NUM_CELLS_ROW + row;
        int inv_pos = x * NUM_CELLS_ROW + (NUM_CELLS_ROW - 1 ) - row;
        // println("indexing=" + xy_pos + ", " + inv_pos);

        DataBuf_D[inv_pos] = byteToInt(read_data[HEADER_LEN + xy_pos]);
      }
    }
  }
  else if(major_ver == 1) {
    int start_x = res_0;
    int end_x = res_1;
    // if(start_x != 0)
    //   return -1;

    offset = 0;

    int xy_pos = 0;
    int inv_pos = 0;
    for(int x = start_x ; x < end_x ; x++) {   //  NUM_CELLS_COL = 3~14
      for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
        // xy_pos = x * NUM_CELLS_ROW + row;
        inv_pos = x * NUM_CELLS_ROW + (NUM_CELLS_ROW - 1 ) - row;
        
        DataBuf_D[inv_pos] = byteToInt(read_data[HEADER_LEN + offset]);
        offset++;
      }
    }
    // println("start_x:" + start_x + ", end_x:" + end_x + ", offset:" + offset + ", inv_pos:" + (inv_pos + offset));
  }

  return -1;

}

