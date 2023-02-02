#include <time.h> 
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SPIFFS.h"

const char* ssid     = "TMNT";
const char* password = "20Darko08";

const char* serverName = "http://boki.in.rs/getTagPost.php";
//unsigned long lastTime = 0;
//unsigned long timerDelay = 30000;

long timezone = 1; 
byte daysavetime = 1;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.print (file.name());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.print(file.size());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

String readFileAndCreateJSON(fs::FS &fs, const char * path){
    String strJson;
    String dataRead;
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return "";
    }

    Serial.println("Read from file: ");
    while(file.available()){
        //dataRead = file.read();
        //Serial.println(dataRead);
        dataRead =file.readStringUntil('\n');
        strJson = strJson +  "{\"tagId\":\""+dataRead+"\"},";
    }
    file.close();

    Serial.println("strJson:");
    Serial.println(strJson);
        
    // brisanje poslednjeg zareza 
    strJson[strJson.length()-1] = ' ';
    Serial.println("strJson bez zareza:");
    Serial.println(strJson);
    
    
    return noviString;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void setup(){
    Serial.begin(115200);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Contacting Time Server");
  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");

  if(!SPIFFS.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  for (int i=0;i<100;i++){  
    String dataForFile = getDateTimeCoded() + "#" + String(esp_random()) + "#\n";
    appendFile(SPIFFS, "/tagdatafile.txt", dataForFile.c_str());
  }
  Serial.println("Data written to file");
  Serial.println("Reading file");
  readFile(SPIFFS, "/tagdatafile.txt");
  Serial.println("Reading FINISHED");
  sendRequest();
  deleteFile(SPIFFS, "/tagdatafile.txt");
  Serial.println("File DELETED");

  listDir(SPIFFS, "/", 0);
  
}

void sendRequest(){
if(WiFi.status()== WL_CONNECTED){
      Serial.println("sending request");
      WiFiClient client;
      HTTPClient http;
    
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
      
      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
      
      // Specify content-type header
      //http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&value1=24.25&value2=49.54&value3=1005.14";           
      // Send HTTP POST request
      //int httpResponseCode = http.POST(httpRequestData);
      
      // If you need an HTTP request with a content type: application/json, use the following:
      http.addHeader("Content-Type", "application/json");
      //String httpRequestData = "{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}"
      //String httpRequestData = "[{\"tag_id\":\"230112121301#2900940E97#\"},{\"tag_id\":\"230112121402#2900940E96#\"},{\"tag_id\":\"230112121503#2900940E95#\"}]";
      String httpRequestData = readFileAndCreateJSON(SPIFFS, "/tagdatafile.txt");;
      int httpResponseCode = http.POST(httpRequestData);

      // If you need an HTTP request with a content type: text/plain
      //http.addHeader("Content-Type", "text/plain");
      //int httpResponseCode = http.POST("Hello, World!");
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}

String getDateTimeCoded(){
  struct tm tmstruct ;
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  String timeCoded = String(tmstruct.tm_year - 100) + 
                      addLeadingZero(String(( tmstruct.tm_mon)+1 )) + 
                      addLeadingZero(String(tmstruct.tm_mday)) +
                      addLeadingZero(String(tmstruct.tm_hour)) +
                      addLeadingZero(String(tmstruct.tm_min)) +
                      addLeadingZero(String(tmstruct.tm_sec)) ;
  
  return timeCoded;
}

String addLeadingZero(String inpString){
  String retString;
  if (inpString.length() == 1)
    retString =  "0"+inpString;
  else
    retString = inpString;
  return retString;
}

void loop(){
}
