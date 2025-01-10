uint8_t RECV_PIN = 9; //TSOP2138: OUT
uint8_t VCC = 11; //TSOP2138: 5V
uint8_t GND = 10; //TSOP2138: GND
bool screenOff;

#include <IRremote.h>
IRrecv irrecv(RECV_PIN);
decode_results results;

#include "LedControl.h"
#include <TimerOne.h>

#define DISPLAYS 6
#define ALIEN 0
#define ALIENS 4
#define BULLETS 5
#define ALIENSPACING 5
#define ROCKET 1
#define updateTime 100000
#define LENGTH 72
#define ANIMATIONTIME 2
#define BULLETRANGE 16
#define IMMUNITY 10
#define LEVELS 5

//Game Modifiers
uint8_t alienShotLimit = 5;
uint8_t dropSpeed = 40;
uint8_t alienFireRate = 10;
uint8_t alienMoveSpeed = 15;
uint8_t bulletSpeed = 2;
uint8_t level = 0;
uint8_t playerLives = 3;

//arduino pins
volatile bool change = 0;
uint8_t data = 5;
uint8_t clk = 7;
uint8_t cs = 6;
uint8_t leftButton = 2;
uint8_t rightButton = 3;
uint8_t shootButton = A3;

//Game Variables
uint8_t moveRange = 22;
uint8_t image[LENGTH] = {0};
volatile uint8_t bulletCount = 0;
volatile uint8_t waitCount = 0;
volatile uint8_t alienMoveCount = 0;
volatile uint8_t animationCount = 0;
volatile uint8_t immunityCount = 0;
volatile uint8_t dropCount = 0;
uint8_t alienDirection = 0;
uint8_t currentshootButton = 0;
uint8_t previousShootButton = 0;
uint8_t alienDir = 1;
uint8_t animationStep = 0;
uint8_t animationX = 5;
uint8_t animationY = 6;
uint8_t animationMatrix = 0;
uint8_t rocketAnimationActive = 0;
uint8_t nextAnimation = 0;
uint8_t alienShootCount = 0;
uint8_t playerImmune = 0;
uint8_t wait = false;

LedControl lc = LedControl(data, clk, cs, 6); //(data, CLK, CS, # of displays)

//Alien Object
struct Alien {
    uint8_t mainPos;
    uint8_t pos[3] = {0, 1, 2};
    uint8_t image[3] = {B11100000, B11000000, B11100000};
    uint8_t img[LENGTH] = {0};
    uint32_t fullIMG = 0;
    uint8_t height = 14;
    bool destroyed = false;
};

//player object
struct Rocket {
    uint8_t height = 1;
    uint8_t mainPos = 11;
    uint8_t pos[3] = {0, 1, 2};
    uint8_t img[LENGTH] = {0};
    bool destroyed = false;
    void reset() {
        mainPos = 11;
        destroyed = false;
    }
} rocket;

//Bullet Object (for both player and alien)
struct Bullet {
    int8_t dir;
    uint8_t pos;
    uint8_t height;
    uint8_t matrix;
    uint8_t bulletReady = 1;
    void Reset(uint8_t x, uint8_t y, uint8_t d) {
        pos = x;
        height = y;
        dir = d;
    }
    uint8_t localPos() {
        return pos % 8;
    }
    void increment() {
        if (!bulletReady) height += dir;
        if (height > BULLETRANGE || height < 0) {
            bulletReady = 1;
        }
    }
    uint8_t getImage() {
        uint8_t img = 128 >> height % 8;
        return img;
    }
};

Alien alien[ALIENS];//array to hold allien instances
Bullet bA[BULLETS];//array to hold bullet instances
Bullet bR;

//##########SETUP##########
// sets up all the pins, the clock, the IR receiver, and the LED matricies
void setup() {
    pinMode(VCC, OUTPUT);
    pinMode(GND, OUTPUT);
    digitalWrite(VCC, HIGH);
    digitalWrite(GND, LOW);
    irrecv.enableIRIn();
    for (uint8_t i = 0; i < ALIENS; i++) {
        alien[i].mainPos = 1 + i * ALIENSPACING;
    }
    for (uint8_t i = 0; i < DISPLAYS; i++) {
        lc.shutdown(i, false);
        lc.setIntensity(i, 5);
        lc.clearDisplay(i);
    }
    Timer1.initialize(updateTime);
    Timer1.attachInterrupt(advance);
}

//##########LOOP#############
// first check for IR signal
// then update all objects and animations relying on time
// then check for collisions
// then refresh the display
void loop() {
    recieveIR();
    countTime();
    checkHit();
    updateImage();
}
//##########LOOP#############
//the arduino library decodes the signal received
// then it will either move the player left or right, or shoot
void recieveIR() {
    if (irrecv.decode(&results)) {
        unsigned int value = results.value;
        switch (value) {
            case 18105://left
                if (wait) return;
                if (rocket.mainPos < moveRange) {
                    rocket.mainPos++;
                }
                break;
            case 42585://right
                if (wait) return;
                if (rocket.mainPos > 1) {
                    rocket.mainPos--;
                }
                break;
            case 5865://shoot
                shoot(ROCKET, rocket.mainPos, rocket.height + 2);
                break;
                case 16575://on/off
                if (screenOff == 0) screenOff = 1;
                else screenOff = 0;
        }
        irrecv.resume();
    }
}

// updates the player image based on position
void rocketshipImg() {
    for (uint8_t i = 0; i < LENGTH; i++) {
        rocket.img[i] = 0;
    }
    if (!rocket.destroyed) {
        rocket.pos[0] = rocket.mainPos - 1;
        rocket.pos[1] = rocket.mainPos;
        rocket.pos[2] = rocket.mainPos + 1;
        rocket.img[rocket.pos[0]] = B11000000 >> rocket.height - 1;
        rocket.img[rocket.pos[1]] = B11100000 >> rocket.height - 1;
        rocket.img[rocket.pos[2]] = B11000000 >> rocket.height - 1;
    }
}

//##########UPDATE IMAGE##########
//sends all the object images to the registers controlling the LEDs
void updateImage() {
    if (screenOff) {
        for (uint8_t i = 0; i < DISPLAYS; i++) {
            lc.clearDisplay(i);
        }
        wait = 1;
        waitCount = 1;
    }
    else {
        rocketshipImg();
        for (uint8_t i = 0; i < LENGTH; i++) {
            image[i] = 0;
            image[i] = image[i] | rocket.img[i];
            image[i] = image[i] | alien[0].img[i];
        }
        setBulletImages();
        setAlienImages();
        if (rocketAnimationActive == 1) {
            showAnimation();
        }
        rotate();
        for (uint8_t v = 0; v < 6; v++) {
            uint8_t change = v * 8;
            for (uint8_t i = 0; i < 8; i++) {
                lc.setRow(v, i, image[i + change]);
            }
        }
    }
}
//increments all the counters used by the different objects (affects positions and actions)
void advance() {
    bulletCount++;
    animationCount++;
    if (wait) {
        waitCount--;
        return;
    }
    alienMoveCount++;
    alienShootCount++;
}
//shooter variable defines if it is a player = 1 or alien = 0
//player can only have one bullet active so checks if it is available and then fires it
//Aliens have a combined pool of bullets so checks if any are available and then fires it
void shoot(uint8_t shooter, uint8_t x, uint8_t y) {
    if (shooter) {
        //it is a player shooting
        if (bR.bulletReady) {
            bR.bulletReady = 0;
            bR.Reset(x, y, 1);
        }
    } else {
        //it is an alien shooting
        for (uint8_t i = 0; i < alienShotLimit; i++) {
            if (bA[i].bulletReady) {
                bA[i].Reset(x, y, -1);
                bA[i].bulletReady = 0;
                return;
            }
        }
    }
}

//sets the bullet position to out of frame and sets the ready flag to true
void resetBullet() {
    for (uint8_t i; i < alienShotLimit; i++) {
        bA[i].bulletReady = 1;
        bA[i].height = 22;
    }
    bR.bulletReady = 1;
    bR.height = 22;
}

//checks all the counters and performs the corresponding actions if it is ready
void countTime() {
    if (bulletCount == bulletSpeed) {//moves the bullet one spot forward
        for (uint8_t i = 0; i < alienShotLimit; i++) bA[i].increment();
        bR.increment();
        bulletCount = 0;
    }
    if (alienMoveCount == alienMoveSpeed) {//moves the aliens one spot
        moveAliens();
        alienMoveCount = 0;
    }
    if (animationCount == ANIMATIONTIME) {//moves to the next animation step
        if (rocketAnimationActive == 1) {
            animationStep++;
        }
        animationCount = 0;
    }
    if (alienShootCount == alienFireRate) {//fires a bullet from a random alien
        alienShootCount = 0;
        Alien shooter = getRandomAlien();
        if (!shooter.destroyed) {
            shoot(ALIEN, shooter.mainPos, shooter.height - 2);
        }
    }
    if (waitCount == 0) {
        if (wait) {//resets the positions of players and aliens but does not reset the game
            resetBullet();
            for (uint8_t i = 0; i < ALIENS; i++) {
                alien[i].mainPos = 1 + i * ALIENSPACING;
            }
            rocketAnimationActive = 0;
        }
        wait = false;
        if (rocket.destroyed) {
            rocket.reset();
        }
    }
    if (dropCount == dropSpeed) {//aliens move down towards the player
        if (alien[0].height < 6) return;
        for (uint8_t i = 0; i < ALIENS; i++) {
            alien[i].height--;
        }
    }
}

//sets the positions of all bullets
void setBulletImages() {
    for (uint8_t i = 0; i < alienShotLimit; i++) {
        setBulletImage(bA[i]);
    }
    setBulletImage(bR);
}
//sets the position of a specific bullet
void setBulletImage(Bullet b) {
    if (b.bulletReady) {
        return;
    }
    b.matrix = getMatrix(b.pos, b.height);
    image[b.localPos() + b.matrix * 8] |= b.getImage();
}

//takes the global position and determines what LED Matrix it is in
// numbered as so:
/* (each matrix is 8x8)
[4][5][6]
[1][2][3]
*/
uint8_t getMatrix(uint8_t x, uint8_t y) {
    uint8_t row = 0;
    uint8_t collumn = 0;
    if (y <= 7) {
        row = 0;
    } else if (y <= 15) {
        row = 1;
    } else if (y <= 23) {
        row = 2;
    }
    if (x <= 7) {
        collumn = 0;
    } else if (x <= 15) {
        collumn = 1;
    } else if (x <= 23) {
        collumn = 2;
    }
    return row * 3 + collumn;
}

//calculates all the alien images based on position
void setAlienImages() {
    for (uint8_t v = 0; v < ALIENS; v++) {
        alienImg(alien[v]);
    }
}
//sets the image for a single alien position 
void alienImg(Alien a) {
    for (uint8_t i = 0; i < LENGTH; i++) {
        a.img[i] = 0;
    }
    if (a.destroyed) {
        return;
    }
    a.pos[0] = a.mainPos - 1;
    a.pos[1] = a.mainPos;
    a.pos[2] = a.mainPos + 1;
    a.fullIMG = 0xE00000 >> a.height - 1;
    image[a.pos[0]] |= a.fullIMG >> 16;
    image[a.pos[0] + 24] |= (a.fullIMG >> 8) % 256;
    image[a.pos[0] + 48] |= a.fullIMG % 256;
    a.fullIMG = 0x600000 >> a.height - 1;
    image[a.pos[1]] |= a.fullIMG >> 16;
    image[a.pos[1] + 24] |= (a.fullIMG >> 8) % 256;
    image[a.pos[1] + 48] |= a.fullIMG % 256;
    a.fullIMG = 0xE00000 >> a.height - 1;
    image[a.pos[2]] |= a.fullIMG >> 16;
    image[a.pos[2] + 24] |= (a.fullIMG >> 8) % 256;
    image[a.pos[2] + 48] |= a.fullIMG % 256;
}
//moves the aliens horizontally in one direction
//the direction changes when an alien reaches the border
void moveAliens() {
    for (uint8_t i = 0; i < ALIENS; i++) {
        if (!alien[i].destroyed) {
            if (alien[i].mainPos <= 1) alienDir = 1;
            else if (alien[i].mainPos >= 22) alienDir = -1;
        }
    }
    for (uint8_t i = 0; i < ALIENS; i++) {
        if (!alien[i].destroyed) {
            alien[i].mainPos += alienDir;
        }
    }
}
//checks if a player bullet has hit an alien or if an alien bullet has hit a player
void checkHit() {
    for (uint8_t i = 0; i < ALIENS; i++) {// check bullet hit an alien
        if (findDifference(bR.height, alien[i].height) <= 1) {
            if (findDifference(bR.pos, alien[i].mainPos) <= 1) {
                if (!alien[i].destroyed) {
                    alien[i].destroyed = 1;
                    bR.height = BULLETRANGE + 1;
                    runAnimation(alien[i].mainPos, alien[i].height);
                    checkAliens();
                }
            }
        }
    }
    for (uint8_t i = 0; i < alienShotLimit; i++) {//check bullet hit player
        if (findDifference(bA[i].height, rocket.height) <= 1) {
            if (findDifference(bA[i].pos, rocket.mainPos) <= 1) {
                if (!bA[i].bulletReady) {
                    rocket.destroyed = true;
                    bA[i].bulletReady = true;
                    if (rocketAnimationActive == 0)
                    runAnimation(rocket.mainPos, rocket.height);
                    resetLevel();
                }
            }
        }
    }
}

uint8_t findDifference(uint8_t num1, uint8_t num2) {//returns the positive difference between two numbers
    if (num1 > num2) {
        return num1 - num2;
    } else {
        return num2 - num1;
    }
}

void runAnimation(uint8_t x, uint8_t y) {//sets up the explotion animation to run at a specific position
    animationMatrix = getMatrix(x, y);
    animationX = x % 8;
    animationY = y % 8;
    animationCount = 0;
    animationStep = 0;
    rocketAnimationActive = 1;
}
//displays an images based on the step of the animation (4 total steps)
void showAnimation() {
    switch (animationStep) {
            case 0:
                image[animationX + animationMatrix * 8] |= 64 >> (animationY - 1) % 8;
                break;
            case 1:
                image[animationX - 1 + animationMatrix * 8] |= 64 >> (animationY - 1) % 8;
                image[animationX + animationMatrix * 8] |= 224 >> (animationY - 1) % 8;
                image[animationX + 1 + animationMatrix * 8] |= 64 >> (animationY - 1) % 8;
                break;
            case 2:
                image[animationX - 1 + animationMatrix * 8] |= 224 >> (animationY - 1) % 8;
                image[animationX + animationMatrix * 8] |= 160 >> (animationY - 1) % 8;
                image[animationX + 1 + animationMatrix * 8] |= 224 >> (animationY - 1) % 8;
                break;
            case 3:
                image[animationX - 1 + animationMatrix * 8] |= 160 >> (animationY - 1) % 8;
                image[animationX + 1 + animationMatrix * 8] |= 160 >> (animationY - 1) % 8;
                break;
        }
}
// picks a random alien and returns it
Alien getRandomAlien() {
    uint8_t findAlien = 1;
    uint8_t randomNum;
    randomNum = random(0, ALIENS);
    return alien[randomNum];
}
//checks if the player is alive, this is called after a player is hit
void checkLife() {
    playerLives--;
    if (playerLives <= 0) {
        gameOver();
    } else {
        resetLevel();
    }
}
//resets the positions of all the alive entities
void resetLevel() {
    waitCount = 50;
    wait = true;
}
//checks how many aliens are alive. if 0 then the game is over
void checkAliens() {
    uint8_t destroyCount = 0;
    for (uint8_t v = 0; v < ALIENS; v++) {
        if (alien[v].destroyed) destroyCount++;
    }
    if (destroyCount == ALIENS) {
        gameOver();
    }
}

void gameOver() {//turns the display off
    for (uint8_t i = 0; i < DISPLAYS; i++) {
        lc.clearDisplay(i);
    }
}

//rotates the image 90 degrees
//because of how the LED Matricies are wired, the images need to be rotated before they are displayed
void rotate() {
    uint8_t newImage[8];
    uint8_t shiftImage[8];
    for (uint8_t rotations = 0; rotations < DISPLAYS; rotations++) {
        uint8_t rotShift = 8 * rotations;
        for (uint8_t v = 0; v < 8; v++) {
            uint8_t shift = v;
            for (uint8_t i = 0; i < 8; i++) {
                shiftImage[i] = (image[i + rotShift] >> shift) % 2;
            }
            newImage[v] = (shiftImage[0] << 7) + (shiftImage[1] << 6) +
            (shiftImage[2] << 5) + (shiftImage[3] << 4)
            + (shiftImage[4] << 3) + (shiftImage[5] << 2) +
            (shiftImage[6] << 1) + (shiftImage[7] << 0);
        }
        for (uint8_t i = 0; i < 8; i++) {
            image[i + rotShift] = newImage[i];
        }
    }
}
