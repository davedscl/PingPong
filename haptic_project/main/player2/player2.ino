// Pins
int pwmPin = 3; // PWM output pin for motor
int dirPin = 12; // direction output pin for motor
int sensorPosPin = A2; // input pin for MR sensor
int buttonPin = 13;

// Absolute Position Variables
int min_value = 0;
int max_value = 1000;
int current_position = 0.0;
int updated_position = 0.0;
int initial_position = 0.0;
bool flipped = false;
int flips = 0;

// Handle Position Variables
double current_handle_position = 0.0;
double last_handle_position = 0.0;
double save_handle_position = 0.0;

// Handle Velocity Variables
double current_handle_velocity = 0.0;
double last_handle_velocity = 0.0;
double last_last_handle_velocity = 0.0;

// Constants for force-torque conversion
double rp = 0.004191;
double rs = 0.073152;
double rh = 0.065659;

// time variable
int time = 0;

// Force output variables
double force = 0;           // force at the handle
double Tp = 0;              // torque of the motor pulley
double duty = 0;            // duty cylce (between 0 and 255)
unsigned int output = 0;    // output command to the motor

double wallConstant =2.1;

void setup() {
  
  // Set up serial communication
  Serial.begin(57600);

  // Set PWM frequency
  setPwmFrequency(pwmPin, 1);
  // Input pins
  pinMode(sensorPosPin, INPUT); // set MR sensor pin to be an input
  pinMode(buttonPin ,INPUT); //set buttonPin to be an input

  // Output pins
  pinMode(pwmPin, OUTPUT);  // PWM pin for motor
  pinMode(dirPin, OUTPUT);  // dir pin for motor

  // Initialize motor
  analogWrite(pwmPin, 0);     // set to not be spinning (0/255)
  digitalWrite(dirPin, LOW);  // set direction

  // Initialize position valiables
  current_position = analogRead(sensorPosPin);
  updated_position = current_position; 
  initial_position = updated_position;
}


void update_min_max(int position){

  if(position < min_value){

    min_value = position;
    
  }else if(position > max_value){

    max_value = position;
    
  }
}

double result = 0.0;

void read_sensor(){
  
  int new_position = analogRead(sensorPosPin);

  // update if new min or max occured
  update_min_max(new_position);
  
  int current_difference = new_position - current_position;
  int current_distance = abs(current_difference);

  int max_distance = max_value - min_value;
  int threshold = max_distance * 0.3;

  // Update tracking variables.
  current_position = new_position;

  if (current_distance > threshold && !flipped) {
    if (current_difference > 0) {
      // Clockwise rotation.
      flips--;
    } else {
      // Counter-clockwise rotation.
      flips++;
    }

    flipped = true;
  } else {
    flipped = false;
  }

  updated_position = new_position + flips * max_distance - initial_position;


  const double hun_deg = 7215;

  double deg = (updated_position / hun_deg) * 100;

  double rad = deg * PI / 180.0;

  result = rh * rad;
  
}

void motorControl(double force)
{
  Tp = rp / rs * rh * force;  // Compute the require motor pulley torque (Tp) to generate that force
  // Determine correct direction for motor torque
  // You may need to reverse the digitalWrite functions according to your motor connections
  if (force < 0) {
    digitalWrite(dirPin, HIGH);
  } else {
    digitalWrite(dirPin, LOW);
  }

  // Compute the duty cycle required to generate Tp (torque at the motor pulley)
  duty = sqrt(abs(Tp) / 0.03);

  // Make sure the duty cycle is between 0 and 100%
  if (duty > 1) {
    duty = 1;
  } else if (duty < 0) {
    duty = 0;
  }
  output = (int)(duty * 255);  // convert duty cycle to output signal
  analogWrite(pwmPin, output); // output the signal
}

/*
    pos2deg(); return angle in degrees
*/
double pos2deg(double abs_pos){

  const double hun_deg = 7215;

  return (abs_pos / hun_deg) * 100;
 
}

/*
    pos2deg(); return angle in radiant
*/
double deg2rad(double deg) {
  
  return deg * PI / 180.0;
  
}

/*
    calculate_handle_position(); calculates handle postion; argument angle in radiant
*/
double calculate_handle_position(double rad){

  // rh = handle radius in meter
  return rh * rad;
  
}

/*
    calculate_handle_velocity(); returns handle velocity in m/s; formula from: https://hapkit.stanford.edu/files/HapkitLab4SolutionsForWebsite.ino
*/
double calculate_handle_velocity(){

  current_handle_velocity = -(0.95 * 0.95) * last_last_handle_velocity + 2 * 0.95 * last_handle_velocity + (1 - 0.95) * (1 - 0.95) * (current_handle_position - last_handle_position) / 0.0001;
  
  last_handle_position = current_handle_position;
  last_handle_velocity = current_handle_velocity;
  last_last_handle_velocity = last_handle_velocity;

  return current_handle_velocity;
  
}

void vibration(){

  digitalWrite(dirPin, LOW);  // set direction
  analogWrite(pwmPin, 150);
  delay(10);
  analogWrite(pwmPin, 0);
  delay(10);
  digitalWrite(dirPin, HIGH);  // set direction
  analogWrite(pwmPin, 150);
  delay(10);
  analogWrite(pwmPin, 0);
  
}

void wall(double handle_position, int incomingByte){

  if(incomingByte == 252){
    motorControl(wallConstant-handle_position);
  }else{
    motorControl(-wallConstant+handle_position);
  }

}

/*
   setPwmFrequency
*/
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if (pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch (divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if (pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if (pin == 3 || pin == 11) {
    switch (divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

void loop() {
  
  // read the position in count
  read_sensor();

  time = millis();
  
  double angle_in_deg = pos2deg(updated_position);
  double angle_in_rad = deg2rad(angle_in_deg);

  // callculate handle position in meters
  current_handle_position = calculate_handle_position(angle_in_rad);
  
  // calculate handle velocity in m/s
  double handle_velocity = calculate_handle_velocity();

  // current handle position in millimeter
  double current_handle_position_mm = current_handle_position * 1000;

  if(time % 100 == 0){

    Serial.println(current_handle_position_mm);
    
  }

  int incomingByte = 0;

  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    if(incomingByte == 254){

      vibration();
      
    } else if(incomingByte == 253 || incomingByte == 252){


      wall(current_handle_position,incomingByte);
      
    } else {

      analogWrite(pwmPin, 0);
      
    }

    //motorControl(0.0);
  }

}
