//////////////////////////////////////////////////////////////////////
//  CLASS  SensorCellView
//////////////////////////////////////////////////////////////////////
int invIndex16(int index) {
  return (NUM_CELLS_ROW - index - 1);
}

class SensorCellView {
  String viewNick = "none";

  int numOfRow = 6;
  int numOfCol = 10;
  int numOfCells;

  int [] cellsValue;
  EditRect [] cellsRect;
  

  EditRect cellsOutline;

  PVector COM_Rate;
  PVector COM_Coord;  
  EditRect gaugeRect_X;
  EditRect gaugeRect_Y;

  
  int OCCUPY_THRESHOLD = 50; 
  int VALID_VALUE = 5;
  

  SensorCellView(int num_col, int num_row, String nick) {
    numOfRow = num_row;
    numOfCol = num_col;
    numOfCells = numOfRow * numOfCol;

    allocCellBuffer(numOfRow * numOfCol);

    setNick(nick);

    COM_Rate = new PVector(0.0f, 0.0f);
    COM_Coord = new PVector(0.0f, 0.0f);
  };
  
  void setNick(String nick) {
    viewNick = nick;
  }
  
  void allocCellBuffer(int cell_num) {
    numOfCells = cell_num;

    cellsValue = new int [cell_num];
    cellsRect = new EditRect [cell_num]; 
  }

  PVector getVectorCOM() {
    return COM_Rate;
  }


  boolean setCellPos_CSV(Table table) {
    println(table.getRowCount() + " total rows in rect_pos.csv file");
    
    //-----------------
    int num_col = 0;
    int num_row = 0;
    int x_start = 0;
    int x_pitch = 0;
    int x_width = 0;
    int y_start = 0;
    int y_pitch = 0;
    int y_height = 0;

    int i = 0;
    try {
      for (TableRow row : table.rows()) {
        //  		X-start	X-pitch	X-width	Y-start	Y-pitch	Y-height

        num_col   = row.getInt("NumCol");
        num_row   = row.getInt("NumRow");
        x_start   = row.getInt("X_start");
        x_pitch   = row.getInt("X_pitch");
        x_width   = row.getInt("X_width");
        y_start   = row.getInt("Y_start");
        y_pitch   = row.getInt("Y_pitch");
        y_height  = row.getInt("Y_height");

        println(num_col + ", " + num_row + ", " + x_start + ", " + x_pitch + ", " + x_width + ", " + y_start + ", " + y_pitch + ", " + y_height);
        i++;

        if(i == 1)
          break;
      }
    } catch (Exception e) {
        println(e.toString());
        return false;
    }
    
    println("read csv out");

    NUM_CELLS_ROW = num_row;
    NUM_CELLS_COL = num_col;

    //-----------------
    //  load csv file 
    String cell_name;
    int [] rect_left  = new int [numOfCells];
    int [] rect_width = new int [numOfCells];
    int [] rect_top   = new int [numOfCells];
    int [] rect_height = new int [numOfCells];

    for(int x = 0 ; x < numOfCol ; x++) {
      int x_offset = x_start + x * x_pitch;

      for(int y = 0 ; y < numOfRow ; y++) {
        int y_offset = y_start + y * y_pitch;
        // int index = x * numOfRow + y;
        int index = x * numOfRow + invIndex16(y);

        rect_left[index]    = x_offset;
        rect_width[index]   = x_width;
        rect_top[index]     = y_offset;
        rect_height[index]  = y_height;

        // println("cell rect)" + rect_left[index] + ", " + rect_top[index] + ", " + rect_width[index] + ", " + rect_height[index]);
      }
    }


    //  create Rect UI
    i = 0;
    for(int cell_index = 0 ; cell_index < numOfCells ; cell_index++){
      cellsRect[cell_index] = 
        new EditRect(rect_left[cell_index], rect_top[cell_index], rect_width[cell_index], rect_height[cell_index]);

        // println("cell rect :" + rect_left[cell_index] + ", " + rect_top[cell_index] + ", " + rect_width[cell_index] + ", " + rect_height[cell_index]);
    }
    
    
    learnGeometry();
    
    createCOMGauge1();
    
    println(viewNick + ": cell position configured.");
    
    return true;
  }
  
  
  void learnGeometry(){
    int left_end, right_end, top_end, bottom_end;
    int  rect_L, rect_R, rect_T, rect_B;

    left_end = right_end = top_end = bottom_end = 0;
    
    left_end  = right_end   = cellsRect[0].getCenterX();
    top_end   = bottom_end   = cellsRect[0].getCenterY();

    // find outline
    for(int i = 0 ; i < numOfCells ; i++){
       rect_L = cellsRect[i].getLeft();
       rect_R = cellsRect[i].getRight();
       rect_T = cellsRect[i].getTop();
       rect_B = cellsRect[i].getBottom();
      
      if( rect_L < left_end) {
        left_end =  rect_L;
      }
      if(right_end <  rect_R) {
        right_end =  rect_R;
      }
      
      if( rect_T < top_end) {
        top_end =  rect_T;
      }
      if(bottom_end <  rect_B) {
        bottom_end =  rect_B;
      }
    }
    
    cellsOutline = new EditRect(left_end, top_end, right_end - left_end, bottom_end - top_end);
    println(viewNick + ": outline (l,r,t,b) : " + left_end + ", " + right_end + ", " + top_end + ", " + bottom_end);
  }


  int gaugeX_Top = 0;
  int gaugeX_Height = 20;
  
  int gaugeY_Left = 0;
  int gaugeY_Width = 47;
  
  void createCOMGauge1() {
    gaugeX_Top = cellsOutline.getTop()-gaugeX_Height * 2 + 10;
    gaugeRect_X = new EditRect(cellsOutline.getLeft(), gaugeX_Top, cellsOutline.getWidth(), gaugeX_Height ); // left, top, width, height
    gaugeRect_X.setTextLeftMargin(cellsOutline.getWidth() / 2);

    int gaugeY_width = 40;
    gaugeY_Left = cellsOutline.getLeft() - gaugeY_width * 2 + 15;
    gaugeRect_Y = new EditRect(gaugeY_Left, cellsOutline.getTop(), gaugeY_Width, cellsOutline.getHeight() ); // left, top, width, height
    gaugeRect_Y.shiftTextDown(cellsOutline.getHeight() / 3);

  }
  /*
  // color map of HSB
  int colorMap(int cell_value) {
    int hue = 180 - (cell_value * 180 / 220 ); // make 0 ~ 220(cell) to 180 ~ 0(hue)

    if(240 < cell_value)
      hue = 0;

    return hue;
  }
  */

  int textSize = 12;
  
  void setEditTextSize(int t_size) {
    textSize = t_size;
    println("SensorCellView::setEditTextSize");
    
    int i = 0;
    for(int cell_index = 0 ; cell_index < numOfCells ; cell_index++){
      cellsRect[cell_index].setFontSize(t_size);
    }

  }
    
  void drawColorCells(){  
    int i = 0;
    String strMsg = " ";
    
    textSize(textSize);

    for ( i = 0 ; i < numOfCells ; i++ ) {
      strMsg = String.format("%d", cellsValue[i]);
      // println(i + ")" + strMsg);
      cellsRect[i].render();  
      cellsRect[i].showString(strMsg);
         

      //  HSB : https://codepen.io/HunorMarton/details/eWvewo
      if(cellsValue[i] == 0) {
        fill(colorMap(cellsValue[i]), 0, 80); // HUE, SATURATE, BRIGHTNESS : black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
      }
      else {
        fill(colorMap(cellsValue[i]), 90, 90); // HUE, SATURATE, BRIGHTNESS : black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
      }
      cellsRect[i].render();  
      cellsRect[i].showInt(cellsValue[i]);
      
    }
  }


  void drawCOMGauge() {
    push();

    //  draw X gauge
    fill(0, 0, 90); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    gaugeRect_X.render();
    gaugeRect_X.showFloat(COM_Rate.x);

    fill(0, 0, 0); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    line(COM_Coord.x, gaugeX_Top, COM_Coord.x, gaugeX_Top + gaugeX_Height);

    //  draw Y gauge
    fill(0, 0, 90); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    gaugeRect_Y.render();
    gaugeRect_Y.showFloat(COM_Rate.y);
    
    //  draw niddle
    stroke(0, 50, 50);
    line(gaugeY_Left, COM_Coord.y, gaugeY_Left + gaugeY_Width, COM_Coord.y);

    pop();
  }


  void anylizeCellValues(int [] cell_values) {
    dumpPacket(cell_values);

    //  calc COM. COM : Center of Mass
    calcCOM_Rate();

    // //  calc coordinates of COM
    // COM_Coord.x = cellsOutline.getCenterX() + COM_Rate.x * cellsOutline.getWidth() / 2;
    // COM_Coord.y = cellsOutline.getCenterY() + COM_Rate.y * cellsOutline.getHeight() / 2;
    //println("vx=" + COM_Coord.x + ", vy=" + COM_Coord.y);
  }

  void dumpPacket(int [] packet_data) {
    for(int i = 0 ; i < numOfCells ; i++) {
      cellsValue[i] = packet_data[i];
    }
  }

  
  PVector getVectorTrim4(PVector v) {
    int x10000 = int(v.x * 10000.0f);
    v.x = float(x10000) / 10000;
    
    int y10000 = int(v.y * 10000.0f);
    v.y = float(y10000) / 10000;

    return v;
  }


  //  calculate center of mass of X
  //  refer to : http://www.softschools.com/formulas/physics/center_of_mass_formula/87/
  //  COM range : -1.0 ~ 1.0
  void calcCOM_Rate() {
    float com_x = 0.0f;
    float com_y = 0.0f;
    
    float sum_weight_pos_x = 0.0f;
    float sum_weight_pos_y = 0.0f;

    int sum_weight = 0;
  
    for(int cell_index = 0 ; cell_index < numOfCells ; cell_index++) {

      sum_weight_pos_x += cellsValue[cell_index] * cellsRect[cell_index].getCenterX();
      sum_weight_pos_y += cellsValue[cell_index] * cellsRect[cell_index].getCenterY();
      sum_weight += cellsValue[cell_index];

    }

    //  calculate COM
    
    if ( OCCUPY_THRESHOLD < sum_weight ) { // to prohibit zero divide error
      com_x = ( (sum_weight_pos_x / sum_weight) - cellsOutline.getCenterX() ) / (cellsOutline.getWidth() / 2);
      com_y = ( (sum_weight_pos_y / sum_weight) - cellsOutline.getCenterY() ) / (cellsOutline.getHeight() / 2);;
    } else {
      com_x = 0.0f; // 0.0f : center, range : (-1.0f ~ 1.0f)
      com_y = 0.0f; // 0.0f : center, range : (-1.0f ~ 1.0f)
    }

    COM_Rate.x = com_x;
    COM_Rate.y = com_y;

    //println(viewNick + "'s Center of Mass : " + COM_Rate.x + ", " + COM_Rate.y);
  }
  

  
  //  Calculate COC. COC : Center of Contour
  int iLeftEnd, iRightEnd;
  int coord_leftEnd,coord_rightEnd;  
  float coordCOC_X, coordLeftEnd, coordRightEnd;
  float coordRCOM_X;

  int INVALID_INDEX = (-1);
  
  void calcContour() {
    //  X
    //  Y
  }
  
  void calcRCOM(){
    //  COM - Center of Contour
    //  calc RCOM
    //coordRCOM_X = coordCOM.x - coordCOC_X;
  }
 
  
} // End of SensorCellView class
