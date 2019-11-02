// accel libs

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
#define PINK 0xF81F

MCUFRIEND_kbv tft(A3, A2, A1, A0, A4);  



void setup() {
  Serial.begin(9600);
  uint16_t ID = tft.readID();
  tft.begin(ID); 
  tft.fillScreen(BLACK);
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
}

void loop() {
  mpu6050.update();
  Serial.println("\nangleX: " );
  Serial.print(mpu6050.getAngleX());
  Serial.print("\tangleY: " );
  Serial.print(mpu6050.getAngleY());
  Serial.print("\tangleZ: " );
  Serial.print(mpu6050.getAngleZ());
  
    Serial.println("========================================");
    Serial.println("Calculating gyro offsets");
    Serial.print("DO NOT MOVE MPU6050");
}
