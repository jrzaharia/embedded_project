#include <pic18.h>
#include "lcd_portd.c"

#define WORLD_SIZE 16
#define DINO_POS 0     // Dino always on the left

// Game variables
unsigned char world[WORLD_SIZE];
unsigned char is_jumping = 0;
unsigned char jump_timer = 0;
unsigned int wait_time = 300;   // starting frame delay
unsigned int time_between_speed_increase = 3000;
unsigned char last_was_cactus = 0;
unsigned char game_running = 0;
unsigned char score = 0;
unsigned int seconds = 0;
unsigned char step = 0;

unsigned long T2 = 0;            // ms counter (Timer2 interrupt)
unsigned int HIT;
unsigned int tone_time;

const unsigned int C5 = 9556;   // jump tone
const unsigned int G3 = 25510;	// game-over tone

const unsigned char MSG0[20] = "   GAME  OVER   ";
const unsigned char MSG1[20] = "SCORE:          ";
const unsigned char MSG2[20] = "TIME:           ";
const unsigned char MSG3[20] = "   DINO  GAME   ";


// Custom characters
const unsigned char DINO[8] = {
    0b00111,
    0b00101,
    0b00111,
    0b10110,
    0b11111,
    0b11110,
    0b01110,
    0b00110
};

const unsigned char CACTUS[8] = {
    0b00100,
    0b00101,
    0b00101,
    0b10110,
    0b10100,
    0b01100,
    0b00100,
    0b00100
};

// Custom characters
const unsigned char DINORUNNINGL[8] = {
    0b00111,
    0b00101,
    0b00111,
    0b10110,
    0b11111,
    0b11110,
    0b01110,
    0b01000
};

// Custom characters
const unsigned char DINORUNNINGR[8] = {
    0b00111,
    0b00101,
    0b00111,
    0b10110,
    0b11111,
    0b11110,
    0b01110,
    0b00010
};

// Write custom character to CGRAM
void LCD_CustomChar(unsigned char loc, const unsigned char *p)
{
    unsigned char i;
    LCD_Inst(0x40 + (loc * 8));
    for (i = 0; i < 8; i++)
        LCD_Write(p[i]);
}

// Reset world: fill with spaces
void init_world() {
    unsigned char i;
    for(i=0; i<WORLD_SIZE; i++)
        world[i] = 32;   // spaces only
}

// Scroll world left
void scroll_world() {
    unsigned char i;

    // Shift left
    for(i = 0; i < WORLD_SIZE - 1; i++)
        world[i] = world[i+1];

    // --- RANDOM CACTUS GENERATION (prevent two in a row) -------
    if(last_was_cactus == 0 && (rand() % 4 == 0)) {
        world[WORLD_SIZE - 1] = 1;   // cactus
        last_was_cactus = 1;
    }
    else {
        world[WORLD_SIZE - 1] = 32;  // empty
        last_was_cactus = 0;
    }
}

void shift_right(){
    unsigned char i;
    // Shift right
    for(i = WORLD_SIZE -1; i > 0; i--)
        world[i] = world[i-1];
}

// Handle jump input
void jump() {
    if(is_jumping) return;
    is_jumping = 1;
    jump_timer = 2;
}

// Update jump state each frame
void update_jump() {
    if(is_jumping) {
        if(jump_timer > 0)
            jump_timer--;
        else
            is_jumping = 0;
    }
}

// Draw world to LCD
void display_world() {
    unsigned char i;

    LCD_Move(0,10);
    LCD_Out(score,5,0);

    // TOP ROW (Dino only if jumping)
    for(i=0; i<WORLD_SIZE - 5; i++) {
        LCD_Move(0, i);
        if(is_jumping && i == DINO_POS)
            LCD_Write(0);
        else
            LCD_Write(32);
    }

    // BOTTOM ROW (ground + Dino)
    for(i=0; i<WORLD_SIZE; i++) {
        LCD_Move(1, i);
    if(!is_jumping && i == DINO_POS){
        LCD_Write(step ? 3 : 2);  // alternate legs
    }else
            LCD_Write(world[i]);
    }
    step = !step; 
}

void display_opening_animation() {
    LCD_Inst(1); // clear screen
    unsigned char i;
    LCD_Move(0,0);
    for (i=0; i<20; i++) {
        LCD_Write(MSG3[i]); //display title
    }
    for(i = 0; i < WORLD_SIZE; i++) {
        // Draw Dino on top row at position i
        LCD_Move(1, i);
        LCD_Write(2); // Dino character
        Wait_ms(400);
        LCD_Move(1,i);
        LCD_Write(3);
        Wait_ms(200); // adjust speed of animation
        LCD_Move(1,i);
        LCD_Write(32);
    }
}

// Update speed every 1 second
void update_speed() {
    if(T2 >= time_between_speed_increase) {       // 3000 ms = 3 second
        if(wait_time > 30) // don't go too fast
            wait_time -= 10;
        time_between_speed_increase = time_between_speed_increase + 3000;
    }
    Wait_ms(wait_time);
}

unsigned char check_collision() {
    if(!is_jumping && world[DINO_POS] == 1)
        return 1;   // HIT!
    return 0;       // safe
}

void end_game(){
    seconds = T2 / 100;
    HIT = 1;	// trigger game-over tone in Timer3 Interrupt while writing game over
	LCD_Inst(1);
    LCD_Move(0,0); 
    unsigned int i; 
    for (i=0; i<20; i++) {
        LCD_Write(MSG0[i]); //display game over
    }

    HIT = 0;
	Wait_ms(100);

	HIT = 1;
    Wait_ms(50);
    HIT = 0;
    
    Wait_ms(1000);
    LCD_Inst(1);
    LCD_Move(0,0);
    for (i=0; i<20; i++) {
        LCD_Write(MSG1[i]); //display score
    }

    LCD_Move(0,10);
    LCD_Out(score, 5, 0);

	LCD_Move(1,0);
    for (i=0; i<20; i++) {
        LCD_Write(MSG2[i]); //display time
    }

	Wait_ms(100);

    LCD_Move(1,10);
    LCD_Out(seconds, 5, 0);
    
	HIT = 0;
    game_running = 0;
    T2 = 0;
    score = 0;

    while(!RB0);

    init_world();
    is_jumping = 0;
    jump_timer = 0;
    wait_time = 300;
    last_was_cactus = 0;
    T2 = 0;
    game_running = 1;
}

// Interrupts
void interrupt IntServe(void)
{
    if (INT1IF) {
        jump();
        INT1IF = 0;
    }

    if (TMR1IF) {
        // output start/jump tone when player starts game or jumps (presses RB1)
        TMR1 = -C5;
        if (RB1) RC1 = !RC1;
        TMR1IF = 0;
    }

    if (TMR2IF) {
        T2++;          // count milliseconds
        TMR2IF = 0;
    }

    if (TMR3IF) {
        TMR3 = -G3;
        if (HIT) RC2 = !RC2;
        TMR3IF = 0;
    }
}

void main(void)
{
    TRISA = 0;
    TRISB = 0xFF;   // input buttons
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    ADCON1 = 0x0F;

    // set up Timer1 for PS = 1
    TMR1CS = 0;
    T1CON = 0x81;
    TMR1ON = 1;
    TMR1IE = 1;
    TMR1IP = 1;
    PEIE = 1;

    // Timer2: 1ms overflow
    T2CON = 0x4D;
    PR2 = 249;
    TMR2IE = 1;
    TMR2IP = 1;

    // set up Timer3 for PS = 1
    TMR3CS = 0;
    T3CON = 0x81;
    TMR3ON = 1;
    TMR3IE = 1;
    TMR3IP = 1;
    PEIE = 1;

    // Enable INT1 (RB1)
    INT1IE = 1;
    TRISB1 = 1;
    INTEDG1 = 1;

    PEIE = 1;
    GIE = 1;

    LCD_Inst(0x0C);
    LCD_CustomChar(0, DINO);
    LCD_CustomChar(1, CACTUS);
    LCD_CustomChar(2, DINORUNNINGL);
    LCD_CustomChar(3, DINORUNNINGR);

    
    display_opening_animation();
    

    init_world();
    display_world();



    T2 = 0;
    is_jumping = 0;
    jump_timer = 0;
    wait_time = 300;
    game_running = 1;
    HIT = 0;

    while(1){
        while(game_running){
            scroll_world();
            update_jump();
            display_world();
            if(check_collision()){
                end_game();
            }
            if(world[DINO_POS] == 1 && is_jumping) {
                score++;  // Dino safely passed a cactus
            }
            update_speed();
        }
    }   
}
