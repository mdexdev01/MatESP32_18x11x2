// This example sketch connects to the public shiftr.io instance and sends a message on every keystroke.
// After starting the sketch you can find the mqttClient here: https://www.shiftr.io/try.
//
// Note: If you're running the sketch via the Android Mode you need to set the INTERNET permission
// in Android > Sketch Permissions.
//
// by Joël Gähwiler
// https://github.com/256dpi/processing-mqtt

import processing.net.*;
import java.util.Arrays;
import java.net.UnknownHostException;

Client tcp_c;
String input;
int data[];
byte [] packet_buf;
int packet_len = 512;

int NUM_OF_FRAME = 40;
byte [][] FRAME_BUFFER;

int frameIndexRd = 0;
int frameIndexWr = 0;
int rollFI(int num) { return (num % NUM_OF_FRAME); }
int getCurBufFIRd() { return rollFI(frameIndexRd); }
int getCurBufFIWr() { return rollFI(frameIndexWr); }
int getPrevBufFIRd() { if(frameIndexRd == 0) return (NUM_OF_FRAME-1); else  return rollFI(frameIndexRd - 1); }
int getPrevBufFIWr() { if(frameIndexWr == 0) return (NUM_OF_FRAME-1); else  return rollFI(frameIndexWr - 1); }
int IncFIRd() { return rollFI(++frameIndexRd); }
int IncFIWr() { return rollFI(++frameIndexWr); }

byte [] remainBuffer;

int HtoI(byte num) {
  int ret_val = 0;
  if(num < 0) {
    ret_val = 256 + num;
  } else {
    ret_val = num;
  }
  return ret_val; 
}


void setup() {
  size(450, 255);
  background(204);
  stroke(0);
  frameRate(5); // Slow it down a little
  
  packet_buf = new byte[packet_len * 3 * 5];
  FRAME_BUFFER = new byte[NUM_OF_FRAME][];
  for(int i = 0 ; i < NUM_OF_FRAME ; i++) {
    FRAME_BUFFER[i] = new byte[packet_len];
  }
  
  remainBuffer = new byte[packet_len];
  
  setup_Timer();

  setup_mqtt_shiftr();
  
  thread("task_tcp_client");

}

void setup_tcp_client(String serverIP, int serverPort) {
  //tcp_c = new Client(this, "172.30.1.66", 800); // Replace with your server's IP and port
  tcp_c = new Client(this, serverIP, serverPort); // Replace with your server's IP and port
  
  println("try tcp_c connect....");
  delay(500);
}

int loop_count = 0;
void draw() {
  if(mqtt_tcp_server_info_updated == true) {
    setup_tcp_client(getTcpServerIP(), getTcpServerPort());
    mqtt_tcp_server_info_updated = false;
  }

  //loop_tcp_client();
  
  {
    int wr_last = frameIndexWr;
    if(0 < frameIndexWr ) {
      int rd_last = frameIndexRd;
      println("rx_len = " + (wr_last - rd_last));
      
      for (int i = rd_last ; i < wr_last ; i++) {
        int rd_index = getCurBufFIRd();
        IncFIRd();
      
        int rx_count = (HtoI(FRAME_BUFFER[rd_index][2]) - 1) * 10000 + (HtoI(FRAME_BUFFER[rd_index][3])-1) * 100 + HtoI(FRAME_BUFFER[rd_index][4]) - 1;
        //println("rx_count = " + rx_count);        
      }
    }
  }
  
  loop_count++;
}

int remainSize = 0;
int good_count = 0;
boolean doubt_disconnect = false;

void task_tcp_client() {
  while(true) {
    loop_tcp_client();
    delay(1);
  }
}

int watch_tcp_status() {
  if(tcp_c == null) {
    println("no server info. tcp_c is not created");
    delay(1000);
    return -1;
  }
  
  //tcp_c.write("client to server\n");
  
  if(tcp_c.active() == false) {
    tcp_c.clear();
    println("--- tcp clear ---");
  //mqttClient.subscribe(mqtt_topic_server);
    setup_mqtt_shiftr();
    println("tcp_c is not alive, turn on mqtt sub");
    return -2;
  }
  
  int available_len = 0;
 
  available_len = tcp_c.available();
  
  //  MEASURE NO PACKET TIME AND CLEAR TCP
  boolean is_server_bye = false;
  if(available_len == 0) {
    if(doubt_disconnect == false) {
      setGrantNow_Timer();
    }
    doubt_disconnect = true;
    if(2000 < getGrantElapsed_Timer()) {
      is_server_bye = true;
    }
    delay(500);
  }
  else {
    doubt_disconnect = false;    
  }
  
  if(is_server_bye == true) {
    println("--- doubt server is dead ---");
    try {
      tcp_c.clear();
      tcp_c.stop();
      tcp_c = null;
    } catch (Exception e) {
      println("tcp e)" + e);
    }
      
    println("--- tcp stop ---");
    //mqttClient.subscribe(mqtt_topic_server);
    setup_mqtt_shiftr();
    println("tcp_c is not alive, turn on mqtt sub");
    return -3;
  }

  return available_len;

}

void loop_tcp_client() 
{
  println("\n < new loop >");
  int available_len = watch_tcp_status();
  
  if(available_len < 0) 
    return;
  
  int read_len = 0;
  //-------------
  
  if(available_len < packet_len) {
    delay(1);
    return;
  }

  try {
    read_len = tcp_c.readBytes(packet_buf);
    //println("packet length = " + read_len + " [good: 512, 1024, 1536, 2048, 2560]");
    //  input = tcp_c.readString();
    //  input = input.substring(0, input.indexOf("\n")); // Only up to the newline

  } catch (Exception e) {
    println("tcp e)" + e);
  }

  
  int num_of_packet = 0;
  int offset = 0;
  
  if(0 < remainSize) {
    if( (HtoI(remainBuffer[0]) == 0xFF) && (HtoI(packet_buf[packet_len - remainSize - 1]) == 0xFE) ) {
      //arrayCopy(packet_buf, 0, remainBuffer, remainSize, packet_len - remainSize);

      int wr_index = getCurBufFIWr();
      arrayCopy(remainBuffer, 0, FRAME_BUFFER[wr_index], 0, remainSize);
      arrayCopy(packet_buf, 0, FRAME_BUFFER[wr_index], remainSize, packet_len - remainSize);
      IncFIWr();
      //wr_index = getPrevBufFIWr();
      //println("wr index = " + wr_index);

      num_of_packet++;
      offset = packet_len - remainSize;
      remainSize = 0; // initialize...
      println("remained is assembled... " +  offset);
    }
  }
  
  while(true) {
    if( (HtoI(packet_buf[offset]) == 0xFF) && (HtoI(packet_buf[offset + packet_len - 1]) == 0xFE) ) {
      arrayCopy(packet_buf, offset, FRAME_BUFFER[getCurBufFIWr()], 0, packet_len);
      IncFIWr();
      
      num_of_packet++;
      offset += packet_len;

      //int wr_index = getPrevBufFIWr();
      //println("wr index = " + wr_index);
    }
    else{
      offset++;
    }
    
    remainSize = read_len - offset; 
    if(remainSize < packet_len) {
      arrayCopy(packet_buf, offset, remainBuffer, 0, remainSize);
      //println("collect remain size ... " +  remainSize);
      break;
    }
  }
  
  println("found = " + num_of_packet);
  
  int wr_last = getPrevBufFIWr();  
  //println("wr last = " + getPrevBufFIWr());
  //  LOG
  {
    if( (HtoI(FRAME_BUFFER[wr_last][0]) != 0xFF) || (HtoI(FRAME_BUFFER[wr_last][1]) != 0xFD) ) {
      println("packet broken " + read_len + " [0]" + HtoI(FRAME_BUFFER[wr_last][0]) + " [1]" + HtoI(FRAME_BUFFER[wr_last][1]) + " [2]" + HtoI(FRAME_BUFFER[wr_last][2]));
      return;
    }
    
     
    int rx_count = (HtoI(FRAME_BUFFER[wr_last][2]) - 1) * 10000 + (HtoI(FRAME_BUFFER[wr_last][3])-1) * 100 + HtoI(FRAME_BUFFER[wr_last][4]) - 1;
    good_count += num_of_packet;
  
    println("good: " + good_count + ", wr index:" + frameIndexWr + " of " + rx_count + ", dif = " + (rx_count - good_count));
  }
  
  byte fill_n = 0;  
  Arrays.fill(packet_buf, 0, read_len, fill_n);

  if (mousePressed == true) {
    // Draw our line
    stroke(255);
    line(pmouseX, pmouseY, mouseX, mouseY);
    // Send mouse coords to other person
    println("cl --> server");
    tcp_c.write(pmouseX + " " + pmouseY + " " + mouseX + " " + mouseY + "\n");
  }
  
  loop_count++;
}

void keyPressed() {
  println("key pressed");
  mqttClient.publish(mqtt_topic_username, "Kim minjae");
}
