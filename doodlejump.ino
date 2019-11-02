#include <list>

#include <MPU6050_tockn.h>
#include <Wire.h>

MPU6050 mpu6050(Wire);

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h> 

#define BLACK 0x0000
#define NAVY 0x000F
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define OLIVE 0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define BLUE 0x001F
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5

MCUFRIEND_kbv tft(A3, A2, A1, A0, A4);  

#define USEGYRO 0
#define HEIGHT 320
#define WIDTH 240

class Vtft{
 public:
  Vtft(MCUFRIEND_kbv &tft_initilizer){
    _tft = &tft_initilizer;
    v_offset = 0;
  }
  void vertScroll(int16_t top, int16_t scrollines, int16_t offset){
    v_offset=(v_offset+offset)%HEIGHT;
    _tft->vertScroll(top, scrollines, v_offset);
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
    _tft->fillRect(x, (y+v_offset)%HEIGHT, w, h, color);
  }
 private:
  uint16_t v_offset;
  MCUFRIEND_kbv* _tft;
};

Vtft vtft(tft);

struct Platform{
  int16_t x;
  int16_t y;
  int16_t w;
  boolean visable;
};
std::list<Platform> platforms;

void setup() {
  Serial.begin(115200);
  uint16_t ID = tft.readID();
  tft.begin(ID); 
  tft.fillScreen(BLACK);
#if USEGYRO == 1
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
#endif
  Platform first;
  first.y = HEIGHT+100;
  first.w = 10+rand()%30;
  first.x = rand()%(WIDTH-first.w);
  first.visable = false;
  platforms.push_back(first);
}


void loop() {
  //delay(1);
#if USEGYRO == 1
  mpu6050.update();
  Serial.println("\nangleX: " );
  Serial.print(mpu6050.getAngleX());
  Serial.print("\tangleY: " );
  Serial.print(mpu6050.getAngleY());
  Serial.print("\tangleZ: " );
  Serial.print(mpu6050.getAngleZ());
#endif
  // presenting the platforms:
  /*
    Store an amount, maybe dynamic (could use data structures)
    store as units, where y units are relative to the screen (can go into negatives -- these aren't rendered)
    only store platforms so far below and above
    when player moves, platforms are generated
   */
  
  if(rand()%100<3){ // spawn new
    Platform spawn;
    spawn.y = HEIGHT+100;
    spawn.w = 10+rand()%30;
    spawn.x = rand()%(WIDTH-spawn.w);
    spawn.visable = false;
    platforms.push_back(spawn);
  }

  // update platform positions
  for(std::list<Platform>::const_iterator iterator = platforms.begin(), end = platforms.end(); iterator != end;){
    Platform &platform = *iterator;
    platform.y--;
    
    if(platform.y<-100 || platform.y>HEIGHT+100){ // too far down or high up
      iterator=platforms.erase(iterator);
      continue;
    }
    if(((platform.y<0) || (platform.y>(HEIGHT-1))) && platform.visable){ // off screen but still drawn
      vtft.fillRect(platform.x, platform.y, platform.w, 1, BLACK);
      platform.visable = false;
    }
    if(!((platform.y<0) || (platform.y>(HEIGHT-1))) && !platform.visable){ // on screen, but not drawn
      vtft.fillRect(platform.x, platform.y, platform.w, 1, WHITE);
      platform.visable = true;
    }
    ++iterator;
  }
  
  //vtft.fillRect(0, 0, WIDTH, 1, RED);
  vtft.vertScroll(0, HEIGHT, 1);
  
  delay(10);

}
