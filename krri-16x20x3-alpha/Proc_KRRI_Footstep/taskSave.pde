int SAVE_INTERVAL = 500;

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
  
  boolean cycle_granted = false;

  //  check if satisfies interval
  {
    long cur_tick = timerSaveCSV.getElapsedTick();

    long interval_cycles = cur_tick / SAVE_INTERVAL;
  
    if (timerSaveCSV.getLastCycle() < interval_cycles) { // 180 : per 200ms
      cycle_granted = true;
      timerSaveCSV.updateLastCycle(interval_cycles);
      println("cur grant = " + interval_cycles);
    }
  }
  
  //  store data to csv file
  if(cycle_granted == true) {
    saveCSV_storeData();
    cp5.getController(CSV_BTN_TAG).setLabel(CSV_BTN_TRUE + " (" + timerSaveCSV.getElapsedTick() / 1000 + " sec)");

    //println("tick(200ms) : " + cur_grant_tick + ", " + cur_tick);
  } 

}


Table tableSaveCSV;
String saveCSV_FileName = "";


void saveCSV_open() {
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

  for (int row = 0 ; row < row_max ; row++ ) {
    for (int col = 0 ; col < col_max ; col++ ) {
      colName = "B-" + "X" + col + "-Y" + row;
      tableSaveCSV.addColumn(colName);
    }
  }

  for (int row = 0 ; row < row_max ; row++ ) {
    for (int col = 0 ; col < col_max ; col++ ) {
      colName = "C-" + "X" + col + "-Y" + row;
      tableSaveCSV.addColumn(colName);
    }
  }

  //--------------------------------------
  //  write header into the file.
  String text_field = cp5.get(Textfield.class, "NAME").getText();
  saveCSV_FileName = strPathCSV + "/" + text_field + "/" + getCurTimeString() + "_" + strFileInfo + ".csv";

  saveTable(tableSaveCSV, saveCSV_FileName);
}


void saveCSV_close() {
}

int save_data_count = 0;

void saveCSV_storeData() {
  TableRow newRow = tableSaveCSV.addRow();

  newRow.setInt("id", tableSaveCSV.getRowCount() - 1);
  newRow.setString("timestamp",    getCurTimeString());
  newRow.setInt("runtime(ms)", (int)timerSaveCSV.getElapsedTick());

  String colName; 
  String strIndex;
  for(int row = 0 ; row < matView_A.numOfRow ; row++ ){ 
    for(int x = 0 ; x < matView_A.numOfCol ; x++) {   //  COL = 16
      int xy_pos = row * matView_A.numOfCol + x;

      colName = "A-" + "X" + x + "-Y" + row;
      newRow.setInt(colName, matView_A.cellsValue[xy_pos]);
      colName = "B-" + "X" + x + "-Y" + row;
      newRow.setInt(colName, matView_B.cellsValue[xy_pos]);
      colName = "C-" + "X" + x + "-Y" + row;
      newRow.setInt(colName, matView_C.cellsValue[xy_pos]);
    }
  }

  saveTable(tableSaveCSV, saveCSV_FileName);
  save_data_count++;  
}
