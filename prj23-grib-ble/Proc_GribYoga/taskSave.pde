


int BASE_0to1(int num) {
  return (num + 1);
}

void setup_Save() {
  // saveDump_open();

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
    cp5.getController(CSV_BTN_TAG).setLabel(CSV_BTN_TRUE + " (" + getElapsedTick_Timer() / 1000 + " sec)");

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
      colName = "B-" + "X" + col + "-Y" + row;
      table.addColumn(colName);
    }
  }

  for (int row = 0 ; row < row_max ; row++ ) {
    for (int col = 0 ; col < col_max ; col++ ) {
      // colName = "T-" + "C" + BASE_0to1(col) + "-R" + BASE_0to1(row);
      colName = "T-" + "X" + col + "-Y" + row;
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

  for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 0 ~ 10
    for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
      int xy_pos = x * NUM_CELLS_ROW + row;
      // colName = "B-" + "C" + BASE_0to1(col) + "-R" + BASE_0to1(row);
      colName = "B-" + "X" + x + "-Y" + row;
      newRow.setInt(colName, DataBuf_D[xy_pos]);
      // newRow.setInt(colName, xy_pos);
    }
  }


  for(int x = 0 ; x < NUM_CELLS_COL ; x++) {   //  NUM_CELLS_COL = 0 ~ 10
    for(int row = 0 ; row < NUM_CELLS_ROW ; row++ ){ //  NUM_CELLS_ROW = 16
      int xy_pos = x * NUM_CELLS_ROW + row;
      // colName = "T-" + "C" + BASE_0to1(col) + "-R" + BASE_0to1(row);
      colName = "T-" + "X" + x + "-Y" + row;
      newRow.setInt(colName, DataBuf_U[xy_pos]);
      // newRow.setInt(colName, xy_pos);
    }
  }


  saveTable(table, saveCSV_FileName);
  save_data_count++;
  
}
