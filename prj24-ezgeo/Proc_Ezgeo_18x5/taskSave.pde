int SAVE_INTERVAL_MS = 25;
LapTimer timerSaveCSV;

int BASE_0to1(int num) {
  return (num + 1);
}

void setup_Save() {
  timerSaveCSV = new LapTimer("csv timer");
}


void update_Save() {
  if ( csv_btn_state == false) 
    return;
  
  //  check if satisfies interval
  if(true == timerSaveCSV.cycleTryCheckOut()) {
    DATA_SAVE_CSV_appendRow();
    cp5.getController(CSV_BTN_TAG).setLabel(CSV_BTN_TRUE + " (" + timerSaveCSV.getElapsed_ms() / 1000 + " sec)");
    println("cur saving time = " + timerSaveCSV.getElapsed_ms());
  } 
}

/////////////////////////////////////////////////////////////
//  
/////////////////////////////////////////////////////////////
Table tableSaveCSV;
String saveCSV_FileName = "";


void DATA_SAVE_CSV_open() {
  //--------------------------------------
  //  Make tableSaveCSV header
  tableSaveCSV = new Table();
  
  tableSaveCSV.addColumn("id");
  tableSaveCSV.addColumn("timestamp");
  tableSaveCSV.addColumn("runtime(ms)");

  int row_max = matView_A.numOfRow;
  int col_max = matView_A.numOfCol;
  String colName; 

  String strIndex;


  for (int row = 0 ; row < row_max ; row++ ) {
    for (int col = 0 ; col < col_max ; col++ ) {
      colName = "A-" + "X" + col + "-Y" + row;
      tableSaveCSV.addColumn(colName);
    }
  }

  //--------------------------------------
  //  write header into the file.
  String text_field = cp5.get(Textfield.class, "NAME").getText();
  saveCSV_FileName = strPathCSV + "/" + text_field + "/" + getCurTimeString() + "_" + strFileInfo + ".csv";

  saveTable(tableSaveCSV, saveCSV_FileName);
}


void DATA_SAVE_CSV_close() {
  saveTable(tableSaveCSV, saveCSV_FileName);
}

int save_data_count = 0;

void DATA_SAVE_CSV_appendRow() {
  TableRow newRow = tableSaveCSV.addRow();

  newRow.setInt("id", tableSaveCSV.getRowCount() - 1);
  newRow.setString("timestamp",    getCurTimeString());
  newRow.setInt("runtime(ms)", (int)timerSaveCSV.getElapsed_ms());

  String colName; 
  String strIndex;
  for(int row = 0 ; row < matView_A.numOfRow ; row++ ){ 
    for(int x = 0 ; x < matView_A.numOfCol ; x++) {   //  COL = 16
      int xy_pos = row * matView_A.numOfCol + x;

      colName = "A-" + "X" + x + "-Y" + row;
      newRow.setInt(colName, matView_A.cellsValue[xy_pos]); // Later, replace colName to viewNick
      // colName = "B-" + "X" + x + "-Y" + row;
      // newRow.setInt(colName, matView_B.cellsValue[xy_pos]);
      // colName = "C-" + "X" + x + "-Y" + row;
      // newRow.setInt(colName, matView_C.cellsValue[xy_pos]);
    }
  }

  save_data_count++;  
}
