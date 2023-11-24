//==========================================================
//  CONFIG DATA VIEW
String csvPath_CellPos_A = "./res/CellPos_Block_A.csv";
String csvPath_CellPos_B = "./res/CellPos_Block_B.csv";
String csvPath_CellPos_C = "./res/CellPos_Block_C.csv";

libMatrixView  matView_A;
libMatrixView  matView_B;
libMatrixView  matView_C;


boolean setup_MatrixView() {
  //  load csv file - rect cell position. <left, top, width, height> of 31 rect
  Table     csvRectPos_A;
  Table     csvRectPos_B;
  Table     csvRectPos_C;
  
  //---------------------------------
  //  Cell position - A
  matView_A = new libMatrixView("mat-A");
  if(false == matView_A.createView_CSV(csvPath_CellPos_A))
    return false;
  println(matView_A.getNick() + " created...");
  matView_A.setEditTextSize(11);

  //  Cell position - B
  matView_B = new libMatrixView("mat-B");
  if(false == matView_B.createView_CSV(csvPath_CellPos_B))
    return false;
  println(matView_B.getNick() + " created...");
  matView_B.setEditTextSize(11);

  //  Cell position - C
  matView_C = new libMatrixView("mat-C");
  if(false == matView_C.createView_CSV(csvPath_CellPos_C))
    return false;
  println(matView_C.getNick() + " created...");
  matView_C.setEditTextSize(11);

  //  Config packet buffer
  configPacket(matView_A.numOfCol, matView_A.numOfRow);

  return true;
}


boolean update_EditView() {
  matView_A.anylizeCurData();
  matView_B.anylizeCurData();
  matView_C.anylizeCurData();
  
  matView_A.drawColorCells();
  matView_B.drawColorCells();
  matView_C.drawColorCells();

  // matView_A.drawCOMGauge();

  //-----------------------------------------------
  //  step 3. draw the x and y gauge of "center of mass"
  PVector   vectorCOM_A = matView_A.getVectorCOM();

  //strBuffer = String.format("Center Of Mass (X, Y) = LEFT (%2.3f, %2.3f), Right (%2.3f, %2.3f)"
  //                      , vectorCOM_A.x, vectorCOM_A.y, vectorCOM_B.x, vectorCOM_B.y);
  //drawTextLog(strBuffer);
  
  return true;
}
