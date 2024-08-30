//==========================================================
//  CONFIG DATA VIEW
String [] csvPath_CellPos_A = {"./res/brandcontentsA.csv", "./res/brandcontentsB.csv", "./res/brandcontentsC.csv"};
// String csvPath_CellPos_B = "./res/brandcontentsB.csv";
// String csvPath_CellPos_C = "./res/brandcontentsC.csv";

libMatrixView[]  matView_A;
// libMatrixView  matView_B;
// libMatrixView  matView_C;

int numOfMat = 3;


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
      mv0.setEditTextSize(16);
      mv0.setEditTextMargin(30, 16);
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
  // PVector   vectorCOM_A = matView_A.getVectorCOM();
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
