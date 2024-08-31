

class LED_Object {
public :

  int x;
  int y;
  int z;

  LED_Object() {};

  LED_Object(int a, int b, int c) {
    x = a;
    y = b;
    z = c;
  }

  // Addition
  LED_Object operator + ( const LED_Object &r ) {
      LED_Object v = { x + r.x ,
                  y + r.y ,
                  z + r.z };
      return v;
  }

  // Subtraction
  LED_Object operator - ( const LED_Object &r ) {
      LED_Object v = { x - r.x ,
                  y - r.y ,
                  z - r.z };
      return v;
  }

  // Negation
  LED_Object operator - ( void ) {
      LED_Object v = { -x ,
                  -y ,
                  -z };
      return v;
  }

  // Increment
  void operator += ( const LED_Object &r ) {
      x += r.x;
      y += r.y;
      z += r.z;
  }

  // Decrement
  void operator -= ( const LED_Object &r ) {
      x -= r.x;
      y -= r.y;
      z -= r.z;
  } 


  //----------- Element-wise multiplication ------------
  LED_Object operator ^ ( const LED_Object &r ) {
    LED_Object v = { x * r.x ,
                y * r.y ,
                z * r.z };
    return v;
  }

  void operator ^= ( const LED_Object &r ) {
    x *= r.x;
    y *= r.y;
    z *= r.z;
  }  

  //-------- Scalar multiplication and division --------
    
  // Scalar product
  LED_Object operator * ( const float s ) {
      LED_Object v = { int(x * s) ,
                  int(y * s) ,
                  int(z * s) };
      return v;
  }

  // Scalar division
  LED_Object operator / ( const float s ) {
      LED_Object v = { int(x / s) ,
                  int(y / s) ,
                  int(z / s) };
      return v;
  }

  // Self multiply
  void operator *= ( const float s ) {
      x = int(x * s);
      y = int(y * s);
      z = int(z * s);
  }

  // Self divide
  void operator /= ( const float s ) {
      x /= s;
      y /= s;
      z /= s;
  }

  // Reverse order - Scalar product --- [Global operator] 
  // LED_Object operator * ( const float s, const LED_Object &r ) {
  //     LED_Object v = { r.x * s ,
  //               r.y * s ,
  //               r.z * s };
  //     return v;
  // }
};

const int cop_fifo_len = 5;
int fifo_next_index = 0;
int fifo_in_index = 0;

LED_Object COP_RingBuf[cop_fifo_len];

LED_Object getLastCOP() {
  return COP_RingBuf[fifo_in_index];
}


