


int BASE_0to1(int num) {
  return (num + 1);
}

void setup_Save() {
  saveDump_open();

}


void update_Save() {
  if ( csv_btn_state == false) 
    return;
  
  int save_interval = 500;
  boolean grant_interval = false;

  //  check if satisfies interval
  {
    long cur_tick = getElapsedTick_Timer();
    long cur_grant_tick = cur_tick / save_interval;
  
    if (getLastGrant_Timer() < cur_grant_tick) { // 180 : per 200ms
      grant_interval = true;
      updateGrant_Timer(cur_grant_tick);
    }
  }
  
  //  store data to csv file
  if(grant_interval == true) {
    saveCSV_storeData();
    cp5.getController(CSV_BTN_TAG).setLabel(CSV_BTN_TRUE + ": (" + getElapsedTick_Timer() / 1000 + " sec)");

    //println("tick(200ms) : " + cur_grant_tick + ", " + cur_tick);
  } 

}


Table table;
String saveCSV_FileName = "CSV_data/";

String saveDump_FileName = "CSV_data/";

void saveDump_open() {
  saveDump_FileName = "CSV_data/";

  String text_field = cp5.get(Textfield.class, "NAME").getText();
  
  String temp_str = strPathCSV + "/" + text_field + "/" + getCurTimeString() + "_" + strFileInfo + ".hex";

  saveDump_FileName = temp_str;

  if(false) {
    int counter = 0;
    for(byte i = 0 ; i < 256 ; i++) {

      System.out.printf("[%03d:0x%02x] ", i,  i); //  intToByte(0xFF)
      if( (i % 16) == 15 ) {
        println(" **");
      }

      counter++;

      if(counter == 256)
        break;
    }
  }

}

void saveDump_storeData(byte [] binData, int binLen) {
  byte [] dump_data;
  dump_data = new byte [binLen];

  System.arraycopy(binData, 0, dump_data, 0, binLen);

  saveBytes(saveDump_FileName, dump_data);

}


void saveCSV_open() {
  saveCSV_FileName = "CSV_data/";

  String text_field = cp5.get(Textfield.class, "NAME").getText();
  
  String temp_str = strPathCSV + "/" + text_field + "/" + getCurTimeString() + "_" + strFileInfo + ".csv";

  table = new Table();
  
  table.addColumn("id");
  table.addColumn("timestamp");
  table.addColumn("runtime(ms)");

  int row_max = NUM_CELLS_ROW;
  int col_max = NUM_CELLS_COL;
  String colName; 

  String strIndex;


  for (int row = 0 ; row < row_max ; row++ ) {
    for (int col = 0 ; col < col_max ; col++ ) {
      colName = "D-" + "C" + col + "-R" + row;
      colName = "D-" + "C" + col + "-R" + row;
      table.addColumn(colName);
    }
  }

  for (int row = 0 ; row < row_max ; row++ ) {
    for (int col = 0 ; col < col_max ; col++ ) {
      // colName = "U-" + "C" + BASE_0to1(col) + "-R" + BASE_0to1(row);
      colName = "U-" + "C" + col + "-R" + row;
      table.addColumn(colName);
    }
  }

  saveCSV_FileName = temp_str;

  saveTable(table, saveCSV_FileName);
}


void saveCSV_close() {
}

int save_data_count = 0;

void saveCSV_storeData() {
  TableRow newRow = table.addRow();
  newRow.setInt("id", table.getRowCount() - 1);
  newRow.setString("timestamp",    getCurTimeString());
  newRow.setInt("runtime(ms)", (int)getElapsedTick_Timer());

  String colName; 
  String strIndex;

  for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){
    for(int col = 0 ; col < NUM_CELLS_COL ; col++) {
      int xy_pos = row * NUM_CELLS_COL + col;
      // colName = "D-" + "C" + BASE_0to1(col) + "-R" + BASE_0to1(row);
      colName = "D-" + "C" + col + "-R" + row;
      newRow.setInt(colName, DataBuf_D[xy_pos]);
    }
  }


  for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){
    for(int col = 0 ; col < NUM_CELLS_COL ; col++) {
      int xy_pos = row * NUM_CELLS_COL + col;
      // colName = "U-" + "C" + BASE_0to1(col) + "-R" + BASE_0to1(row);
      colName = "U-" + "C" + col + "-R" + row;
      newRow.setInt(colName, DataBuf_U[xy_pos]);
    }
  }


  saveTable(table, saveCSV_FileName);
  save_data_count++;
  
}
