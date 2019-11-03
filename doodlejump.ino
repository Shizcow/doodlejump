// C++ libs
#include <list>
#include <cmath>
// Accelerometer libs
#include <MPU6050_tockn.h>
#include <Wire.h>
MPU6050 mpu6050(Wire);
// LCD libs
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h> 
MCUFRIEND_kbv tft(A3, A2, A1, A0, A4);  

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

#define HEIGHT 320
#define WIDTH 240

class Vtft{
  /* Virtual tft display
     Hardware scrolling is really fast, but changes where pixel coordinates are drawn
     For example, drawing a pixel at (0,0) over and over while scrolling the screen
     causes only a single pixel to be seen, moving around

     To combat this, an offset is added and kept track of, allowing the screen to behave
     as expected
  */
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
    if(y+h>HEIGHT){ // if a rectangle isn't split up across the y=0 hardware boundary, 
      _tft->fillRect(x, y, w, (HEIGHT)-y, color); // artifacting occurs
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

// platforms are kept as a dynamic list to keep memory useage low
struct Platform{
  uint16_t x;
  uint16_t y;
  uint16_t w;
};
std::list<Platform> platforms;

void scroll_and_generate(uint16_t distance){
  // Scrolls the screen down, cleaning up any platforms that need to go off the bottom and generating new ones at the top
  if(distance == 0)
    return;
  // first, update old platforms
  for(std::list<Platform>::const_iterator iterator = platforms.begin(), end = platforms.end(); iterator != end;){
    Platform &platform = *iterator; // step through the std::list

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
  uint16_t i = HEIGHT-1; // start at the top
  do{
    if(rand()%100<5){
      Platform spawn; // new platform
      spawn.y = i; // at given Y level, stepping down
      spawn.w = 10+rand()%30; // of random width
      spawn.x = rand()%(WIDTH-spawn.w); // and random position
      platforms.push_back(spawn); // added to list
      vtft.fillRect(spawn.x, spawn.y, spawn.w, 1, WHITE); // and drawn
    }
  }while(i-->(HEIGHT-distance));
}

// all the player physics and rendering is kept here
class Player{
 public:
  Player(){ // init physics values
    width=25;
    height=25;
    x = (WIDTH-width)/2;
    y = 150;
    prev_x = x;
    prev_y = y;
    x_speed = 0.1; // measured in pixels per second
    y_speed = 0; // measured in pixels per second
    y_accel = -250; // measured in pixels per second^2
    color = RED;
  }
  void force_render(){ // render without any fancy caluclations -- used for first render
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
     Region 3 and 4 are cleared, and then any platforms underneath them are redrawn
     */
    int16_t left;
    int16_t bottom;
    int16_t right;
    int16_t top;
    // bounding box 1
    if(y>prev_y){
      top = y+height;
      bottom = prev_y+height;
      if(bottom < y) // if not overlap
	bottom = y;
    } else {
      top = prev_y;
      bottom = y;
      if(top > y+height) // if not overlap
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
    if(right-left > 0) // make sure boxes overlap at all
      vtft.fillRect(left, bottom, right-left, top-bottom, color);
    // bounding box 2
    top   = y+height;
    bottom = y;
    if(x>prev_x){
      left = prev_x+width;
      right   = x+width;
      if(left < x) // if not overlap
	left = x;
    } else {
      left = x;
      right = prev_x;
      if(right>x+width) // if not overlap
	right = x+width;
    }
    vtft.fillRect(left, bottom, right-left, top-bottom, color);
    // bounding box 3
    if(y>prev_y){
      top   = y-1;
      bottom = prev_y;
      if(top > prev_y+height) // if not overlap
	top = prev_y+height;
    } else {
      top = prev_y+height;
      bottom = y+height+1;
      if(bottom < prev_y) // if not overlap
	bottom = prev_y;
    }
    if(x>prev_x){
      left = x;
      right = prev_x+width;
    } else {
      left = prev_x;
      right = x+width;
    }
    if(right-left > 0) // make sure boxes overlap at all
      clearRect(left, bottom, right, top);
    // bounding box 4
    top   = prev_y+height;
    bottom = prev_y;  
    if(x>prev_x){
      left = prev_x;
      right = x-1;
      if(right > prev_x+width) // if not overlap
	right = prev_x+width;
    } else {
      left = x+width+1;
      right = prev_x+width;
      if(left < prev_x) // if not overlap
	left = prev_x;
    }
    clearRect(left, bottom, right, top);
  }
  boolean calc_next_pos(double time){
    prev_x = x;
    prev_y = y; // store last coordinates in order to rerender properly

    x = std::fmod(x+x_speed,WIDTH); // modulus for wrap around

    y_speed+=y_accel*time; // basic physics calculations
    y+=y_speed*time;

    // check for platforms to bounce off of
    if(y_speed<=0){ // but only if player is falling down
      for(std::list<Platform>::const_iterator iterator = platforms.begin(), end = platforms.end(); iterator != end; iterator++){
	Platform &platform = *iterator; // step over platforms
	if(platform.y < prev_y && platform.y > y){ // check if it's in our path down
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
	    x = intercept_1; // place player on top of platform
	    y = platform.y+1;
	    y_speed = 250; // bounce
	  }
	}
      }
    }
    
    if(y > HEIGHT*2/3){ // check if player is getting too high up -- scroll screen down
      scroll_and_generate((uint32_t)y-HEIGHT*2/3); // adjust screen & clean up platforms
      prev_y -= y-HEIGHT*2/3; // adjust position relative to screen
      y = (double)(uint32_t)HEIGHT*2/3;
    }

    if(x<0) // check if player need to wrap from one side of the screen to the other
      x=WIDTH+x;
    x = fmod(x,WIDTH);
    
    if(y<0) // player hit the bottom -- game over
      return false;
    
    render(); // once we know the player's position, show it on screen
    
    return true;
  }
  void clearRect(uint16_t left, uint16_t bottom, uint16_t right, uint16_t top){
    // clear a rectangle and redraw any platforms behind it
    left--;bottom--;top++;right++; // float to int casts are unreliable, so take a little more to be safe
    vtft.fillRect(left, bottom, right-left, top-bottom, BLACK); // replace background
    // after clearing, check to see if there's any platforms we need to render again
    for(std::list<Platform>::const_iterator iterator = platforms.begin(), end = platforms.end(); iterator != end; iterator++){
      Platform &platform = *iterator; // step through each platform
      if(platform.y <= top && platform.y >= bottom) // if it's in the same y-area as the rectangle
	if((left >= platform.x && left <= platform.x+platform.w)
	   || (right >= platform.x && right <= platform.x+platform.w)
	   || (platform.x >= left && platform.x <= right)
	   || (platform.x+platform.w >= left && platform.x+platform.w <= right)) // check if x overlaps at all
	  vtft.fillRect(platform.x, platform.y, platform.w, 1, WHITE); // it's just as fast to redraw the whole platform
    }
  }
  double x, y, x_speed, y_speed, y_accel, width, height, prev_x, prev_y; // relative to screen
  uint16_t color;
};

Player player;

int32_t gyro_meta_offset; // need a second offset because the gyro drifts a lot
void setup() {
  // first, all the boring inits
  Serial.begin(115200);
  uint16_t ID = tft.readID();
  tft.begin(ID); 
  tft.fillScreen(BLACK);
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  
  scroll_and_generate(HEIGHT); // create a fresh set of platforms
  // create a floor for the player to start on
  Platform floor; // new platform
  floor.y = 0; // at the bottom
  floor.w = WIDTH-1; // spanning the whole screen
  floor.x = 0;
  platforms.push_back(floor); // add to list
  vtft.fillRect(floor.x, floor.y, floor.w, 1, WHITE); // and draw
  player.force_render(); // then put the player on screen

  // calibrate gyro
  mpu6050.update();
  gyro_meta_offset = mpu6050.getAngleZ();
}


void loop() {
  // tilt controls
  mpu6050.update();
  player.x_speed = (mpu6050.getAngleZ()-gyro_meta_offset)/5;

  delay(1); // delay just enough to make physics feel consistant

  if(!player.calc_next_pos(0.01)){ // calculate position. If this returns false (if statement is true), game over
    tft.fillScreen(BLACK); // clear the screen
    player.x = (WIDTH-player.width)/2; // put the player back at start
    player.y = 150;
    player.prev_x = player.x;
    player.prev_y = player.y;
    player.x_speed = 0;
    player.y_speed = 0;
    player.force_render(); // and draw the player
    platforms.clear(); // get rid of the platforms
    scroll_and_generate(HEIGHT); // create a fresh set of platforms
    // create a floor for the player to start on
    Platform floor;
    floor.y = 0;
    floor.w = WIDTH-1;
    floor.x = 0;
    platforms.push_back(floor);
    vtft.fillRect(floor.x, floor.y, floor.w, 1, WHITE);
    mpu6050.update(); // finally, recalibrate the gyro and start over
    gyro_meta_offset = mpu6050.getAngleZ();
  }

}
