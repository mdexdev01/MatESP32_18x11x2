//==========================================================
//  CONFIG DATA VIEW
String csvPath_CellPos_A = "./res/winnieconnie.csv";
String csvPath_CellPos_B = "./res/CellPos_Block_B.csv";
String csvPath_CellPos_C = "./res/CellPos_Block_C.csv";

libMatrixView  matView_A;
libMatrixView  matView_B;
libMatrixView  matView_C;


boolean setup_MatrixView() {
  //  load csv file - rect cell position. <left, top, width, height> of 31 rect
  
  //---------------------------------
  //  Cell position - A
  matView_A = new libMatrixView("Air mat");
  if(false == matView_A.createView_CSV(csvPath_CellPos_A))
    return false;

  libMatrixView mv0 = matView_A;
  {
    mv0.setEditTextSize(22);
    mv0.setEditTextMargin(60, 32);
    println(String.format("libMatView UI of \"%s\" is built by CSV.", mv0.getNick()) );

    //  Config packet buffer
    configPacket(mv0.numOfCol, mv0.numOfRow);
    println(String.format("libPacket Buffer of \"%s\" is allocated by CSV.", mv0.getNick()) );
  }

  return true;
}


boolean update_EditView() {
  matView_A.anylizeCurData();
  
  matView_A.drawColorCells();

  // matView_A.drawCOMGauge();

  //-----------------------------------------------
  //  step 3. draw the x and y gauge of "center of mass"
  PVector   vectorCOM_A = matView_A.getVectorCOM();

  //strBuffer = String.format("Center Of Mass (X, Y) = LEFT (%2.3f, %2.3f), Right (%2.3f, %2.3f)"
  //                      , vectorCOM_A.x, vectorCOM_A.y, vectorCOM_B.x, vectorCOM_B.y);
  //drawTextLog(strBuffer);
  
  return true;
}
