#include <SPI.h>
#include <Ethernet.h>

const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const char server[] = "submit.vortexit.co.nz";
const char databaseHash[] = "c81e728d";    // Database Hash you need to register @ portal.vortexit.co.nz

IPAddress ip(192, 168, 0, 177); // Set the static IP address to use if the DHCP fails to get assign
EthernetClient client;

// readPage Variables
char serverResult[32]; // string for incoming serial data
int stringPosition = 0; // string index counter readPage()
bool startRead = false; // is reading? readPage()

//--------------------------------------------------------------------------

void setup() 
{
  Serial.begin(9600);
  if (Ethernet.begin(mac) == 0) {
    // Try to congifure using a static IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
}

void loop()
{
  char ethIP[16];
  char serialOutput1[25];
  IPAddress ethIp = Ethernet.localIP();
  sprintf(serialOutput1, "IP:%d.%d.%d.%d%-20s", ethIp[0], ethIp[1], ethIp[2], ethIp[3], " ");
  Serial.println(serialOutput1);
  float checkConnectionResult = checkConnection();
  switch ((int) checkConnectionResult) 
    {
    case 0:
       Serial.println("Connected Good");
            break;  
    case 2:
       Serial.println("Time Out Connection to Server");  
            break;  
    case 7: 
       Serial.println("User Hash Not Found in DB");    
            break;  
    case 8: 
       Serial.println("No Hash Input");    
            break;        
  }
  Serial.println("-------------------------------------");
  delay(10000);
}

byte checkConnection()
{
  if (client.connect(server, 80)) 
  {
    client.print("GET /check_connection.php?");
    client.print("DatabaseHash=");
    client.print(databaseHash); 
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
    client.println();
    return readPage();
  } else {
    return 1;
  }
}

float readPage()
{
  stringPosition = 0;
  unsigned long startTime = millis(); 
  memset( &serverResult, 0, 32 ); //Clear serverResult memory
  while(true)
  {
    if (millis() < startTime + 10000) // Time out of 10000 milliseconds (Possible cause of crash on Ethernet)
    {
      if (client.available())
      {
        char c = client.read();
        if (c == '<' )  //'<' Start character
        {
          startRead = true; //Ready to start reading the part 
        } else if (startRead) 
        {
          if(c != '>') //'>' End character
          {
            serverResult[stringPosition] = c;
            stringPosition ++;
          } else {
            startRead = false;
            client.stop();
            client.flush();        
            if(strcmp(serverResult, "SUCCESSFUL") == 0) return 0; //SUCCESSFUL
            if(strcmp(serverResult, "ERROR_DATABASE") == 0) return 3; //ERROR_DATABASE
            if(strcmp(serverResult, "ERROR_MISSING_DATA") == 0) return 4; //ERROR_MISSING_DATA
            if(strcmp(serverResult, "ERROR_NO_BARCODE_DB") == 0) return 5; //ERROR_NO_BARCODE_DB
            if(strcmp(serverResult, "ERROR_NO_BARCODE_INPUT") == 0) return 6; //ERROR_NO_BARCODE_INPUT
            if(strcmp(serverResult, "ERROR_DATABASE_HASH_INPUT") == 0) return 7; //ERROR_DATABASE_HASH_INPUT
            if(strcmp(serverResult, "ERROR_HASH_INPUT") == 0) return 8; //ERROR_HASH_INPUT
          }
        }
      }
    } else {
      client.stop();
      client.flush();
      return 2; //TIMEOUT
    }
  }                                                                                                        
}
