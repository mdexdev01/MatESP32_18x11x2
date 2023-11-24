//////////////////////////////////////////////////////////////////////
//  CLASS  DoubleArrayView
//  Draw color cells and COM gauge
//////////////////////////////////////////////////////////////////////
class DoubleArrayView {
  //  num of cells in each row  
  int numOfRow = 6;
  int numOfCol = 10;
  int numOfCells = numOfRow * numOfCol;
  int numOfArray = 2; 

  int [] ArrayA_Values;
  int [] ArrayB_Values;
  
  EditRect [] ArrayA_Rect;
  EditRect [] ArrayB_Rect;
  
  EditRect ArrayA_Outline;
  EditRect ArrayB_Outline;
  
  EditRect ArrayA_Contour;
  EditRect ArrayB_Contour;
 

  PVector ArrayA_COM;
  PVector ArrayB_COM;

  PVector ArrayA_COM_Coord;
  PVector ArrayB_COM_Coord;
  
  EditRect RectGauge_Ax;
  EditRect RectGauge_Ay;
  EditRect RectGauge_Bx;
  EditRect RectGauge_By;
  
  EditRect extGaugeText_Ax;
  EditRect extGaugeText_Ay;
  EditRect extGaugeText_Bx;
  EditRect extGaugeText_By;

 
  int OCCUPY_THRESHOLD = 50;
  
  
 
  int VALID_VALUE = 5;

  DoubleArrayView(int num_col, int num_row) {
    numOfRow = num_row;
    numOfCol = num_col;
    numOfCells = numOfRow * numOfCol;
    allocCellBuffer(numOfRow * numOfCol);

    ArrayA_COM = new PVector(0.0f, 0.0f);
    ArrayB_COM = new PVector(0.0f, 0.0f);
    
    ArrayA_COM_Coord = new PVector(0.0f, 0.0f);
    ArrayB_COM_Coord = new PVector(0.0f, 0.0f);
  };

  void allocCellBuffer(int cell_num) {
    numOfCells = cell_num;

    ArrayA_Values = new int [numOfCells]; 
    ArrayB_Values = new int [numOfCells]; 

    ArrayA_Rect = new EditRect [numOfCells]; 
    ArrayB_Rect = new EditRect [numOfCells]; 
  }
  
  EditRect getOneCell_AreaA(int col, int row) {
    return ArrayA_Rect[col + numOfCol * row];
  };  
  
  EditRect getOneCell_AreaB(int col, int row) {
    return ArrayB_Rect[col + numOfCol * row];
  };  
  
  PVector getVectorCOM_A() {
    return ArrayA_COM;
  }

  PVector getVectorCOM_B() {
    return ArrayB_COM;
  }


  boolean loadCSV_DoubleArrayPosition(Table table) {
    //Table table = file_name;
    //table = loadTable("../image/rect_pos.csv", "header");
    
    if(table.getRowCount() != numOfCells ) { // 2 : header lines, 2 : num of Arrays
      println("Error, Row count is " + table.getRowCount()); 
      return false;
    }
      
    println(table.getRowCount() + " total rows in rect_pos.csv file");
    
    //  load csv file 
    String cell_name;
    int [] rect_left_A  = new int [numOfCells];
    int [] rect_top_A   = new int [numOfCells];
    int [] rect_width_A = new int [numOfCells];
    int [] rect_height_A = new int [numOfCells];

    int [] rect_left_B  = new int [numOfCells];
    int [] rect_top_B   = new int [numOfCells];
    int [] rect_width_B = new int [numOfCells];
    int [] rect_height_B = new int [numOfCells];

    int i = 0;
    for (TableRow row : table.rows()) {
      cell_name       = row.getString("[Row-Col]");
      rect_left_A[i]    = row.getInt("A-LEFT");
      rect_top_A[i]     = row.getInt("A-TOP");
      rect_width_A[i]   = row.getInt("A-WIDTH");
      rect_height_A[i]  = row.getInt("A-HEIGHT");
       
      rect_left_B[i]    = row.getInt("B-LEFT");
      rect_top_B[i]     = row.getInt("B-TOP");
      rect_width_B[i]   = row.getInt("B-WIDTH");
      rect_height_B[i]  = row.getInt("B-HEIGHT");
       
      println(cell_name + ", " + rect_left_A[i] + ", " + rect_top_A[i] + ", " + rect_width_B[i] + ", " + rect_height_B[i]);
      i++;
    }
    
    println("read csv out");
    


    //  create Rect UI
    i = 0;
    int row_index = 0; // column index
    int col_index = 0; // column index
    
    for (row_index = 0; row_index < numOfRow; row_index++) {
      for (col_index = 0; col_index < numOfCol; col_index++) {
        i = row_index * numOfCol + col_index;
        
        //////////////////////////////////////////////DELETE + 8 !!!!!!!!
        ArrayA_Rect[i] = new EditRect(rect_left_A[i], rect_top_A[i], rect_width_A[i] + 8, rect_height_A[i]);
        ArrayA_Rect[i].setFontSize(textSize);
        
        ArrayB_Rect[i] = new EditRect(rect_left_B[i], rect_top_B[i], rect_width_B[i] + 8, rect_height_B[i]);
        ArrayB_Rect[i].setFontSize(textSize);
      }
    }
    println("make rect of Array A & B");

    
    learnGeometry();
    
    return true;
  }
  
  
  void learnGeometry(){
    int left_end, right_end, top_end, bottom_end;
    
    // calculate outline of A
    left_end  = ArrayA_Rect[0].getCenterX();
    top_end   = ArrayA_Rect[numOfCells - 1].getCenterY();

    right_end = ArrayA_Rect[numOfCells - 1].getCenterX();
    bottom_end =ArrayA_Rect[0].getCenterY();
    
    ArrayA_Outline = new EditRect(left_end, top_end, right_end - left_end, bottom_end - top_end);
    println("A - outline (l,r,t,b) : " + left_end + ", " + right_end + ", " + top_end + ", " + bottom_end);
    
    // calculate outline of B
    left_end  = ArrayB_Rect[0].getCenterX();
    top_end   = ArrayB_Rect[numOfCells - 1].getCenterY();

    right_end = ArrayB_Rect[numOfCells - 1].getCenterX();
    bottom_end =ArrayB_Rect[0].getCenterY();
    
    ArrayB_Outline = new EditRect(left_end, top_end, right_end - left_end, bottom_end - top_end);
    
    println("B - outline (l,r,t,b) : " + left_end + ", " + right_end + ", " + top_end + ", " + bottom_end);

    ///
    ArrayA_Contour = new EditRect(ArrayA_Outline.getCenterX(), ArrayA_Outline.getCenterY(), 1, 1); 
    ArrayB_Contour = new EditRect(ArrayB_Outline.getCenterX(), ArrayB_Outline.getCenterY(), 1, 1); 
    
  }


  int gaugeX_Top_A = 0;
  int gaugeX_Top_B = 0;
  int gaugeX_offset_Y = 120;
  int gaugeX_Height = 20;

  int gaugeY_Left_A = 0;
  int gaugeY_Left_B = 0;
  int gaugeY_offset_X = 75;
  int gaugeY_Width = 32;
  
  void createCOMGauge() {
    int textSize = 15;
    
    //gaugeX_Top_A = ArrayA_Outline.getTop()-gaugeX_Height_A * 2 + 10;
    gaugeX_Top_A = ArrayA_Outline.getBottom() + gaugeX_offset_Y;
    RectGauge_Ax = new EditRect(ArrayA_Outline.getLeft() - 10, gaugeX_Top_A, ArrayA_Outline.getWidth() + 20, gaugeX_Height ); // left, top, width, height
    RectGauge_Ax.setTextLeftMargin(ArrayA_Outline.getWidth() / 2);
    //RectGauge_Ax.setTextLeftMargin(3);
    
    extGaugeText_Ax = new EditRect(RectGauge_Ax.getRight() +15, gaugeX_Top_A, 45, 20 ); // left, top, width, height
    extGaugeText_Ax.setFontSize(textSize);
    extGaugeText_Ax.setTextLeftMargin(3);
    
    //gaugeX_Top_B = ArrayB_Outline.getTop()-gaugeX_Height_B * 2 + 10;
    gaugeX_Top_B = ArrayB_Outline.getBottom() + gaugeX_offset_Y;
    RectGauge_Bx = new EditRect(ArrayB_Outline.getLeft() - 10, gaugeX_Top_B, ArrayB_Outline.getWidth() + 20, gaugeX_Height ); // left, top, width, height
    RectGauge_Bx.setTextLeftMargin(ArrayB_Outline.getWidth() / 2);
    //RectGauge_Bx.setTextLeftMargin(3);
    
    extGaugeText_Bx = new EditRect(RectGauge_Bx.getLeft() -60, gaugeX_Top_B, 45, 20 ); // left, top, width, height
    extGaugeText_Bx.setFontSize(textSize);
    extGaugeText_Bx.setTextLeftMargin(1);

    //  230104
    gaugeY_Left_A = ArrayA_Outline.getLeft() - gaugeY_offset_X;
    RectGauge_Ay = new EditRect(gaugeY_Left_A, ArrayA_Outline.getTop() - 10, gaugeY_Width, ArrayA_Outline.getHeight() + 20); // left, top, width, height
    //RectGauge_Ay.shiftTextDown(ArrayA_Outline.getHeight() / 3);
    RectGauge_Ay.setTextLeftMargin(1);
    
    extGaugeText_Ay = new EditRect(RectGauge_Ay.getLeft() -50, RectGauge_Ay.getCenterY()-10, 45, 20 ); // left, top, width, height
    extGaugeText_Ay.setFontSize(textSize);
    extGaugeText_Ay.setTextLeftMargin(1);

    //gaugeY_Left_B = ArrayB_Outline.getLeft() - gaugeY_width * 2 + 15;
    gaugeY_Left_B = ArrayB_Outline.getRight() + 43;
    RectGauge_By = new EditRect(gaugeY_Left_B, ArrayB_Outline.getTop() - 10, gaugeY_Width, ArrayB_Outline.getHeight() + 20); // left, top, width, height
    //RectGauge_By.shiftTextDown(ArrayB_Outline.getHeight() / 3);
    RectGauge_By.setTextLeftMargin(1);
    
    extGaugeText_By = new EditRect(RectGauge_By.getRight() +5, RectGauge_By.getCenterY()-10, 45, 20 ); // left, top, width, height
    extGaugeText_By.setFontSize(textSize);
    extGaugeText_By.setTextLeftMargin(1);

  }
  
  //// color map of HSB
  //int max_cell_val = 180;
  //int colorMap(int cell_value) {
  //  // make 0 ~ max_cell_val(cell_value) to 180 ~ 0(hue)
  //  int hue = 180 - (cell_value * 180 / max_cell_val ); 

  //  if(max_cell_val < cell_value)
  //    hue = 0;

  //  return hue;
  //}
  
  int textSize = 12;
  
  void setEditTextSize(int t_size) {
    textSize = t_size;

    int i = 0;
    int row_index = 0; // column index
    int col_index = 0; // column index
    
    println("DoubleArrayView::setEditTextSize");
    
    for (row_index = 0; row_index < numOfRow; row_index++) {
      for (col_index = 0; col_index < numOfCol; col_index++) {
        i = row_index * numOfCol + col_index;
        
        ArrayA_Rect[i].setFontSize(textSize);
        
        ArrayB_Rect[i].setFontSize(textSize);
      }
    }

  }
  
  void drawColorCells(){  
    int i = 0;
    String strMsg = " ";
    
    textSize(textSize);


    for ( i = 0 ; i < numOfCells ; i++ ) {
/*      
      strMsg = String.format("%d-%d", ArrayA_Values[i] / 100, ArrayA_Values[i] % 100);
      ArrayA_Rect[i].render();  
      ArrayA_Rect[i].showString(strMsg);
      
      strMsg = String.format("%d-%d", ArrayB_Values[i] / 100, ArrayA_Values[i] % 100);
      ArrayB_Rect[i].render();  
      ArrayB_Rect[i].showString(strMsg);
*/
      
      //  HSB : https://codepen.io/HunorMarton/details/eWvewo
      if(ArrayA_Values[i] == 0) {
        fill(colorMap(ArrayA_Values[i]), 0, 80); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
      }
      else {
        fill(colorMap(ArrayA_Values[i]), 90, 90); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
      }
      ArrayA_Rect[i].render();  
      ArrayA_Rect[i].showInt(ArrayA_Values[i]);
      
      if(ArrayB_Values[i] == 0) {
        fill(colorMap(ArrayB_Values[i]), 0, 80); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
      }
      else{
        fill(colorMap(ArrayB_Values[i]), 90, 90); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
      }
      ArrayB_Rect[i].render();  
      ArrayB_Rect[i].showInt(ArrayB_Values[i]);
    }
  }

  

  void drawCOMGauge() {
    //line(x1, y1, x2, y2)
    //rect(x, y, w, h)  
   
    int niddle_thick_half = 5;
    
    push();
    //  --------------------------------------------------
    //    Array A
    //  draw X gauge
    fill(0, 10, 0); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(60, 30, 10); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    RectGauge_Ax.render();
    RectGauge_Ax.showFloat(ArrayA_COM.x);
    
    fill(0, 0, 100); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    extGaugeText_Ax.render();
    extGaugeText_Ax.showFloat(ArrayA_COM.x);

    //  draw niddle
    fill(0, 0, 95); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(60, 70, 60); // black(0, 0, 0) . white(0, 0, 100), yellow(60, 70, 60), color (180~0, 100, 100) 
    //line(ArrayA_COM_Coord.x, gaugeX_Top_A, ArrayA_COM_Coord.x, gaugeX_Top_A + gaugeX_Height);
    rect(ArrayA_COM_Coord.x - niddle_thick_half, gaugeX_Top_A, niddle_thick_half * 2, gaugeX_Height);

    //  draw Y gauge
    fill(0, 10, 0); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(60, 30, 10); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    RectGauge_Ay.render();
    RectGauge_Ay.showFloat(ArrayA_COM.y);
    
    fill(0, 0, 100); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    extGaugeText_Ay.render();
    extGaugeText_Ay.showFloat(ArrayA_COM.y);
    
    //  draw niddle
    fill(0, 0, 95); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(0, 0, 100); // black(0, 0, 0) . white(0, 0, 100), yellow(60, 70, 60), color (180~0, 100, 100) 
    //line(gaugeY_Left_A, ArrayA_COM_Coord.y, gaugeY_Left_A + gaugeY_Width, ArrayA_COM_Coord.y);
    rect(gaugeY_Left_A, ArrayA_COM_Coord.y - niddle_thick_half, gaugeY_Width, niddle_thick_half * 2);
  
    //  --------------------------------------------------
    //    Array B
    //  draw X gauge
    fill(0, 10, 0); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(60, 30, 10); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    RectGauge_Bx.render();
    RectGauge_Bx.showFloat(ArrayB_COM.x);
    
    fill(0, 0, 100); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    extGaugeText_Bx.render();
    extGaugeText_Bx.showFloat(ArrayB_COM.x);
  
    //  draw niddle
     // [0] : Any index of Row1(0~14) can replace 0.
    fill(0, 0, 95); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(60, 70, 60); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    //line(ArrayB_COM_Coord.x, gaugeX_Top_B, ArrayB_COM_Coord.x, gaugeX_Top_B + gaugeX_Height);
    rect(ArrayB_COM_Coord.x - niddle_thick_half, gaugeX_Top_B, niddle_thick_half * 2, gaugeX_Height);

    //  draw Y gauge
    fill(0, 10, 0); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(60, 30, 10); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    RectGauge_By.render();
    RectGauge_By.showFloat(ArrayB_COM.y);
    
    fill(0, 0, 100); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    extGaugeText_By.render();
    extGaugeText_By.showFloat(ArrayB_COM.y);

    //  draw niddle
    fill(0, 0, 95); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    stroke(60, 70, 60); // black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    //line(gaugeY_Left_B, ArrayB_COM_Coord.y, gaugeY_Left_B + gaugeY_Width, ArrayB_COM_Coord.y);
    //rect(gaugeY_Left_B, ArrayB_COM_Coord.y-5, gaugeY_Left_B + gaugeY_Width, ArrayB_COM_Coord.y+5);
    rect(gaugeY_Left_B, ArrayB_COM_Coord.y - niddle_thick_half, gaugeY_Width, niddle_thick_half * 2);

    pop();
  }


  void anylizeCellValues(int [] cell_values) {
    reorderPacketToDoubleView(cell_values);
    analyzeDist();
  }

  void reorderPacketToDoubleView(int [] packet_data) {
    for(int i = 0 ; i < numOfCells ; i++) {
      ArrayA_Values[i] = packet_data[i];
    }

    for(int i = 0 ; i < numOfCells ; i++) {
      ArrayB_Values[i] = packet_data[numOfCells + i];
    }
  }


  //  anylyze distribution such as center of mass, ...
  void analyzeDist() {    
    //  calc COM. COM : Center of Mass
    calcCOM_Double();

    //  calc coordinates of COM
    ArrayA_COM_Coord.x = ArrayA_Outline.getCenterX() + ArrayA_COM.x * ArrayA_Outline.getWidth() / 2;
    ArrayA_COM_Coord.y = ArrayA_Outline.getCenterY() + ArrayA_COM.y * ArrayA_Outline.getHeight() / 2;
    
    ArrayB_COM_Coord.x = ArrayB_Outline.getCenterX() + ArrayB_COM.x * ArrayB_Outline.getWidth() / 2;
    ArrayB_COM_Coord.y = ArrayB_Outline.getCenterY() + ArrayB_COM.y * ArrayB_Outline.getHeight() / 2;
    
    //println("vx=" + ArrayA_COM_Coord.x + ", vy=" + ArrayA_COM_Coord.y);
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
  void calcCOM_Double() {
    float com_a_x = 0.0f;
    float com_a_y = 0.0f;
    float com_b_x = 0.0f;
    float com_b_y = 0.0f;
    
    float sum_weight_pos_a_x = 0.0f;
    float sum_weight_pos_a_y = 0.0f;
    float sum_weight_pos_b_x = 0.0f;
    float sum_weight_pos_b_y = 0.0f;

    int sum_weight_a = 0;
    int sum_weight_b = 0;
  
    float pitch_x = 2.0f / (numOfCol - 1) ; // 2/( 7-1). 2 means the whole length of range : -1~1. so 0 is center. ( 7-1) means the number of interval of  7 cells. 
    //float pitch_y = 2.0f / (numOfRow - 1) ; // 2/(11-1). 2 means the whole length of range : -1~1. so 0 is center. (11-1) means the number of interval of 11 cells. 
    float pitch_y = -2.0f / (numOfRow - 1) ; // cell index starts from bottom in the screen then -2.0f, otherwise from top then 2.0f 
    
    for(int cell_index = 0 ; cell_index < numOfCells ; cell_index++) {
      float cell_pos_x = -1.0f + (cell_index % numOfCol) * pitch_x; // -1.0f means the most left cell's x-position while the most right cell is 1.0f.
      //float cell_pos_y = -1.0f + (cell_index / numOfCol) * pitch_y; // -1.0f means the most left cell's x-position while the most right cell is 1.0f.
      float cell_pos_y = 1.0f + (cell_index / numOfCol) * pitch_y; // cell index starts from bottom in the screen then 1.0f, otherwise from top then -1.0f
      
      //  Array A
      sum_weight_pos_a_x += ArrayA_Values[cell_index] * cell_pos_x;
      sum_weight_pos_a_y += ArrayA_Values[cell_index] * cell_pos_y;
      sum_weight_a += ArrayA_Values[cell_index];


      //  Array B
      sum_weight_pos_b_x += ArrayB_Values[cell_index] * cell_pos_x;
      sum_weight_pos_b_y += ArrayB_Values[cell_index] * cell_pos_y;
      sum_weight_b += ArrayB_Values[cell_index];
    }

    //  calculate COM
    if ( OCCUPY_THRESHOLD < sum_weight_a ) {
      com_a_x = sum_weight_pos_a_x / sum_weight_a;
    } else {
      com_a_x = 0.0f; // 0.0f : center, range : (-1.0f ~ 1.0f)
    }
    ArrayA_COM.x = com_a_x;
    //println("com_a_x : " + com_a_x);

    if ( OCCUPY_THRESHOLD < sum_weight_a ) {
      com_a_y = sum_weight_pos_a_y / sum_weight_a;
    } else {
      com_a_y = 0.0f; // 0.0f : center, range : (-1.0f ~ 1.0f)
    }
    //println("com_a_y : " + com_a_y);
    ArrayA_COM.y = com_a_y;

    if ( OCCUPY_THRESHOLD < sum_weight_b ) {
      com_b_x = sum_weight_pos_b_x / sum_weight_b;
    } else {
      com_b_x = 0.0f; // 0.0f : center, range : (-1.0f ~ 1.0f)
    }
    ArrayB_COM.x = com_b_x;
    //println("com_b_x : " + com_b_x);

    if ( OCCUPY_THRESHOLD < sum_weight_b ) {
      com_b_y = sum_weight_pos_b_y / sum_weight_b;
    } else {
      com_b_y = 0.0f; // 0.0f : center, range : (-1.0f ~ 1.0f)
    }
    ArrayB_COM.y = com_b_y;
    //println("com_b_y : " + com_b_y);

  }
  


  
  //  Calculate COC. COC : Center of Contour
  int iLeftEnd, iRightEnd;
  int coord_leftEnd,coord_rightEnd;  
  float coordCOC_X, coordLeftEnd, coordRightEnd;
  float coordRCOM_X;

  int INVALID_INDEX = (-1);
  
  //  choose the max value of 7 cells in this row
  boolean isValidCellInThisRow(int [] values, int row) {
    for(int i = 0 ; i < numOfCol ; i++){
      if(VALID_VALUE < values[numOfCol * row + i])
        return true;
    }
    return false;
  }
  
  //  choose the max value of 11 cells in this column
  boolean isValidCellInThisCol(int [] values, int col) {
    for(int i = 0 ; i < numOfRow ; i++){
      if(VALID_VALUE < values[numOfCol * i + col])
        return true;
    }
    return false;
  }
  
  void calcContour() {
    //  A - X
    iLeftEnd = iRightEnd = INVALID_INDEX;
    coordLeftEnd = coordRightEnd = ArrayA_Outline.getCenterX();
    coord_leftEnd = coord_rightEnd = ArrayA_Outline.getCenterX();

    for(int i = 0 ; i < numOfCol ; i++) {
      if(true == isValidCellInThisCol(ArrayA_Values, i)) {
        iLeftEnd = i;
        break;
      }      
    }
    for(int i = (numOfCol - 1) ; 0 <= i ; i--) {
      if(true == isValidCellInThisCol(ArrayA_Values, i)) {
        iRightEnd = i;
        break;
      }
    }
    
    if( (iLeftEnd == INVALID_INDEX) || (iRightEnd == INVALID_INDEX) ) {
      ArrayA_Contour.setRect(ArrayA_Outline.getCenterX(), ArrayA_Outline.getCenterY(), 1, 1);
    }
    else {
      coordLeftEnd = ArrayA_Rect[iLeftEnd].getCenterX();
      coordRightEnd = ArrayA_Rect[iRightEnd].getCenterX();
    }
    
    //  A - Y
    //  B - X
    //  B - Y
  }
  
  void calcRCOM(){
    //  COM - Center of Contour
    //  calc RCOM
    //coordRCOM_X = coordCOM.x - coordCOC_X;
  }

} // End of class
