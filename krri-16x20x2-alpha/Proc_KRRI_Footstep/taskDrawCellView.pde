//==========================================================
//  CONFIG DATA VIEW
String csvPath_CellPos_A = "./res/CellPos_Block_A.csv";
String csvPath_CellPos_B = "./res/CellPos_Block_B.csv";
String csvPath_CellPos_C = "./res/CellPos_Block_C.csv";

int [] DataBuf_A = new int [NUM_CELLS_COL * NUM_CELLS_ROW];
int [] DataBuf_B = new int [NUM_CELLS_COL * NUM_CELLS_ROW];  
int [] DataBuf_C = new int [NUM_CELLS_COL * NUM_CELLS_ROW];  

libMatrixView  MatDataView_A;
libMatrixView  MatDataView_B;
libMatrixView  MatDataView_C;


void setup_EditView() {
  //  load csv file - rect cell position. <left, top, width, height> of 31 rect
  Table     csvRectPos_A;
  Table     csvRectPos_B;
  Table     csvRectPos_C;

  
  //---------------------------------
  //  Cell position - A
  csvRectPos_A = loadTable(csvPath_CellPos_A, "header");
  MatDataView_A = new libMatrixView(NUM_CELLS_COL, NUM_CELLS_ROW, "mat-up");

  if(false == MatDataView_A.setCellPos_CSV(csvRectPos_A)) {
    println("Failed to load CSV file. " + csvPath_CellPos_A);
    return;
  }
  else {
    println("Succeeded to load CSV file. " + csvPath_CellPos_A);
  }
  MatDataView_A.createCOMGauge1();
  MatDataView_A.setEditTextSize(11);

  for (int i = 0 ; i < (NUM_CELLS_COL * NUM_CELLS_ROW) ; i++) {
    DataBuf_A[i] = i;
  }  

  //---------------------------------
  //  Cell position - B
  csvRectPos_B = loadTable(csvPath_CellPos_B, "header");
  MatDataView_B = new libMatrixView(NUM_CELLS_COL, NUM_CELLS_ROW, "mat-down"); 

  if(false == MatDataView_B.setCellPos_CSV(csvRectPos_B)) {
    println("Failed to load CSV file. " + csvPath_CellPos_B);
    return;
  }
  else {
    println("Succeeded to load CSV file. " + csvPath_CellPos_B);
  }
  MatDataView_B.createCOMGauge1();
  MatDataView_B.setEditTextSize(11);

  for (int i = 0 ; i < (NUM_CELLS_COL * NUM_CELLS_ROW) ; i++) {
    DataBuf_B[i] = i;
  }  

  //---------------------------------
  //  Cell position - C
  csvRectPos_C = loadTable(csvPath_CellPos_C, "header");
  MatDataView_C = new libMatrixView(NUM_CELLS_COL, NUM_CELLS_ROW, "mat-down"); 

  if(false == MatDataView_C.setCellPos_CSV(csvRectPos_C)) {
    println("Failed to load CSV file. " + csvPath_CellPos_C);
    return;
  }
  else {
    println("Succeeded to load CSV file. " + csvPath_CellPos_C);
  }
  MatDataView_C.createCOMGauge1();
  MatDataView_C.setEditTextSize(11);

  for (int i = 0 ; i < (NUM_CELLS_COL * NUM_CELLS_ROW) ; i++) {
    DataBuf_C[i] = i;
  }  

}


boolean update_EditView() {
  // if (false == isPortOpened)
  //  return false;

  MatDataView_A.anylizeCellValues(DataBuf_A);
  MatDataView_B.anylizeCellValues(DataBuf_B);
  MatDataView_C.anylizeCellValues(DataBuf_C);
  
  MatDataView_A.drawColorCells();
  MatDataView_B.drawColorCells();
  MatDataView_C.drawColorCells();


  // MatDataView_A.drawCOMGauge();

  //-----------------------------------------------
  //  step 3. draw the x and y gauge of "center of mass"
  PVector   vectorCOM_A = MatDataView_A.getVectorCOM();

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

  int major_ver   = byteToInt(read_data[2]);
  int minor_ver   = byteToInt(read_data[3]);

  int packet_len  = byteToInt(read_data[4]);
  int board_id    = byteToInt(read_data[5]);

  int reserved_0  = byteToInt(read_data[6]);
  int reserved_1  = byteToInt(read_data[7]);

  // println("board = " + board_id + ", major:" + major_ver + ", len:" + packet_len + ", start:" + byteToInt(read_data[0]) + ", res1:" + reserved_1);

  parseData_BottomLeftStart(read_data, board_id);
  // if(board_id == 0) {
  //   parseData_TopRightStart(read_data);
  //   VB_Filled_Board0 = true;
  //   rx_count_board_0++;
  // }
  // else if(board_id == 1) {
  //   parseData_BottomLeftStart(read_data);
  //   rx_count_board_1++;
  // }

  
  return 0;

}

int [] src_buf_u = new int [NUM_CELLS_COL * NUM_CELLS_ROW];

void compressTest_U() {
    String strMsg = " ";

  // System.arraycopy(PacketRawData, 0, PacketData, 0, PACKET_LEN_TYPE0);
  //  copy data to begin buffer
  for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 3~14
    for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
      int xy_pos = x * NUM_CELLS_ROW + row;

      src_buf_u[xy_pos] = DataBuf_A[xy_pos];
      DataBuf_A[xy_pos] = 0;
    }
  }

  IntList zeroRows; // https://processing.org/reference/IntList.html
  zeroRows = new IntList();

  IntList noneRows; // https://processing.org/reference/IntList.html
  noneRows = new IntList();

  byte [] row_data = new byte [NUM_CELLS_COL * (NUM_CELLS_ROW + 1)];
  int comp_row_index = 0;

  //  COMPRESS : check run length
  for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
    int sum_row = 0;

    row_data[comp_row_index * (NUM_CELLS_ROW + 1)] = intToByte(row);

    for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 3~14
      int xy_pos = x * NUM_CELLS_ROW + row;

      row_data[comp_row_index * (NUM_CELLS_COL + 1) + x + 1] = intToByte(src_buf_u[xy_pos]);

      sum_row += src_buf_u[xy_pos];
    }

    if(sum_row < 30) {
      strMsg = String.format("Zero - (row: %d/%d)", row, NUM_CELLS_ROW );
      // println(strMsg);
      zeroRows.append(row);
    }
    else {
      strMsg = String.format("Data - (row: %d/%d)", row, NUM_CELLS_ROW );
      // println(strMsg);
      noneRows.append(row);

      comp_row_index++;
    }

    //  verify none zero rows
    // println("\nnew line");
    for(int i = 0 ; i < noneRows.size() ; i++) {
      // println(noneRows.get(i));
    }
  }

  //  DECOMPRESS : check run length
  int decomp_row_index = 0;
  for(int i = 0 ; i < noneRows.size() ; i++) {
    int this_row_index = row_data[i * (NUM_CELLS_ROW + 1)];
    println(this_row_index);

    for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 3~14
      int xy_pos = x * NUM_CELLS_ROW + this_row_index;

      // DataBuf_A[xy_pos] = row_data[decomp_row_index * (NUM_CELLS_ROW + 1) + x];
    }
  }


  //  copy end buffer to data buffer
  for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 3~14
    for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
      int xy_pos = x * NUM_CELLS_ROW + row;

      DataBuf_A[xy_pos] = src_buf_u[xy_pos];
    }
  }
}



int parseData_BottomLeftStart(byte[] read_data, int board_index) {
  int offset = 0; 

  for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 20
    for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 16

      int xy_pos = NUM_CELLS_COL * row + x;
      // offset = (NUM_CELLS_COL - x - 1) * NUM_CELLS_ROW + (NUM_CELLS_ROW - row - 1);

      if(read_data[xy_pos] < 5)
        read_data[xy_pos] = 0;

      if(board_index == 0) {
        DataBuf_A[xy_pos] = byteToInt(read_data[HEADER_LEN + xy_pos]);
        VB_Filled_Board0 = true;
      }
      else if(board_index == 1) {
        DataBuf_B[xy_pos] = byteToInt(read_data[HEADER_LEN + xy_pos]);
      }

    }
  }

  return 0;
}


int parseData_TopRightStart(byte[] read_data) {
  int offset = 0; 

  for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 10
    for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16

      int xy_pos = x * NUM_CELLS_ROW + row;
      offset = (NUM_CELLS_COL - x - 1) * NUM_CELLS_ROW + (NUM_CELLS_ROW - row - 1);

      DataBuf_A[offset] = byteToInt(read_data[HEADER_LEN + xy_pos]);

      if(DataBuf_A[offset] < 5)
        DataBuf_A[offset] = 0;
    }
  }

  return 0;
}



int parseData_BottomLeftStart(byte[] read_data) {
  int offset = 0; 

  for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 10
    for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
      int xy_pos = x * NUM_CELLS_ROW + row;

      DataBuf_B[xy_pos] = byteToInt(read_data[HEADER_LEN + xy_pos]);

      if(DataBuf_B[xy_pos] < 5)
        DataBuf_B[xy_pos] = 0;
    }
  }
  
  int compen_sum = (DataBuf_B[119] + DataBuf_B[121] + DataBuf_B[104] + DataBuf_B[136] ) / 4;
  DataBuf_B[120] = compen_sum;

  return 0;

}
