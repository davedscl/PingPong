import processing.serial.*;
import processing.sound.*;

// port
String port_name_p1 = "";
String port_name_p2 = "";
Serial port_p1;
Serial port_p2;

// image variables
PImage ball, background, player;

// player positions
float player1_position_read = 0.0;
float player2_position_read = 0.0;
float player1_position = 0.0;
float player2_position = 0.0;

float p1_position_read = 0.0;
float p2_position_read = 0.0;

// ball variables
float ball_x = 0.0;
float ball_y = 0.0;

// score variables
int player1_score = 0;
int player2_score = 0;

// sound variables
SoundFile ping_sound;
SoundFile pong_sound;
SoundFile win_sound;

float vertSpeed, horiSpeed;

int count = 0;


void setup(){ 
  
  // window size
  size(900,800);
  imageMode(CENTER);
  
  ball = loadImage("../ball.png");
  background = loadImage("../back.png");
  player = loadImage("../player.png");
  
  ping_sound = new SoundFile(this, "../ping.mp3");
  pong_sound = new SoundFile(this, "../pong.mp3");
  win_sound = new SoundFile(this, "../win.mp3");

  String paddle_p1 = "AG0JR280";
  String paddle_p2 = "AG0JQQNH";
  
  // intialize player1 connection
  if(port_name_p1.equals("")){
    port_p1 = scanForArduino(paddle_p1, port_p1);
  } else {
    port_p1 = new Serial(this, port_name_p1, 57600);
  }
  
  // intialize player2 connection
  if(port_name_p2.equals("")){
    port_p2 = scanForArduino(paddle_p2, port_p2);
  } else {
    port_p2 = new Serial(this, port_name_p1, 57600);
  }
  
  player1_position = width/2;
  player2_position = width/2;
  
  player1_score = 0;
  player2_score = 0;
  
  resetBall(1);
  
  //port.bufferUntil(10); 

}

void draw(){ // same as loop in arduino

  // draw background
  image(background, width/2, height/2, width, height);
  
  // read player positions
  read_player1_position(port_p1);
  read_player2_position(port_p2);
  
  //text("received: " + p1_position_read, 10,50);
  //text("received: " + p2_position_read, 800,50);
  
  // calculate graphical positions
  player1_position = calculate_player_position(p1_position_read, port_p1);
  player2_position = calculate_player_position(p2_position_read, port_p2);
    
  // draw players
  image(player, player1_position, height - player.height);
  image(player, player2_position, 0 + player.height);
  
  
  text(player1_score + " : " + player2_score , 10,height/2);
  
  if((player1_score == 10) || (player2_score == 10)){
    
    textSize(30);
    if(player1_score == 10){ 
      text("Player 1 won", 350, 400);
    } else {
      text("Player 2 won", 350, 400);
    }
    textSize(12);
    
    if(count == 0){
      win_sound.play();
      count++;
    }
    
    
  } else {
    
    // Calculate new position of ball - being sure to keep it on screen
    ball_x = ball_x + horiSpeed;
    ball_y = ball_y + vertSpeed;
  
    // ball is bellow bottom
    if(ball_y >= height){
      resetBall(1);
      player2_score += 1;
    }
    
    // ball is above top
    if(ball_y <= 0){
      resetBall(-1);
      player1_score += 1;
    }
    
    // if ball touches right wall
    if(ball_x >= width){
      wallBounce();
    }
    
    // if ball touches left wall
    if(ball_x <= 0){
      wallBounce();
    }
  
    // Draw the ball in the correct position and orientation
    translate(ball_x,ball_y);
    if(vertSpeed > 0) rotate(-sin(horiSpeed/vertSpeed));
    else rotate(PI-sin(horiSpeed/vertSpeed));
    image(ball,0,0);
    
    // Do collision detection between bat and ball
    if(player1TouchingBall()) {
      float distFromBatCenter = player1_position-ball_x;
      horiSpeed = -distFromBatCenter/10;
      vertSpeed = -vertSpeed;
      ball_y = height-(player.height*2);
      
      // play sound
      ping_sound.play();
      
      // write to arduino to vibrate on touch
      port_p1.write(254); 
    
    }
  
 
  }

  if(player2TouchingBall()) {
    float distFromBatCenter = player2_position-ball_x;
    horiSpeed = -distFromBatCenter/10;
    vertSpeed = -vertSpeed;
    ball_y = 0+(player.height*2);
    
    // play sound
    pong_sound.play();
    
    // write to arduino to vibrate on touch
    port_p2.write(254);
  }
  
}

void read_player1_position(Serial port){
  
  // read player position from serial
  if((port != null) && (port.available()>0)) {
    
    // read message from serial port
    String message = port.readStringUntil('\n');
    
    if(message != null) {
      // cast message to float
      p1_position_read = float(message);
      
    } 
  }
}

void read_player2_position(Serial port){
  
  // read player position from serial
  if((port != null) && (port.available()>0)) {
    
    // read message from serial port
    String message = port.readStringUntil('\n');
    
    if(message != null) {
      // cast message to float
      p2_position_read = float(message);
      
    } 
  }
}


float calculate_player_position(float position_read, Serial port){
 
  float player_position = position_read * 10 + 500;
  
  // player hit left wall
  if((player_position - player.width/2) < 0.0){
   
    player_position = 0.0 + player.width/2;
    
    port.write(252);
    
  // player hit right wall
  } else if((player_position + player.width/2) > width){
    
     player_position = width - player.width/2;
     
     port.write(253);
    
  } else {
   
     port.write(251);  
    
  }
  
  return player_position;
  
}


void resetBall(int direction){
  
  ball_x = width/2 - ball.width/2;
  ball_y = height/2 - ball.height/2;
  vertSpeed = 20 * direction;
  horiSpeed = 1 * direction;
}


void wallBounce()
{
  horiSpeed = -horiSpeed;
}



boolean player1TouchingBall() {
  float distFromBatCenter = player1_position-ball_x;
  return (ball_y > height-(player.height*2)) && (ball_y < height-(player.height/2)) && (abs(distFromBatCenter)<player.width/2);
}

boolean player2TouchingBall() {
  float distFromBatCenter = player2_position-ball_x;
  return (ball_y < 0+(player.height*2)) && (ball_y > 0+(player.height/2)) && (abs(distFromBatCenter)<player.width/2);
}



Serial scanForArduino(String playername, Serial port){
  
  try {
    
    for(int i=0; i < Serial.list().length ;i++) {
      //println(Serial.list()[i]);
      if(Serial.list()[i].contains(playername)) {
        port = new Serial(this, Serial.list()[i], 57600);
      }
    }
    
  } catch(Exception e) {
    
    println("Cannot connect to Arduino !");
    println(e.getMessage());
    
  }
  
  println("Connected Player: " + playername + " :)");
  
  return port;
}
