#include <IRremote.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <arpa/inet.h>

static int taskCore = 0;
static int taskCore0 = 1;
int RECV_PIN = 14;
int ledPin  = 2;

const char* ssid      = "";
const char* password  = "";
char  *proprietario   = "";
char  *idSensor       = "gpaS0000001";
char *globalIp        = "";

SemaphoreHandle_t xMutex;

IRrecv irrecv(RECV_PIN);

decode_results results;

void coreTask(void *pvParameters){

    irrecv.enableIRIn();

    String taskMessage = "Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();

    Serial.println(taskMessage);
 
    while(true){
      
        if(verificarIR()){

        //  Serial.println("V�lido");
       }

      irrecv.resume();
  
      delay(100);
    }
  
}

void coreTaskWiFi(void *pvParameters){

    String taskMessage = "TaskWifi running on core ";
    taskMessage = taskMessage + xPortGetCoreID();

    Serial.println(taskMessage);

    String element;
 
    while(true){
      
      if(WiFi.status() == WL_CONNECTED){;

        xSemaphoreTake(xMutex,portMAX_DELAY);

        if(globalIp != ""){
          Serial.print("Global:  ");
          Serial.println(globalIp);
          globalIp = "";
        }
    
        xSemaphoreGive(xMutex);
        
        delay(2000 - piscarLed(2));
        
      }else{

        delay(5000 - piscarLed(6));
        
      }
     
    }
  
}

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  delay(100);

  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
  }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

  delay(100);

  xMutex = xSemaphoreCreateMutex();

  delay(100);
 
  xTaskCreatePinnedToCore(
                    coreTask,   /* Function to implement the task */
                    "coreTask", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    taskCore);  /* Core where th*/

  delay(100);

  xTaskCreatePinnedToCore(
                    coreTaskWiFi,   /* Function to implement the task */
                    "coreTasWiFi", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    taskCore0);  /* Core where th*/

  delay(100);
 
  pinMode(ledPin, OUTPUT);

}

void loop() {


  delay(1000);

}

 boolean verificarIR(){

  if (irrecv.decode(&results)) {
    
    if(results.bits == 32){

      struct in_addr addr;
      
      addr.s_addr = htonl(results.value); // s_addr must be in network byte order 
      
      char *s = inet_ntoa(addr); // --> "10.1.2.3"

      char charBuf[WiFi.subnetMask().toString().length()+1];

      WiFi.subnetMask().toString().toCharArray(charBuf, WiFi.subnetMask().toString().length()+1);

       if(testIp(s, "192.168.0.0",charBuf , true) == true){
        
          return true;
          
       }else{
        
          return false;
       
       }

    }
    
  }
  
  return false;
}

int piscarLed(int vezes){

  for(int i = 0; i < vezes; i++ ){

    digitalWrite(ledPin, HIGH);

    delay(300);

    digitalWrite(ledPin, LOW);

    delay(300);
        
  }

  return vezes * 300;

}

boolean sendMessage(char *ip, HTTPClient http){

  
  http.begin("http://" + *ip);

  int httpCode = http.GET();

  if(httpCode > 0){

    if(httpCode == HTTP_CODE_OK){

      return true;
      
    }else{

      return false;
      
    }
    
  }else{

    return false;
    
  }

  http.end();
  
}

String dados(char *proprietarioRemoto){

  char payload[1024];

  sprintf(payload, "values[ip]=%s&values[proprietario]=%s&values[proprietarioRemoto]", 
  WiFi.localIP(), proprietario, proprietarioRemoto);

  return payload;
}

uint32_t IPToUInt(const std::string ip) {
    int a, b, c, d;
    uint32_t addr = 0;

    if (sscanf(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
        return 0;

    addr = a << 24;
    addr |= b << 16;
    addr |= c << 8;
    addr |= d;
    return addr;
}

bool IsIPInRange(const std::string ip, const std::string network, const std::string mask) {
    uint32_t ip_addr = IPToUInt(ip);
    uint32_t network_addr = IPToUInt(network);
    uint32_t mask_addr = IPToUInt(mask);

    uint32_t net_lower = (network_addr & mask_addr);
    uint32_t net_upper = (net_lower | (~mask_addr));

    if (ip_addr >= net_lower &&
        ip_addr <= net_upper)
        return true;
    return false;
}

boolean testIp(char * ip, const std::string network, const std::string mask, bool expected) {
    if (IsIPInRange(ip, network, mask) != expected) {
     //   printf("Failed! %s %s %s %s\n", ip.c_str(), network.c_str(), mask.c_str(), expected ? "True" : "False");
        return false;
    } else {
     //   printf("Success! %s %s %s %s\n", ip.c_str(), network.c_str(), mask.c_str(), expected ? "True" : "False");

         xSemaphoreTake(xMutex,portMAX_DELAY);

        globalIp = ip;
    
        xSemaphoreGive(xMutex);
          
        return true;
    }
    return false;
}
