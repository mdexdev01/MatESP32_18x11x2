import processing.net.*;
// import java.net.*;
// import java.io.*;
import java.net.InetAddress;
import java.net.UnknownHostException;


//	http://learningprocessing.com/examples/chp19/example-19-01-simple-server
//  https://github.com/Links2004/arduinoWebSockets


public String HostAddress;
public String HostName;

int now;


Server s;
Client c;
String input;
int data[];

int portNum = 1000;

void setup_WifiServer() 
{
//   ws= new WebsocketServer(this,portNum + 1,"/john");
  now=millis();

	try {
		s = new Server(this, portNum); // Start a simple server on a port
		//	if binding error : cmd > netstat -ano > taskkill/

		InetAddress addr = InetAddress.getLocalHost();

		// Get IP Address
		byte[] ipAddr = addr.getAddress();

		// Extract Just the IP. Vanilla addr returns name and address separated by a '/' character
		String raw_addr = addr.toString();
		String[] list = split(raw_addr,'/');
		HostAddress = list[1];

		// Get hostname
		HostName = addr.getHostName();
	} catch (UnknownHostException e) {
		println("Wifi e)" + e);
	}
	println(HostAddress + ":" + HostName + " Port : " + portNum);

	// draw_WifiServer();

}


int loop_send = 0;
void draw_WifiServer() 
{
	c = s.available();
	if (c != null) {
		input = c.readString();
		println(input);
	}

	if( now + 2000 < millis() ){
		// ws.sendMessage("Server message");
		s.write("Hi client, server is alive");
		now = millis();
		// println("send " + loop_send);
	}

	delay(50);

	loop_send++;
}

String incomingMessage = "";


// The serverEvent function is called whenever a new client connects.
void serverEvent(Server server, Client client) {
  incomingMessage = "A new client has connected:" + client.ip();
  println(incomingMessage);
}
