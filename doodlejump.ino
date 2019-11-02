#include <list>
#include <cmath>

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

#define USEGYRO 1
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
    y=(y+v_offset)%HEIGHT;
    if(y+h>HEIGHT){ // need to split up across hardware boundary
      _tft->fillRect(x, y, w, (HEIGHT)-y, color);
      _tft->fillRect(x, 0, w, h-(HEIGHT)+y, color);
    } else {
      _tft->fillRect(x, y, w, h, color);
    }
  }
 private:
  uint16_t v_offset;
  MCUFRIEND_kbv* _tft;
};

Vtft vtft(tft);

struct Platform{
  uint16_t x;
  uint16_t y;
  uint16_t w;
  boolean visable;
};
std::list<Platform> platforms;

int t=0;
void scroll_and_generate(uint16_t distance){
  // first, update old platforms
  for(std::list<Platform>::const_iterator iterator = platforms.begin(), end = platforms.end(); iterator != end;){
    Platform &platform = *iterator;

    // undraw/remove platforms that are about to go off screen
    if(platform.y<distance){
      vtft.fillRect(platform.x, platform.y, platform.w, 1, BLACK);
      iterator=platforms.erase(iterator);
      continue;
    }

    // move all the remaining ones down
    platform.y-=distance;
    ++iterator;
  }

  // do the scrolling
  vtft.vertScroll(0, HEIGHT, distance);
  
  // finally, draw new ones
  uint16_t i = HEIGHT-1;
  do{
    if(rand()%100<5){
      Platform spawn;
      spawn.y = i;
      spawn.w = 10+rand()%30;
      spawn.x = rand()%(WIDTH-spawn.w);
      platforms.push_back(spawn);
      vtft.fillRect(spawn.x, spawn.y, spawn.w, 1, WHITE);
    }
  }while(i-->(HEIGHT-distance));

}

class Player{
 public:
  Player(){
    width=25;
    height=25;
    x = (WIDTH-width)/2;
    y = 150;
    prev_x = x;
    prev_y = y;
    x_speed = 0.1; // measured in pixels per second
    y_speed = 0; // measured in pixels per second
    y_accel = -250; // measured in pixels per second^2
    color = BLUE;
  }
  void force_render(){
    vtft.fillRect(prev_x, prev_y, width, height, color);
  }
  void render(){
    if((uint16_t)x == (uint16_t)prev_x && (uint16_t)y == (uint16_t)prev_y)
      return;
    
    /* First, calculate new area to fill with player
       Done in 2 steps (only if overlapping):

             NEW AREA
             _________
            |  :      |
            |1 :  2   |
      ______|__:      |
     |      |0 |      |
     |      |__|______|
     |  4   :  |
     |      :3 |
     |______:__|
      OLD AREA

             OLD AREA
             _________
            |  :      |
            |3 :  4   |
      ______|__:      |
     |      |0 |      |
     |      |__|______|
     |  2   :  |
     |      :1 |
     |______:__|
      NEW AREA

     Region 0 is ignored, as we don't need to write to those pixels
     Region 1 and 2 will be rendered as seperate rectangles to keep command count low
     */
    int16_t left;
    int16_t bottom;
    int16_t right;
    int16_t top;
    // bounding box 1
    if(y>prev_y){
      top = y+height;
      bottom = prev_y+height;
      if(bottom < y)
	bottom = y;
    } else {
      top = prev_y;
      bottom = y;
	top = y+height;
      left=0;
      right=10;
    }
    if(x>prev_x){
      left = x;
      right = prev_x+width;
    } else {
      left = prev_x;
      right = x+width;
    }
    if(right-left > 0) // make sure boxes overlap
      vtft.fillRect(left, bottom, right-left, top-bottom, RED);
    // bounding box 2
    top   = y+height;
    bottom = y;
    if(x>prev_x){
      left = prev_x+width;
      right   = x+width;
      if(left < x)
	left = x;
    } else {
      left = x;
      right = prev_x;
      if(right>x+width)
	right = x+width;
    }
    vtft.fillRect(left, bottom, right-left, top-bottom, RED);
    // bounding box 3
    if(y>prev_y){
      top   = y-1;
      bottom = prev_y;
      if(top > prev_y+height)
	top = prev_y+height;
    } else {
      top = prev_y+height;
      bottom = y+height+1;
      if(bottom < prev_y)
	bottom = prev_y;
    }
    if(x>prev_x){
      left = x;
      right = prev_x+width;
    } else {
      left = prev_x;
      right = x+width;
    }
    if(right-left > 0) // make sure boxes overlap
      clearRect(left, bottom, right, top);
    // bounding box 4
    top   = prev_y+height;
    bottom = prev_y;  
    if(x>prev_x){
      left = prev_x;
      right = x-1;
      if(right > prev_x+width)
	right = prev_x+width;
    } else {
      left = x+width+1;
      right = prev_x+width;
      if(left < prev_x)
	left = prev_x;
    }
    clearRect(left, bottom, right, top);
  }
  boolean calc_next_pos(double time){
    prev_x = x;
    prev_y = y; // store last coordinates in order to rerender properly

    x = std::fmod(x+x_speed,WIDTH); // modulus for wrap around

    y_speed+=y_accel*time;
    y+=y_speed*time;

    // check for platforms to bounce off of
    if(y_speed<=0){ // but only if player is falling down
      for(std::list<Platform>::const_iterator iterator = platforms.begin(), end = platforms.end(); iterator != end; iterator++){
	Platform &platform = *iterator;
	if(platform.y < prev_y && platform.y > y){ // it's in our path down
	  /* the X calculation is a bit more complicated. We need to see if the platform is in any part of our projected path
	     to do so, we need to calculate the path by seeing where the projections of the bottom 2 lines intersect the y level of the platform:
	     ......
             :OLD :
             :....:
              .    .
                .    .
             .....x.==.x.....
                    .    .
                      .    .__
                        . |NEW|
                          !___!
	     From there, bounds can be calculated and adjustments made
	   */

	  //calculate intercepts by solving point slope form
	  double y1 = prev_y;
	  double y2 = y;
	  double x1 = prev_x;
	  double x2 = x;
	  double intercept_1 = (platform.y-y1)*(x2-x1)/(y2-y1)+x1;
	  double intercept_2 = intercept_1+width;

	  if((platform.x >= intercept_1 && platform.x <= intercept_2)
	     || (platform.x+platform.w >= intercept_1 && platform.x+platform.w <= intercept_2)
	     || (intercept_1 >= platform.x && intercept_1 <= platform.x+platform.w)
	     || (intercept_2 >= platform.x && intercept_2 <= platform.x+platform.w)){ // check for collision
	    y = platform.y+1;
	    y_speed = 250;
	  }
	}
      }
    }
    
    if(y > HEIGHT*2/3){ // time to move up
      scroll_and_generate((uint32_t)y-HEIGHT*2/3); // adjust screen & clean up platforms
      prev_y -= y-HEIGHT*2/3; // adjust position relative to screen
      y = (double)(uint32_t)HEIGHT*2/3;
    }

    if(x<0)
      x=WIDTH+x;
    x = fmod(x,WIDTH);
    
    if(y<0){
      return false;
    }
    
    render();
    return true;
  }
  void clearRect(uint16_t left, uint16_t bottom, uint16_t right, uint16_t top){
    left--;bottom--;top++;right++;
    vtft.fillRect(left, bottom, right-left, top-bottom, BLACK);
    // after clearing, check to see if there's any platforms we need to render again
    for(std::list<Platform>::const_iterator iterator = platforms.begin(), end = platforms.end(); iterator != end; iterator++){
      Platform &platform = *iterator;
      if(platform.y <= top && platform.y >= bottom) // in same y-plane
	if((left >= platform.x && left <= platform.x+platform.w)
	   || (right >= platform.x && right <= platform.x+platform.w)
	   || (platform.x >= left && platform.x <= right)
	   || (platform.x+platform.w >= left && platform.x+platform.w <= right)) // x overlaps
	  vtft.fillRect(platform.x, platform.y, platform.w, 1, WHITE); // it's just as fast to redraw the whole platform
    }
  }
  double x, y, x_speed, y_speed, y_accel, width, height, prev_x, prev_y; // relative to screen
  uint16_t color;
};

Player player;

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
  scroll_and_generate(HEIGHT); // create a fresh set of platforms
  // create a floor for the player to start on
  Platform floor;
  floor.y = 0;
  floor.w = WIDTH-1;
  floor.x = 0;
  platforms.push_back(floor);
  vtft.fillRect(floor.x, floor.y, floor.w, 1, WHITE);
  player.force_render();
}


void loop() {
  //delay(1);
#if USEGYRO == 1
  mpu6050.update();
  player.x_speed = (mpu6050.getAngleX()-23)/10;
#endif

  delay(1);

  if(!player.calc_next_pos(0.01)){ //game over -- reset
    tft.fillScreen(BLACK);
    player.x = (WIDTH-player.width)/2;
    player.y = 150;
    player.prev_x = player.x;
    player.prev_y = player.y;
    player.x_speed = 0;
    player.y_speed = 0;
    platforms.clear();
    scroll_and_generate(HEIGHT); // create a fresh set of platforms
    // create a floor for the player to start on
    Platform floor;
    floor.y = 0;
    floor.w = WIDTH-1;
    floor.x = 0;
    platforms.push_back(floor);
    vtft.fillRect(floor.x, floor.y, floor.w, 1, WHITE);
    player.force_render();
  }

}
