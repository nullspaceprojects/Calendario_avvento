#pragma once

#include <Adafruit_NeoPixel.h>
#include <U8g2lib.h>

#define display_width_ 128
#define display_height_ 64

/********* DEFINIZIONE CLASSE HELPER TIMER ************/
class TimerC {

  private:
        unsigned long _start_time;
        unsigned long _elapsed_time;
  
  public:
      TimerC()
      {
        _start_time = 0;
        _elapsed_time = 0;
      }

      void start()
      {
        if(_start_time != 0)
        {
          return;
        }
        _start_time = millis();
      }
      
      unsigned long getET()
      {
        //in millisecondi
        _elapsed_time = millis() - _start_time;
        return _elapsed_time;
      }
      double getETSec()
      {
        unsigned long et_milli =  getET();
        return et_milli/1000.0;
      }
      void reset()
      {
        _start_time = millis();
        _elapsed_time = 0;
      }
      void stop_()
      {
        if(_start_time == 0)
        {
          return;
        }
        _start_time = 0;
        _elapsed_time = 0;
        
      }
};

/********* DEFINIZIONE CLASSE HELPER NEWS CHE CONTIENE LE NEWS SCARICATE TRAMITE LA POST HTTPS ************/
class News
{
    public:
      const char* Titolo;
      const char* SottoTitolo;
      const char* Pubblicazione;
      const char* Tema;
      News(){}
};

/********* DEFINIZIONE CLASSE HELPER PER CREARE GIOCHI DI LUCI ************/
class GiochiLeds
{
  private:
    Adafruit_NeoPixel* _strip;
  public:
    GiochiLeds(Adafruit_NeoPixel* strip_)
    {
      _strip = strip_;
    }

    uint32_t Wheel(byte WheelPos) {
      WheelPos = 255 - WheelPos;
      if(WheelPos < 85) {
        return _strip->Color(255 - WheelPos * 3, 0, WheelPos * 3);
      }
      if(WheelPos < 170) {
        WheelPos -= 85;
        return _strip->Color(0, WheelPos * 3, 255 - WheelPos * 3);
      }
      WheelPos -= 170;
      return _strip->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }

    // Fill strip pixels one after another with a color. Strip is NOT cleared
    // first; anything there will be covered pixel by pixel. Pass in color
    // (as a single 'packed' 32-bit value, which you can get by calling
    // strip.Color(red, green, blue) as shown in the loop() function above),
    // and a delay time (in milliseconds) between pixels.
    void colorWipe(uint32_t color, int wait) {
      for(int i=0; i<_strip->numPixels(); i++) { // For each pixel in _strip->..
        _strip->setPixelColor(i, color);         //  Set pixel's color (in RAM)
        _strip->show();                          //  Update strip to match
        delay(wait);                           //  Pause for a moment
      }
    }
    void colorWipe2(uint32_t c, uint8_t wait, int until_led_id=0) {
      if (until_led_id==0)
      {
        until_led_id = _strip->numPixels()-1;
      }
      //until_led_id è compreso nel for perchè <=
      for(uint16_t i=0; i<=until_led_id; i++) {
        _strip->setPixelColor(i, c);
        _strip->show();
        delay(wait);
      }
    }

    // Theater-marquee-style chasing lights. Pass in a color (32-bit value,
    // a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
    // between frames.
    void theaterChase(uint32_t color, int wait) {
      for(int a=0; a<10; a++) {  // Repeat 10 times...
        for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
          _strip->clear();         //   Set all pixels in RAM to 0 (off)
          // 'c' counts up from 'b' to end of strip in steps of 3...
          for(int c=b; c<_strip->numPixels(); c += 3) {
            _strip->setPixelColor(c, color); // Set pixel 'c' to value 'color'
          }
          _strip->show(); // Update strip with new contents
          delay(wait);  // Pause for a moment
        }
      }
    }
    // Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
    void rainbow(int wait) {
      // Hue of first pixel runs 5 complete loops through the color wheel.
      // Color wheel has a range of 65536 but it's OK if we roll over, so
      // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
      // means we'll make 5*65536/256 = 1280 passes through this loop:
      for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
        // strip.rainbow() can take a single argument (first pixel hue) or
        // optionally a few extras: number of rainbow repetitions (default 1),
        // saturation and value (brightness) (both 0-255, similar to the
        // ColorHSV() function, default 255), and a true/false flag for whether
        // to apply gamma correction to provide 'truer' colors (default true).
        _strip->rainbow(firstPixelHue);
        // Above line is equivalent to:
        // _strip->rainbow(firstPixelHue, 1, 255, 255, true);
        _strip->show(); // Update strip with new contents
        delay(wait);  // Pause for a moment
      }
    }
    // Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
    void theaterChaseRainbow(int wait) {
      int firstPixelHue = 0;     // First pixel starts at red (hue 0)
      for(int a=0; a<30; a++) {  // Repeat 30 times...
        for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
          _strip->clear();         //   Set all pixels in RAM to 0 (off)
          // 'c' counts up from 'b' to end of strip in increments of 3...
          for(int c=b; c<_strip->numPixels(); c += 3) {
            // hue of pixel 'c' is offset by an amount to make one full
            // revolution of the color wheel (range 65536) along the length
            // of the strip (_strip->numPixels() steps):
            int      hue   = firstPixelHue + c * 65536L / _strip->numPixels();
            uint32_t color = _strip->gamma32(_strip->ColorHSV(hue)); // hue -> RGB
            _strip->setPixelColor(c, color); // Set pixel 'c' to value 'color'
          }
          _strip->show();                // Update strip with new contents
          delay(wait);                 // Pause for a moment
          firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
        }
      }
    }
    void rainbow2(uint16_t id_pixel, uint16_t& j, uint8_t wait) {
      _strip->setPixelColor(id_pixel, Wheel(j & 255));
      _strip->show();
      ++j;
      if(j>255)
        j=0; 
      if (wait>0)
      {
        delay(wait);
      }
    }
    void block_painting(uint32_t c, uint8_t wait, int until_led_id=0) {
      if (until_led_id==0)
      {
        until_led_id = _strip->numPixels()-1;
      }
      //until_led_id è compreso nel for perchè <=
      for(uint16_t i=0; i<=until_led_id; i++) {
        _strip->setPixelColor(i, c);
        //_strip->show();
        //delay(wait);
      }
      _strip->show();
      delay(wait);
    }
    void rainbow3(uint16_t first_hue, int8_t reps,uint8_t saturation, uint8_t brightness, bool gammify, uint16_t numLEDs)
    {
      for (uint16_t i=0; i<numLEDs; i++) 
      {
        uint16_t hue = first_hue + (i * reps * 65536) / numLEDs;
        uint32_t color = _strip->ColorHSV(hue, saturation, brightness);
        if (gammify) color = _strip->gamma32(color);
        _strip->setPixelColor(i, color);
      }
    }
    void rainbowModified(int wait, uint16_t until_led) //until_led id led non compreso
    {
      // Hue of first pixel runs 5 complete loops through the color wheel.
      // Color wheel has a range of 65536 but it's OK if we roll over, so
      // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
      // means we'll make 5*65536/256 = 1280 passes through this loop:
      for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
        // strip.rainbow() can take a single argument (first pixel hue) or
        // optionally a few extras: number of rainbow repetitions (default 1),
        // saturation and value (brightness) (both 0-255, similar to the
        // ColorHSV() function, default 255), and a true/false flag for whether
        // to apply gamma correction to provide 'truer' colors (default true).
        rainbow3(firstPixelHue,1,255,255,true,until_led);
        // Above line is equivalent to:
        // _strip->rainbow(firstPixelHue, 1, 255, 255, true);
        _strip->show(); // Update strip with new contents
        delay(wait);  // Pause for a moment
      }
    }


    void Play()
    {
      // Fill along the length of the strip in various colors...
      colorWipe(_strip->Color(255,   0,   0), 50); // Red
      colorWipe(_strip->Color(  0, 255,   0), 50); // Green
      colorWipe(_strip->Color(  0,   0, 255), 50); // Blue

      // Do a theater marquee effect in various colors...
      theaterChase(_strip->Color(127, 127, 127), 50); // White, half brightness
      theaterChase(_strip->Color(127,   0,   0), 50); // Red, half brightness
      theaterChase(_strip->Color(  0,   0, 127), 50); // Blue, half brightness

      rainbow(10);             // Flowing rainbow cycle along the whole strip
      theaterChaseRainbow(50); // Rainbow-enhanced theaterChase variant
    }

};

class SnowGenerator
{
  public:
  static const uint8_t nfiocchi=3;
  uint8_t vRandX[display_height_][nfiocchi]  = {{}};
  uint8_t vRandY[display_height_]  = {};
  uint8_t currentY;
  bool start_over;

  private:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C* _u8g2;

  public:
    SnowGenerator(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2_)
    {
      _u8g2 = u8g2_;
      randomSeed(analogRead(0));
      for(int i=0; i<nfiocchi; ++i)
      {
        vRandX[0][i] = (uint8_t)random(0, 128);
      }
      currentY=0;
      start_over=false;
      
    }
    void Animate(bool draw)
    {     
      //shifto ogni riga di +1 : e.g, la riga y alla riga y+1
      for(int y=display_height_-2; y>=0; --y)
      {
        for(int i=0; i<nfiocchi; ++i)
        {
          vRandX[y+1][i] = vRandX[y][i];
        }
      }
            
      //creo la neve virtuale per la riga 0     
      for(int i=0; i<nfiocchi; ++i)
      {
        vRandX[0][i] = (uint8_t)random(0, 128);
      }

      if (draw)
      {
        for(int y=0; y<display_height_; ++y)
        {
          if(!start_over)
          {
            if(y>currentY)
            {
              break;
            }
          }
          for(int x=0; x<nfiocchi; ++x)
          {
            
            this->_u8g2->drawPixel(vRandX[y][x],y);
          }
          
        }
      }
    
      ++currentY;
      if (currentY>=display_height_-1)
      {
        currentY=0;
        start_over=true;
      }
    }
};

enum TipoAnimazione
{
  oscillazione,
  movimento
};

class BitmapAnimation
{
  public:
    int nframe;
    int display_height;
    int display_width;
    int current_frame;
    int X;
    int Y;
    int msChangeFrame;
    int frame_height;
    int frame_width;
    int Xincrement;
    double Yincrement;
    bool done;

  private:
    int StartY;
    double current_time;
    bool first_scan;
    TimerC* timerFrames;
    TimerC* timerMove;
    double phaseY;
    TipoAnimazione taY;


  public:
    BitmapAnimation(int dh_, int dw_, int fh_, int fw_, int nframe_, int startX_, int startY_, int msChangeFrame_, int Xincrement_, double Yincrement_, TipoAnimazione taY_)
    {
        frame_height=fh_;
        frame_width=fw_;
        nframe=nframe_;
        display_height = dh_;
        display_width = dw_;
        current_frame=0;
        X=startX_;
        taY = taY_;
        Yincrement = Yincrement_;
        phaseY = 0;
        if (taY==TipoAnimazione::movimento)
        {
          Y=startY_; 
        }
        else
        {
          Y=0;
          StartY = startY_;         
        }             
        Xincrement=Xincrement_;
        current_time = 0;
        first_scan = true;
        timerFrames = new TimerC();
        timerMove = new TimerC();
        timerFrames->start();
        timerMove->start();
    }

    void Animate(bool anim)
    {
        if (anim)
        {
            done = false;
            if(timerFrames->getET()>=msChangeFrame)
            {
              timerFrames->reset();
              //cambia frame
              ++current_frame;
              if(current_frame>=nframe)
                current_frame=0;
            }
            

            //Per la Y eseguo un movimento di "fluttuazione" nell'aria della slitta
            //lo simulo tramite una sinusoide
            long et_move = timerMove->getET();
            if(et_move>0)
            {
              timerMove->reset();

              X+=Xincrement;
              //se il punto X=0 (corner in alto a SX) è fuori dallo schermo a destra:
              if (X>display_width)
              {
                //allora porto tutta l'immagine sul lato esterno sinistro dello schermo
                //in questo modo il bordo destro dell'immagine si trova alla cordinata X=0.
                //L'immagine cosi esce da destra e rientra da sinistra
                X = -frame_width;
                phaseY=0;
                done = true;
              }
              

              //current_time += et_move*0.001; //in sec
              //Y = StartY + 3*sin(TWO_PI*100*current_time);
              if(taY==TipoAnimazione::oscillazione)
              {
                  Y = StartY + 5*sin(phaseY);
                  phaseY+=Yincrement; //M_PI/8;
              }
              else
              {
                Y+=static_cast<int>(Yincrement);
              }
              

              //Serial.println(Y);
            }
            

        }
        else
        {
          //statica
          done = true;
        }
    }



};

