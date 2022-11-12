#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <stdio.h>
#include <ArduinoJson.h> //configuratore: https://arduinojson.org/v6/assistant/#/step1
#include "StringSplitter.h"
#include <Adafruit_NeoPixel.h>
#include "raicert2.h"
#include <U8g2lib.h> //dal library manager:U8g2  //https://github.com/olikraus/u8g2/wiki (deprecato)//https://www.az-delivery.de/en/products/1-3zoll-i2c-oled-display
#include "Utilities.h"
#include "frames.h"

News* dynamicArrayNews;

/********* ISTANZA CLASSE DISPLAY I2C SH1106 128X64 ************/
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
BitmapAnimation santa_claus_anim = BitmapAnimation(display_height_, display_width_, frame_height_, frame_width_, animation_frames, 0, 15, 100, 5, M_PI/8, TipoAnimazione::oscillazione);
SnowGenerator snow = SnowGenerator(&u8g2); 

/********* ISTANZA WIFI e certificato per POST HTTPS ************/
WiFiClient client;
const char* ssid = "FRITZ!Box 7530 JB";                          
const char* password = "psw";
X509List cert(cert_DigiCert_Global_Root_CA);

/********* INIZIALIZZA TIMER E POLLING TIME ************/
TimerC* TimeInState = new TimerC();
TimerC* TimerNuovaNewsADisplay = new TimerC();

/********* LEDS NEO-PIXELS ************/
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, 14, NEO_GRB + NEO_KHZ800); //24 LEDS, D5

//Inizializzo la classe per i giochi led quando è il giorno di natale 25 dic
GiochiLeds giochi_led = GiochiLeds(&strip);

/********* VARIABILI GLOBALI ************/
bool first_scan=true;
int count_post_ok=0;
bool ok_dt=false;
bool ok_accaddeoggi=false;
int state=0, state_old=0;
String dt="", mese="0", giorno="0";
int i_giorno=0, i_giorno_old=0;
uint16_t id_wheel=0; //[0-255]
int idx_news=0;
//STRINGE PER HTTP GET E POST
const String GET_DATA_ORA_ATTUALE = "http://worldclockapi.com/api/json/cet/now";
const String POST_ACCADDE_OGGI = "https://www.raicultura.it/atomatic/rai-search-service/api/v2/raicultura/archive";
uint8_t NumeroNotizie =0;
bool visualize_splash_screen=true;

/********* FUNZIONI GLOBALI PER GESTIONE DATA-FEEDS ************/

// ************************************************************
bool PostAccaddeOggi(const String& mese, const String& giorno, uint8_t& numero_notizie)
{
  //WiFiClient client;
  //Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  //Serial.printf("Using certificate: %s\n", cert_DigiCert_Global_Root_CA); 
  HTTPClient http;
  bool ok_begin = http.begin(client, POST_ACCADDE_OGGI);
  //Serial.println("POST BEGIN: " + String(ok_begin));
  String anno_mese_giorno = String("*-"+mese+"-"+giorno);
  // If you need an HTTP request with a content type: application/json, use the following:
  http.addHeader("Content-Type", "application/json");
  String payload_post = String("{\"page\":0,\"pageSize\":12,\"filters\":{\"types\":[\"accadde oggi\"],\"date\":\""+anno_mese_giorno+"\"},\"param\":\"null\",\"post_filters\":{}}");
  //Serial.println(payload_post);
  int httpResponseCode = http.POST(payload_post);
  if (httpResponseCode>0) 
  {
    //Serial.print("HTTP POST Response code: ");
    //Serial.println(httpResponseCode);
    String payload = http.getString();
    //Serial.println(payload);

    //DESERIALIZZA JSON:
    DynamicJsonDocument doc(4096);

    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("POST deserializeJson() failed: "));
      Serial.println(error.f_str());
      http.end();
      return false;
    }

    int total = doc["total"]; // 4 dati storici accaduti oggi
    numero_notizie = total;
    if(dynamicArrayNews != nullptr) 
    {
      delete[] dynamicArrayNews;
    }   
    dynamicArrayNews = new News[total];
    if(dynamicArrayNews == nullptr) {
        Serial.println(F("POST Dynamic array failure"));
        http.end();
        return false;
    }
    int id_attuale=1;
    Serial.print("\nN."+String(total));Serial.print(" EVENTI ACCADUTI IL: ");Serial.println(giorno+"/"+mese);
    for (JsonObject hit : doc["hits"].as<JsonArray>()) {

      //const char* hit_id = hit["id"]; // "ContentItem-2ea6d0f6-0a15-4b30-acd1-7fe0b4049610", ...
      const char* hit_title = hit["title"]; // "Elezioni democratiche in Argentina", "Perugia-Juventus: Renato ...
      const char* hit_subtitle = hit["subtitle"]; // "Dopo la dittatura vince Alfonsin", "Lo stadio porterà il ...
      const char* hit_data_pubblicazione = hit["data_pubblicazione"]; // "1983-10-30", "1977-10-30", ...
      //const char* hit_friendlyType = hit["friendlyType"]; // "ACCADDE OGGI", "ACCADDE OGGI", "ACCADDE OGGI", ...
      const char* hit_themes_0_label = hit["themes"][0]["label"]; // "Storia", "Storia", "Storia", "Storia"
      Serial.println("--- "+String(id_attuale)+"/"+String(total)+")");
      Serial.print("\t Titolo: ");Serial.println(hit_title);
      Serial.print("\t SottoTitolo: ");Serial.println(hit_subtitle);
      Serial.print("\t Pubblicazione: ");Serial.println(hit_data_pubblicazione);
      Serial.print("\t Tema: ");Serial.println(hit_themes_0_label);
      Serial.println("\n====================\n");

      dynamicArrayNews[id_attuale-1].Titolo = hit_title;
      dynamicArrayNews[id_attuale-1].SottoTitolo = hit_subtitle;
      dynamicArrayNews[id_attuale-1].Pubblicazione = hit_data_pubblicazione;
      dynamicArrayNews[id_attuale-1].Tema = hit_themes_0_label;

      ++id_attuale;
    }
    // Free resources
    http.end();
    return true;
  }
  else {
    Serial.print("POST Error code: ");
    Serial.print(httpResponseCode);
    Serial.print(" ");
    Serial.println(HTTPClient::errorToString(httpResponseCode));
    // Free resources
    http.end();
    return false;
  }


}
// ************************************************************
bool GetCurrentDateTime(String& dt, String& mese, String& giorno)
{
  //ESECUZIONE DELLA GET PER OTTENERE DATA-ORA ATTUALI
  WiFiClient client;
  HTTPClient http;
  http.begin(client, GET_DATA_ORA_ATTUALE );

  // Send HTTP GET request
  int httpResponseCode = http.GET();
  if (httpResponseCode>0) {
    //Serial.print("HTTP GET Response code: ");
    //Serial.println(httpResponseCode);
    String payload = http.getString();
    //Serial.println(payload);

    //DESERIALIZZA JSON:
    StaticJsonDocument<384> doc;

    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("GET deserializeJson() failed: "));
      Serial.println(error.f_str());
      http.end();
      return false;
    }

    //const char* id = doc["$id"]; // "1"
    const char* currentDateTime = doc["currentDateTime"]; // "2022-10-31T19:52+01:00"
    dt = String(currentDateTime);
    StringSplitter *splitter1 = new StringSplitter(dt, 'T', 3);
    String data = splitter1->getItemAtIndex(0);
    splitter1 = new StringSplitter(data, '-', 3);
    mese = splitter1->getItemAtIndex(1);
    giorno = splitter1->getItemAtIndex(2);
    //Serial.print("mese: ");Serial.println(mese);
    //Serial.print("giorno: ");Serial.println(giorno);
    //const char* utcOffset = doc["utcOffset"]; // "01:00:00"
    //bool isDayLightSavingsTime = doc["isDayLightSavingsTime"]; // false
    //const char* dayOfTheWeek = doc["dayOfTheWeek"]; // "Monday"
    //const char* timeZoneName = doc["timeZoneName"]; // "Central Europe Standard Time"
    //long long currentFileTime = doc["currentFileTime"]; // 133117195696534140
    //const char* ordinalDate = doc["ordinalDate"]; // "2022-304"

    // Free resources
    http.end();
    return true;

  }
  else {
    Serial.print("GET Error code: ");
    Serial.print(httpResponseCode);
    Serial.print(" ");
    Serial.println(HTTPClient::errorToString(httpResponseCode));
    // Free resources
    http.end();
    return false;
  }

}

void ConnectWiFi()
{
  //CONNESSIONE AL WIFI
  WiFi.begin(ssid, password);
  //TENTATIVI DI CONNESSIONE
  int counter = 0;
  while (!WiFi.isConnected()) 
  {
    delay(200);    
    if (++counter > 100)
    {
      //SE NON SI CONNETTE DOPO 100 TENTATIVI, RESETTA ESP 
      ESP.restart();
    }
     
  }
  Serial.println("\nWiFi Connesso");
}

/*************** INIZIO PROGRAMMA ARDUINO ************/

void setup() {

  Serial.begin(115200);

  u8g2.begin();
  u8g2.setBitmapMode(1); //set the background to transparant
  
  ConnectWiFi();
  WiFi.setAutoReconnect(true);

  //SETUP LEDS NEOPIXELS
  strip.begin();
  strip.setBrightness(75);
  strip.show(); // Initialize all pixels to 'off'

  //AVVIO TIMER PER HTTP GET-POST
  TimeInState->start();
  TimerNuovaNewsADisplay->start();
}

void loop() 
{

  if(visualize_splash_screen)
  {
    u8g2.clearBuffer();
    u8g2.drawXBMP(32,
                  0, 
                  logo_ninf_width, 
                  logo_ninf_height, 
                  logo_ninf_bits);
    u8g2.sendBuffer();
    visualize_splash_screen=false;
  }

  switch (state) 
  {
    case 0:
    {
      state=100;
      break;
    }
    case 100:
    {
      if (WiFi.isConnected())
      {
        state=200;
      }
      break;
    }
    case 200:
    {
      ok_dt = GetCurrentDateTime(dt,mese,giorno);
      if(ok_dt)
      {
        first_scan=false;
        state=300;
      }
      else
      {
        //errore GET
        state=225;      
      }
      break;
    }
    case 225:
    {
      if(first_scan)
      {
        //se alla prima accensione la GET fallisce,
        //aspetto solo 500ms per eseguire nuova GET
        if(TimeInState->getET()>500)
        {
          state=100;
        }
      }
      else
      {
        //se la GET fallisce ma ho gia eseguito una GET con successo,
        //aspetto un po' di piu prima (2000ms) prima di eseguire nuova GET
        if(TimeInState->getET()>2000)
        {
          state=100;
        }
      }     
      break;
    }
    case 300:
    {
      i_giorno = giorno.toInt(); 
      if (i_giorno != i_giorno_old)
      {
        //scoccato nuovo giorno
        state=400;
      }
      else
      {
        //ancora lo stesso giorno
        //aspetto 10 secondi
        //e vado a fare una altra GET per sapere data-ora        
        state=325;
      }
      i_giorno_old = i_giorno;
      break;
    }
    case 325:
    {
      //ancora lo stesso giorno
      //aspetto 10 secondi
      //e vado a fare una altra GET per sapere data-ora 
      if(TimeInState->getET()>10000)
      {
        state=100;
      }
      break;
    }
    case 400:
    {
      ok_accaddeoggi = PostAccaddeOggi(mese, giorno, NumeroNotizie);
      if(ok_accaddeoggi)
      {
        //aggiorna dati su display
        state=500;
      }
      else
      {
        //errore POST
        //riprova a fare la POST
        state=450;
      }
      break;
    }
    case 450:
    {
      //aspetto 2sec prima di fare nuova POST dopo l'errore
      if(TimeInState->getET()>2000)
      {
        if (WiFi.isConnected())
        {
          //esegui di nuovo la POST
          state=400;
        }
      }     
      break;
    }
    case 500:
    {
      //aggiorna dati su display
      ++count_post_ok;
      //per evitare overflow
      if(count_post_ok>100)
        count_post_ok=2;
      //ricomincia da capo
      state=100;
      break;
    }

  }//fine switch

  //resetta il timer al cambiamento di stato
  //questo ci permette di sapere da quanto tempo si è in quel determinato stato
  if(state!=state_old)
  {
     TimeInState->reset();
  }
  state_old=state;

  if(ok_accaddeoggi)
  {
    //ogni 10 secondi printa una nuova news
    if(TimerNuovaNewsADisplay->getET()>10000 || count_post_ok==1)
    {

      //prima eseguo animazione santa claus
      //facciamo animazione bloccante
      do
      {
        santa_claus_anim.Animate(true);
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB14_tr);
        u8g2.drawStr(4,16,"Happy Xmas");
        snow.Animate(true);
        
        u8g2.drawXBMP(santa_claus_anim.X,
                      santa_claus_anim.Y, 
                      santa_claus_anim.frame_width, 
                      santa_claus_anim.frame_height, 
                      list_of_frames[santa_claus_anim.current_frame]);
        u8g2.sendBuffer();
        delay(10);
      }while(!santa_claus_anim.done);
      
      TimerNuovaNewsADisplay->reset();
      ++count_post_ok;
      if(dynamicArrayNews!=nullptr && NumeroNotizie>0)
      {             
        u8g2.clearBuffer();					// clear the internal memory
        //FONTS: https://github.com/olikraus/u8g2/wiki/fntlist8#8-pixel-height
        /*** TITOLO ***/
        u8g2.setFont(u8g2_font_ncenB08_tr);	
        u8g2_uint_t sw = u8g2.getStrWidth(dynamicArrayNews[idx_news].Titolo);
        uint8_t ypos=10;
        if(sw<u8g2.getDisplayWidth()-2)
        {
          u8g2.drawStr(0,ypos,dynamicArrayNews[idx_news].Titolo);
        }
        else
        {
          //faccio 3 righe (considero che 3 righe sono sempre sufficienti)
          String Titolo = String(dynamicArrayNews[idx_news].Titolo);
          for (int i=Titolo.length()-1; i>1; --i) {
            sw = u8g2.getStrWidth(Titolo.substring(0,i).c_str());
            if(sw<u8g2.getDisplayWidth()-2)
            {
              //prima riga
              u8g2.drawStr(0,ypos,Titolo.substring(0,i).c_str());
              //seconda riga
              String seconda_riga=Titolo.substring(i);
              //vedo quanto è lunga la seconda riga
              sw = u8g2.getStrWidth(seconda_riga.c_str());
              //se entra ok
              if(sw<u8g2.getDisplayWidth()-2)
              {
                ypos += 8+3;
                u8g2.drawStr(0,ypos,seconda_riga.c_str());//da i fino alla fine della stringa
              }
              else
              {
                //altrimenti faccio una terza riga
                for (int i2=seconda_riga.length()-1; i2>1; --i2) 
                {
                    sw = u8g2.getStrWidth(seconda_riga.substring(0,i2).c_str());
                    if(sw<u8g2.getDisplayWidth()-2)
                    {
                      //seconda riga
                      ypos += 8+3;
                      u8g2.drawStr(0,ypos,seconda_riga.substring(0,i2).c_str());
                      //terza riga
                      ypos += 8+3;
                      u8g2.drawStr(0,ypos,seconda_riga.substring(i2).c_str()); //da i fino alla fine della stringa
                      break;
                    }
                }
              }
              
              break;
            }
          }
          
        }
        
        /*** SOTTOTITOLO ***/       
        u8g2.setFont(u8g2_font_ncenB08_tr);
        //
        sw = u8g2.getStrWidth(dynamicArrayNews[idx_news].SottoTitolo);
        if(sw<u8g2.getDisplayWidth()-2)
        {
          ypos += 8+5;
          u8g2.drawStr(0,ypos,dynamicArrayNews[idx_news].SottoTitolo);
        }
        else
        {
          //faccio 3 righe (considero che 3 righe sono sempre sufficienti)
          String SottoTitolo = String(dynamicArrayNews[idx_news].SottoTitolo);
          for (int i=SottoTitolo.length()-1; i>1; --i) {
            sw = u8g2.getStrWidth(SottoTitolo.substring(0,i).c_str());
            if(sw<u8g2.getDisplayWidth()-2)
            {
              //prima riga
              ypos += 8+5;
              u8g2.drawStr(0,ypos,SottoTitolo.substring(0,i).c_str());
              //seconda riga
              String seconda_riga=SottoTitolo.substring(i);
              //vedo quanto è lunga la seconda riga
              sw = u8g2.getStrWidth(seconda_riga.c_str());
              //se entra ok
              if(sw<u8g2.getDisplayWidth()-2)
              {
                ypos += 8+3;
                u8g2.drawStr(0,ypos,seconda_riga.c_str());
              }
              else
              {
                //altrimenti faccio una terza riga
                for (int i2=seconda_riga.length()-1; i2>1; --i2) 
                {
                    sw = u8g2.getStrWidth(seconda_riga.substring(0,i2).c_str());
                    if(sw<u8g2.getDisplayWidth()-2)
                    {
                      //seconda riga
                      ypos += 8+3;
                      u8g2.drawStr(0,ypos,seconda_riga.substring(0,i2).c_str());
                      //terza riga
                      ypos += 8+3;
                      u8g2.drawStr(0,ypos,seconda_riga.substring(i2).c_str()); //da i fino alla fine della stringa
                      break;
                    }
                }
              }
              break;
            }
          }
        }
              
        /*** PUBBLICAZIONE ***/
        ypos += 8+5;
        u8g2.setFont(u8g2_font_ncenB08_tr);	
        u8g2.drawStr(0,ypos,dynamicArrayNews[idx_news].Pubblicazione);
        /*** TEMA ***/
        //u8g2.setFont(u8g2_font_ncenB08_tr);	
        //u8g2.drawStr(0,10+8+5+8+5+8+5,dynamicArrayNews[idx_news].Tema);
        u8g2.sendBuffer();					// transfer internal memory to the display

        ++idx_news;
        if(idx_news>=NumeroNotizie)
        {
          idx_news=0;
        }
        /*
        Serial.print(F("sizeof(dynamicArrayNews[0]): "));Serial.println(sizeof(dynamicArrayNews[0]));
        Serial.print(F("sizeof(dynamicArrayNews): "));Serial.println(sizeof(dynamicArrayNews));
        Serial.print(F("LengthOfArray: "));Serial.println(LengthOfArray);
        Serial.print(F("idx_news: "));Serial.println(idx_news);
        */
      }
    
    }
  }

  /***** SOLO PER TEST *****/
  //decommentare per falsificare il giorno attuale durante i tests
  //commentare se si vuole usare il giorno attuale reale ottenuto dalla GET
  i_giorno = 25;

  //if (i_giorno==0 || (!ok_dt && first_scan) )
  if (i_giorno==0 || !ok_dt )
  {    
    //errore
    strip.clear();
    giochi_led.colorWipe2(strip.Color(255, 0, 0), 100, 0); //ROSSO
  }
  else if(i_giorno>24)
  {
    //eseguo un gioco di colori su tutti i leds
    strip.clear();
    //colorContemporaneamente(strip.Color(0, 0, 255), 100, 0); // BLUE
    giochi_led.Play();
  }
  else if(i_giorno==1)
  {
    //faccio il rainbow sul giorno corrente i_giorno -> idled = i_giorno-1 = 0
    strip.clear();
    giochi_led.rainbow2(0, id_wheel, 20);
  }
  else //da 2 a 24
  {
    strip.clear();
    //e.g., i_giorno=10 -> led da 0 a 8 == blu
    //giochi_led.block_painting(strip.Color(0, 0, 255), 5, i_giorno-2); // Blue i_giorno=10-> id_led=9 se escludo poi il giorno attuale devo togliere 2
   
    //faccio il rainbow sul giorno corrente i_giorno -> idled = i_giorno-1
    giochi_led.rainbow2(i_giorno-1, id_wheel, 20);  
     giochi_led.rainbowModified(10,i_giorno-1);//idled = i_giorno-1 non compreso
  }

}
