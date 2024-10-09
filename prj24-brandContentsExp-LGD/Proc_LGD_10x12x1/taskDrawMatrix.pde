//==========================================================
//  CONFIG DATA VIEW
String [] csvPath_CellPos_A = {"./res/lgd_A.csv", "./res/lgd_B.csv", "./res/lgd_C.csv", "./res/lgd_D.csv"};

libMatrixView[]  matView_A;

int numOfMat = 1; // test : 4 is OK.


boolean create_MatrixView() {
  //  load csv file - rect cell position. <left, top, width, height> of 31 rect
  
  //---------------------------------
  //  Cell position
  matView_A = new libMatrixView[numOfMat];

  for(int mat_count = 0 ; mat_count < numOfMat ; mat_count++) {
    println("mat : " + mat_count);
    matView_A[mat_count] = new libMatrixView("Dog mat");
    // matView_A[mat_count] = new libMatrixView(String.format("Dog mat-%d", mat_count));

    if(false == matView_A[mat_count].createView_CSV(csvPath_CellPos_A[mat_count]))
      return false;

    libMatrixView mv0 = matView_A[mat_count];
    {
      mv0.setEditTextSize(14);
      mv0.setEditTextMargin(36, 24);
      println(String.format("libMatView UI of \"%s\" is built by CSV.", mv0.getNick()) );

      //  Config packet buffer
      configPacket(mv0.numOfCol, mv0.numOfRow);
      println(String.format("libPacket Buffer of \"%s\" is allocated by CSV.", mv0.getNick()) );
    }
  }

  return true;
}

void fill_MatrixView(){

}


boolean calculate_MatrixView() {
  for(int mat_count = 0 ; mat_count < numOfMat ; mat_count++) {
    matView_A[mat_count].anylizeCurData();
  }
  
  //-----------------------------------------------
  //  step 3. draw the x and y gauge of "center of mass"
  // PVector   vectorCOM_A = matView_A[0].getVectorCOM();
  //drawTextLog(String.format("Center Of Mass (X, Y) = LEFT (%2.3f, %2.3f), Right (%2.3f, %2.3f)"
  //                      , vectorCOM_A.x, vectorCOM_A.y, vectorCOM_B.x, vectorCOM_B.y));
  
  return true;
}

void draw_MatrixView() {
  for(int mat_count = 0 ; mat_count < numOfMat ; mat_count++) {
    matView_A[mat_count].drawColorCells();

  // matView_A[mat_count].drawCOMGauge();
  }
}
