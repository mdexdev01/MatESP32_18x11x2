
//////////////////////////////////////////////////////////////////////
//  CLASS  EditRect
//  Rectangle with text
//////////////////////////////////////////////////////////////////////
class EditRect {
  int x, y; // starting point
  int w, h; // width, height
  
  EditRect(int start_x, int start_y, int rect_width, int rect_height){
    x = start_x;
    y = start_y;
    w = rect_width;
    h = rect_height;
    
    text_margin_left = rect_width / 2 - 5;
    text_shift_down = rect_height / 2 + rect_height / 5;
  };  
  void setRect(int start_x, int start_y, int rect_width, int rect_height){
    x = start_x;
    y = start_y;
    w = rect_width;
    h = rect_height;
  }
  int getLeft() { return x; };
  int getRight() { return (x + w); };
  int getWidth() { return w; };
  int getTop() { return y; };
  int getBottom() { return (y + h); };
  int getHeight() { return h; };
  int getCenterX() { return (x + w/2);};
  int getCenterY() { return (y + h/2);};

  void render(){
    rect(x, y, w, h);
  }
  
  int font_size = 12;
  void setFontSize(int f_size) {
    font_size = f_size;
  }
  
  int text_margin_left = 3;
  int text_shift_down  = 14;
  
  void setTextLeftMargin(int left_margin) {
    text_margin_left = left_margin;
  }
  void setTextTopMargin(int top_margin) {
    text_shift_down = top_margin;
  }
  
  void shiftTextDown(int shift_size) {
    text_shift_down = shift_size;
  }
  void showInt(int v){
    push();
    fill(0, 0, 0);// black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    textSize(font_size);
    text(v, x + text_margin_left, y + text_shift_down);
    pop();
  }                                                                            
  void showFloat(float v){
    push();
    fill(0, 0, 0);// black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    //fill(60, 70, 60);// black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    textSize(font_size);
    text(v, x + text_margin_left, y + text_shift_down);
    pop();
  }
  void showString(String s){
    push();
    fill(0, 0, 0);// black(0, 0, 0) . white(0, 0, 100), color (180~0, 100, 100) 
    textSize(font_size);
    text(s, x + text_margin_left, y + text_shift_down);
    pop();
  }
}
