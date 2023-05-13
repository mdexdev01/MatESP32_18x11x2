import mqtt.*;
MQTTClient mqttClient;

String mqtt_topic_server = "/xedm01/grib/server";
String mqtt_topic_username = "/xedm01/grib/user";

boolean setup_mqtt_shiftr() {
  mqttClient = new MQTTClient(this);
  
  int trial_max = 10;
  
  println("trying mqtt...");

  while(trial_max != 0) {
    try{
      mqttClient.connect("mqtt://public:public@public.cloud.shiftr.io", "processing");
      break;
    } catch (Exception e) {
      println("mqtt e)" + e);
      trial_max--;
    }
  }
  
  if(trial_max == 0)  {
    println("mqtt FAIL");
    return false;
  }

  println("mqtt connected");
  
  return true;  
}



void clientConnected() {
  println("mqttClient connected");

  mqttClient.subscribe(mqtt_topic_server);
}

String tcp_server_ip = "";
int tcp_server_port = 0;
boolean mqtt_tcp_server_info_updated = false;

void setTcpServerIP(String ip) {
  tcp_server_ip = ip;
}

void setTcpServerPort(int port) {
  tcp_server_port = port;
}

String getTcpServerIP() {
  return tcp_server_ip;
}

int getTcpServerPort() {
  return tcp_server_port;
}


void messageReceived(String topic, byte[] payload) {
  println("new message: " + topic + " - " + new String(payload));

  {
    //input = "[server]ip : 172.3.5.87 , port : 380\r\n";
    input =  new String(payload);

    try{
      input = input.substring(0, input.indexOf("\r"));
    } catch (Exception e) {
      println("mqtt e)" + e);
    }
    println(input);
    
    String tag = input.substring(0, input.indexOf("]") + 1);
    if( tag.equals("[server]") ) {
      println("tag:" + tag);
    }

    input = input.substring(input.indexOf("]") + 1, input.length());
    
    String [] messages = splitTokens(input, ",");
    
    String [] key_value;
    key_value = trim(splitTokens(messages[0], ":"));
    if(key_value[0].equals("ip")) {
      tcp_server_ip = key_value[1]; 
    }
    else {
      println("***** wrong server ip" + key_value[1]);
      return;
    }
    
    key_value = trim(splitTokens(messages[1], ":"));
    if(key_value[0].equals("port")) {
      tcp_server_port = int(key_value[1]); 
    }
    else {
      println("***** wrong server port" + key_value[1]);
      return;
    }

    println("tcp_server_ip:" + tcp_server_ip + ", tcp_server_port:" + tcp_server_port);
    
    setTcpServerIP(tcp_server_ip);
    setTcpServerPort(tcp_server_port);
    mqtt_tcp_server_info_updated = true;
  }
  //mqttClient.unsubscribe("/xedm01");
  mqttClient.unsubscribe(mqtt_topic_server);


}

void connectionLost() {
  println("connection lost");
}
