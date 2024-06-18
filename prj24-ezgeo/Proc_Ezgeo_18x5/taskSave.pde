int SAVE_INTERVAL_MS = 25;
LapTimer timerSaveCSV;

int BASE_0to1(int num) {
  return (num + 1);
}

void setup_Save() {
  timerSaveCSV = new LapTimer("save csv timer");
}


void update_Save() {
  if ( csv_btn_state == false) 
    return;
  
  //  check if satisfies interval
  if(true == timerSaveCSV.cycleTryCheckOut()) {
    MATRIX_SAVE_CSV_appendRow();
    cp5.getController(CSV_BTN_TAG).setLabel(CSV_BTN_TRUE + " (" + timerSaveCSV.getElapsed_ms() / 1000 + " sec)");
    println("cur saving time = " + timerSaveCSV.getElapsed_ms());
  } 
}

/////////////////////////////////////////////////////////////
//  
/////////////////////////////////////////////////////////////
Table tableSaveCSV;
String saveCSV_FileName = "";

libMatrixView mv_Save;

void MATRIX_SAVE_CSV_open(libMatrixView viewToSave) {
  //--------------------------------------
  //  Make tableSaveCSV header
  tableSaveCSV = new Table();
  
  tableSaveCSV.addColumn("id");
  tableSaveCSV.addColumn("timestamp");
  tableSaveCSV.addColumn("runtime(ms)");

  mv_Save = viewToSave;

  int row_max = mv_Save.numOfRow;
  int col_max = mv_Save.numOfCol;
  String colName; 

  for (int row = 0 ; row < row_max ; row++ ) {
    for (int col = 0 ; col < col_max ; col++ ) {
      colName = String.format("A-X%02d-Y%02d", col, row);
      tableSaveCSV.addColumn(colName);
    }
  }

  //--------------------------------------
  //  write header into the file.
  String text_field = cp5.get(Textfield.class, "NAME").getText();
  saveCSV_FileName = String.format("%s\\%s\\%s_%s.csv", strPathCSV, text_field, getCurTimeString(), strFileInfo);

  saveTable(tableSaveCSV, saveCSV_FileName);
}


void MATRIX_SAVE_CSV_close() {
  saveTable(tableSaveCSV, saveCSV_FileName);
}

int save_data_count = 0;

void MATRIX_SAVE_CSV_appendRow() {
  TableRow newRow = tableSaveCSV.addRow();

  newRow.setInt("id", tableSaveCSV.getRowCount() - 1);
  newRow.setString("timestamp",    getCurTimeString());
  newRow.setInt("runtime(ms)", (int)timerSaveCSV.getElapsed_ms());

  String colName; 

  for(int row = 0 ; row < mv_Save.numOfRow ; row++ ){ 
    for(int x = 0 ; x < mv_Save.numOfCol ; x++) {   //  COL = 16
      int xy_pos = row * mv_Save.numOfCol + x;

      colName = String.format("A-X%02d-Y%02d", x, row);
      newRow.setInt(colName, mv_Save.cellsValue[xy_pos]); // Later, replace colName to viewNick
    }
  }

  save_data_count++;  
}
