#if 1

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

#define MINPRESSURE 200
#define MAXPRESSURE 1000

// ALL Touch panels and wiring is DIFFERENT
// Calibrate using TouchScreen_Calibr_native.ino
// copy-paste results from TouchScreen_Calibr_native.ino in MCUFRIEND_kbv library
const int XP = 6, XM = A2, YP = A1, YM = 7; //ID=0x9341
const int TS_LEFT = 855, TS_RT = 192, TS_TOP = 925, TS_BOT = 139; //portrait

// Initialising objects
MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Define button variables
Adafruit_GFX_Button one_btn, two_btn, three_btn, four_btn, five_btn, six_btn, seven_btn, eight_btn, nine_btn, 
asterisk_btn, zero_btn, hashtag_btn, call_btn, delete_btn, plus_btn, phone_btn, sms_btn, end_call_btn, answer_call_btn, 
decline_call_btn, home_btn, send_msg_btn, a_btn, b_btn, c_btn, d_btn, e_btn, f_btn, g_btn, h_btn, i_btn, j_btn, k_btn,
l_btn, m_btn, n_btn, o_btn, p_btn, q_btn, r_btn, s_btn, t_btn, u_btn, v_btn, w_btn, x_btn, y_btn, z_btn, space_btn, capital_btn,
other_char_btn, return_btn, next_btn, mute_btn;

int pixel_x, pixel_y;     //Touch_getXY() updates global vars

// Function which detects touch inputs on a touchscreen and retrieves the x and y coordinates of the touch point
bool Touch_getXY(void) {
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
        Serial2.print(pixel_x);
        Serial2.print(pixel_y);
    }
    return pressed;
}

// Define Colours
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0xED8D
#define PURPLE  0xF352

int x = 12; // Set x coordinate for starting cursor position for phone number entry & messages
int y = 65; // Set y coordinate for starting cursor position for phone number entry
int y_message = 75; // Set y coordinate for starting cursor position for messages

char Phone_Number[12] = {}; // Set phone number to have length of 12 characters (must start with country code e.g. Australia +61)
int phone_digits = 0; // initialise phone number digits indexing

char message_words[90] = {}; // Set message character to be sent. Max 90 characters long
int message_char = 0; // initialise message characters indexing

char Sender_Phone_Number[12] = {}; // set to store phone number of the sender

char Sender_Message_Words[90] = {}; // set to store received message from sender
char Short_Sender_Message[16] = {}; // set to display part of the recieved message in a notification

int screen_number = 1; // Set the initial screen number to be 1 to indicate home screen
int prev_screen_number = screen_number; // initialise previous screen to home screen

bool mic_mute = false; // Set mic to unmute

// Set screens to unrendered
bool home_screen_rendered = false;
bool keypad_screen_rendered = false;
bool call_screen_rendered = false;
bool incoming_call_screen_rendered = false;
bool phone_number_rendered = false;
bool keyboard_screen_rendered = false;
bool message_screen_rendered = false;
bool sent_sms_screen_rendered = false;
bool sms_notification_rendered = false;
bool message_viewer_rendered = false;

void setup(void) {
    Serial.begin(115200);   
    Serial.println("****************************");
    Serial.println("using serial1 & serial2 v 10");
    Serial.println("****************************");
    Serial2.begin(115200);

    uint16_t ID = tft.readID(); // Read the ID of the tft
    if (ID == 0xD3D3) ID = 0x9486; // write-only shield
    tft.begin(ID);
    tft.setRotation(0);  //PORTRAIT
    tft.fillScreen(BLACK);
    home_screen_processor(); // Load home screen      
    
    delay(2000);

    // AT Command to enable CLIP. 
    // This command allows the arduino board to about the phone number of the incoming caller.
    Serial2.write("AT+ClIP=1\r\n");
    Serial.println(_readSerial());
    delay(200);
   
    // AT Command to check CLIP returns OK
    Serial2.write("AT+ClIP?\r\n");
    Serial.println(_readSerial());
    delay(200);
}

void loop(void) {
   if (Serial2.available()) {
        String gsmResponse = "";
        while (Serial2.available()) {
          gsmResponse = Serial2.readStringUntil('\n');
          Serial.println(gsmResponse);
        }
        
        // If the gsm reponse starts with +CLIP, retrieve & store phone number in a string
        if (gsmResponse.startsWith("+CLIP:")) {
            int start_Index = gsmResponse.indexOf('\"');
            int end_Index = gsmResponse.indexOf('\"', start_Index+1);
            String caller_Number = gsmResponse.substring(start_Index+1, end_Index);
            
            if (start_Index != -1 && end_Index != -1) {
                strcpy(Phone_Number, caller_Number.c_str());
            }
        
            Serial.print("start index: ");
            Serial.println(start_Index);
            Serial.print("End index: ");
            Serial.println(end_Index);
            Serial.println("$$$$$$$$$$$");
            Serial.print("Caller Number: ");
            Serial.println(Phone_Number);
        } 
        
        // If the gsm reponse starts with RING, display incoming call screen
        if (gsmResponse.startsWith("RING")) {
            /*
            // AT command to set it to speaker
             Serial2.write("AT+CSDVC=3\r\n");
             Serial.println(_readSerial());
             delay(100);
            */
            Serial.print("prev screen number before: ");
            Serial.println(prev_screen_number);
            if (screen_number != 4) {
                prev_screen_number = screen_number;
                screen_number = 4;
            }
            Serial.print("prev screen number after: ");
            Serial.println(prev_screen_number);
        }

        // If the gsm reponse starts with VOICE CALL: END:, return to screen before call
        if (gsmResponse.startsWith("VOICE CALL: END:")) {
            Serial.println("&&&& Entered &&&&");
            Serial.print("Phone_number before ");
            Serial.println(Phone_Number);
            memset(Phone_Number, 0, sizeof(Phone_Number)); 
            phone_number_rendered = false;
            if (prev_screen_number == 2) {
                screen_number = 2;
                keypad_screen_rendered = false;
            } else {
                screen_number = 1;
                home_screen_rendered = false;
            }
            incoming_call_screen_rendered = false;
            Serial.print("Phone_number after ");
            Serial.println(Phone_Number);
        }

        // If the gsm reponse starts with MISSED_CALL:, return to screen before call
        if (gsmResponse.startsWith("MISSED_CALL:")) {
            Serial.print("Phone_number before ");
            Serial.println(Phone_Number);
            memset(Phone_Number, 0, sizeof(Phone_Number));
            phone_number_rendered = false; 
            if (prev_screen_number == 2) {
                screen_number = 2;
                keypad_screen_rendered = false;
            } else {
                screen_number = 1;
                home_screen_rendered = false;
            }
            incoming_call_screen_rendered = false;
            Serial.print("Phone_number after ");
            Serial.println(Phone_Number);
        }

        // If the gsm reponse starts with +CMTI:, retrive & store message in a string & display the message as a pop up notification
        if (gsmResponse.startsWith("+CMTI:")) {
            //get mem and message index
            //+CMTI: "SM",8
            char start_mem_index = gsmResponse.indexOf('\"');
            char end_mem_index = gsmResponse.indexOf('\"', start_mem_index+1);
            String sms_mem_location = gsmResponse.substring(start_mem_index+1, end_mem_index);  
            char start_sms_location = gsmResponse.indexOf(',');
            char end_sms_location = gsmResponse.indexOf(' ', start_sms_location+1);
            String sms_index = gsmResponse.substring(start_sms_location+1, end_sms_location);
            
            Serial.println(sms_mem_location);
            Serial.println(sms_index);
            
            // AT command to set message format to text mode
            Serial2.write("AT+CMGF=1\r");
            delay(100);
            Serial.println(_readSerial());

            // AT command to set preferred SMS message storage
            Serial2.write("AT+CPMS=\"");
            Serial2.write(sms_mem_location.c_str());
            Serial2.write("\"\r");
            delay(100);
            Serial.println(_readSerial());

            // AT comamnd to read the message
            Serial2.write("AT+CMGR=");
            Serial2.write(sms_index.c_str());
            Serial2.write("\r");
            //delay(100);
            gsmResponse = _readSerial();
            Serial.println("reading message");

            Serial.println(gsmResponse);
            Serial.print("startsWith AT+CMGR=? ");        
            Serial.println(gsmResponse.startsWith("AT+CMGR="));

            // If the gsm reponse starts with AT+CMGR=, 
            if (gsmResponse.startsWith("AT+CMGR=")) {
              // Get sender's phone number
              char start_sender_number = gsmResponse.indexOf(',');
              char end_sender_number = gsmResponse.indexOf(',', start_sender_number+2);
              String sender_number = gsmResponse.substring(start_sender_number+2, end_sender_number-1);
              Serial.println("@@@@@@@@@@");
              Serial.println(sender_number);

              // Store sender's phone number
              if (start_sender_number != -1 && end_sender_number != -1) {
                strcpy(Sender_Phone_Number, sender_number.c_str());
              }

              Serial.println("############");
              Serial.println(Sender_Phone_Number);
              
              //get the sms text
              char start_sender_message = gsmResponse.indexOf('\n');
              char end_sender_message = gsmResponse.indexOf('\n', start_sender_message+1);
              char remove_extra_carriage = gsmResponse.lastIndexOf("OK");
              String sender_message = gsmResponse.substring(end_sender_message+1, remove_extra_carriage-1);
              Serial.println("#### start ######");
              Serial.println(sender_message);
              Serial.println("##### end #####");

              // Store the SMS Message Text
              if (start_sender_message != -1 && end_sender_message != -1 && remove_extra_carriage != -1) {
                strcpy(Sender_Message_Words, sender_message.c_str());
              }
              Serial.println("#### start ######");
              Serial.println(Sender_Message_Words);
              Serial.println("##### end #####");

              // Display sms pop up notification on current screen
              if (screen_number != 8 || screen_number != 3 || screen_number != 4 || screen_number != 7 || screen_number != 9) {
                prev_screen_number = screen_number;
                screen_number = 8;
              }
            }
        }
    } else {
          //Serial.println("Serial2 is not Available");
    }
    
    // If screen_number = 1, display home screen
    if (screen_number == 1) {
        home_screen_processor();
    }
  
    // If screen_number = 2, display phone call app screen  
    if (screen_number == 2) {
        keypad_screen_processor();
    }

    // If screen_number = 3, display during call screen
    if (screen_number == 3) {
        call_screen_processor();
    }

    // If screen_number = 4, display incoming call screen
    if (screen_number == 4) {
        incoming_call_screen_processor();
    }
      
    // If screen_number = 5, display phone number to message screen  
    if (screen_number == 5) {
        keyboard_screen_processor();  
    }

    // If screen_number = 6, display message app screen
    if (screen_number == 6) {
        message_screen_processor();
    }

    // If screen_number = 7, display sent message screen
    if (screen_number == 7) {
        sent_sms_screen_processor();
    }

    // If screen_number = 8, display sms notification on current screen
    if (screen_number == 8) {
        sms_notification_processor();
    }

    // If screen_number = 9, display message viewer screen
    if (screen_number == 9) {
        message_viewer_processor();          
    }
    delay(50); 
}

// Cursor position when typing phone number
void Cursor_Position(int x) {
    if (x < 12) {
        tft.setCursor(12,y);
    } else if (x > 210) {
        tft.setCursor(210,y);
    } else {
        tft.setCursor(x,y);
    } 
}

// Cursor position when typing message
int Message_Cursor_Position(int x) {
    if (x < 12) {
        tft.setCursor(12,y_message);
    } else if (x > 216) {
        x = 12;
        if (y_message < 135) {
            y_message += 15;
        }    
        tft.setCursor(x,y_message);
        return x;
    } else {  
        tft.setCursor(x,y_message);
    } 
}

// Function to Dial a phone number
void Call_Number() {
    Serial.print("calling number ");
    Serial.println(Phone_Number);
    //Serial.println("Check if SIM card is present:");

    // Checks if the device is responding and ready to receive further commands
    // Should return OK
    Serial2.write("AT\r\n");
    Serial.println("after AT");
    Serial.println(_readSerial());
    Serial.println("after AT read serial");
    
    delay(100);

    // AT command to check network registration status
    // Should return OK
    Serial2.write("AT+CREG?\r\n");
    Serial.println("after AT+CREG");
    Serial.println(_readSerial());
    Serial.println("after AT+CREG read serial");
    delay(1000);

    // AT command to check the status of the SIM card's PIN (Personal Identification Number)
    // Should return OK
    Serial2.write("AT+CPIN?\r\n");
    Serial.println(_readSerial());
    delay(1000);

    Serial.print("Number to call ->(");
    Serial.print(Phone_Number);
    Serial.println(")");
    
    // AT command to call number
    Serial2.write("ATD");
    Serial2.write((char*) Phone_Number);
    Serial2.write(";\r\n");
   
    Serial.println(_readSerial());

    delay(100);
}

// Function to end phone call
void End_Call() {
    Serial.println("Ending Call");
    // AT command to hang up call
    Serial2.write("AT+CHUP\r\n");
    //Serial.println(_readSerial());
    delay(1000);
}

// Function to answer phone call
void Answer_Call() {
    Serial.println("Answering Call");
    // AT command to answer call
    Serial2.write("ATA\r\n");
    Serial.println(_readSerial());
    delay(1000);
}

// Function to decline incoming call
void Decline_Call() {
    Serial.println("Declining Call");
    // AT command to choose voice hang up control and allow ATH to disconnect the call
    Serial2.write("AT+CVHU=0\r\n");
    Serial.println(_readSerial());
    delay(1000);
    // AT command to disconnect the call
    Serial2.write("ATH\r\n");
    Serial.println(_readSerial());
    delay(1000);
}

// Function to read data from Serial2 serial port & retrieve a complete string of data sent to serial.
String _readSerial() {
  int _timeout = 0;
  while  (!Serial2.available() && _timeout < 200) {
    delay(13);
    _timeout++;
  }
  
  if (Serial2.available()) {
      return Serial2.readString();      
  }
}

// Function to return to the previous screen number after viewing notifications
void change_screen_number(int screen_number) {
    if (screen_number == 1) {
        home_screen_rendered = false;
    } else if (screen_number == 2) {
        keypad_screen_rendered = false;
    } else if (screen_number == 5) {
        keyboard_screen_rendered = false;  
    } else if (screen_number == 6) {
        message_screen_rendered = false;
    }
}

// Function to process the homescreen
void home_screen_processor() {
    if (!home_screen_rendered) {
        tft.setRotation(0);  //PORTRAIT
        tft.fillScreen(BLACK);
        phone_btn.initButton(&tft, 50, 135, 95, 40, WHITE, CYAN, BLACK, "Phone", 3);
        sms_btn.initButton(&tft, 180, 135, 95, 40, WHITE, CYAN, BLACK, "SMS", 3);

        phone_btn.drawButton(false);
        sms_btn.drawButton(false);

        home_screen_rendered = true;
    }

    bool down = Touch_getXY();
    phone_btn.press(down && phone_btn.contains(pixel_x, pixel_y));
    sms_btn.press(down && sms_btn.contains(pixel_x, pixel_y));

    if (phone_btn.justReleased()) {
        phone_btn.drawButton();
    }
  
    if (phone_btn.justPressed()) {
        phone_btn.drawButton(true);
        prev_screen_number = screen_number;
        screen_number = 2;
        keypad_screen_rendered = false;  
    }

    if (sms_btn.justReleased()) {
        sms_btn.drawButton();
    }
  
    if (sms_btn.justPressed()) {
        sms_btn.drawButton(true);
        screen_number = 5;
          
    }
}

// Function to process the phone call app screen
void keypad_screen_processor() {  
    if (!keypad_screen_rendered) {
        tft.setRotation(0);  //PORTRAIT
        tft.fillScreen(BLACK);
        x = 12;
        tft.setCursor(x,y);
        Phone_Number[12] = {};
        phone_digits = 0;

        one_btn.initButton(&tft, 40, 135, 70, 40, WHITE, CYAN, BLACK, "1", 3);
        two_btn.initButton(&tft, 120, 135, 70, 40, WHITE, CYAN, BLACK, "2", 3);
        three_btn.initButton(&tft, 200, 135, 70, 40, WHITE, CYAN, BLACK, "3", 3);
        four_btn.initButton(&tft, 40, 185, 70, 40, WHITE, CYAN, BLACK, "4", 3);
        five_btn.initButton(&tft, 120, 185, 70, 40, WHITE, CYAN, BLACK, "5", 3);
        six_btn.initButton(&tft, 200, 185, 70, 40, WHITE, CYAN, BLACK, "6", 3);
        seven_btn.initButton(&tft, 40, 235, 70, 40, WHITE, CYAN, BLACK, "7", 3);
        eight_btn.initButton(&tft, 120, 235, 70, 40, WHITE, CYAN, BLACK, "8", 3);
        nine_btn.initButton(&tft, 200, 235, 70, 40, WHITE, CYAN, BLACK, "9", 3);
        asterisk_btn.initButton(&tft, 40, 285, 70, 40, WHITE, CYAN, BLACK, "*", 3);
        zero_btn.initButton(&tft, 120, 285, 70, 40, WHITE, CYAN, BLACK, "0", 3);
        hashtag_btn.initButton(&tft, 200, 285, 70, 40, WHITE, CYAN, BLACK, "#", 3);
        call_btn.initButton(&tft, 122, 335, 80, 40, GREEN, GREEN, BLACK, "Call", 3);
        delete_btn.initButton(&tft, 203, 335, 60, 40, RED, RED, BLACK, "X", 3);
        plus_btn.initButton(&tft, 40, 335, 70, 40, WHITE, CYAN, BLACK, "+", 3);
        home_btn.initButton(&tft, 120, 380, 65, 40, WHITE, CYAN, BLACK, "<-", 3);

        one_btn.drawButton(false);
        two_btn.drawButton(false);
        three_btn.drawButton(false);
        four_btn.drawButton(false);
        five_btn.drawButton(false);
        six_btn.drawButton(false);
        seven_btn.drawButton(false);
        eight_btn.drawButton(false);
        nine_btn.drawButton(false);
        asterisk_btn.drawButton(false);
        zero_btn.drawButton(false);
        hashtag_btn.drawButton(false);
        call_btn.drawButton(false);
        delete_btn.drawButton(false);
        plus_btn.drawButton(false);
        home_btn.drawButton(false);
    
        tft.drawRect(5, 50, 230, 45, CYAN);
        
        keypad_screen_rendered = true;
    }

    bool down = Touch_getXY();
    one_btn.press(down && one_btn.contains(pixel_x, pixel_y));
    two_btn.press(down && two_btn.contains(pixel_x, pixel_y));
    three_btn.press(down && three_btn.contains(pixel_x, pixel_y));
    four_btn.press(down && four_btn.contains(pixel_x, pixel_y));
    five_btn.press(down && five_btn.contains(pixel_x, pixel_y));
    six_btn.press(down && six_btn.contains(pixel_x, pixel_y));
    seven_btn.press(down && seven_btn.contains(pixel_x, pixel_y));
    eight_btn.press(down && eight_btn.contains(pixel_x, pixel_y));
    nine_btn.press(down && nine_btn.contains(pixel_x, pixel_y));
    asterisk_btn.press(down && asterisk_btn.contains(pixel_x, pixel_y));
    zero_btn.press(down && zero_btn.contains(pixel_x, pixel_y));
    hashtag_btn.press(down && hashtag_btn.contains(pixel_x, pixel_y));
    call_btn.press(down && call_btn.contains(pixel_x, pixel_y));
    delete_btn.press(down && delete_btn.contains(pixel_x, pixel_y));
    plus_btn.press(down && plus_btn.contains(pixel_x, pixel_y));
    home_btn.press(down && home_btn.contains(pixel_x, pixel_y));

    if (one_btn.justReleased()) {
        one_btn.drawButton();
    }
  
    if (one_btn.justPressed()) {
        one_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("1");
            Phone_Number[phone_digits] = 49;
            phone_digits++;
            x += 18;
        }   
    }

    if (two_btn.justReleased()) {
        two_btn.drawButton();
    }
  
    if (two_btn.justPressed()) {
        two_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("2");
            Phone_Number[phone_digits] = 50;
            phone_digits++;
            x += 18;
        }   
    }

    if (three_btn.justReleased()) {
        three_btn.drawButton();
    }
  
    if (three_btn.justPressed()) {
        three_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("3");
            Phone_Number[phone_digits] = 51;
            phone_digits++;
            x += 18;
        }    
    }

    if (four_btn.justReleased()) {
        four_btn.drawButton();
    }
  
    if (four_btn.justPressed()) {
        four_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("4");
            Phone_Number[phone_digits] = 52;
            phone_digits++;
            x += 18;
        }   
    }

    if (five_btn.justReleased()) {
        five_btn.drawButton();
    }
  
    if (five_btn.justPressed()) {
        five_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("5");
            Phone_Number[phone_digits] = 53;
            phone_digits++;
            x += 18;
        }  
    }

    if (six_btn.justReleased()) {
        six_btn.drawButton();
    }
  
    if (six_btn.justPressed()) {
        six_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("6");
            Phone_Number[phone_digits] = 54;
            phone_digits++;
            x += 18;
        }   
    }

    if (seven_btn.justReleased()) {
        seven_btn.drawButton();
    }
  
    if (seven_btn.justPressed()) {
        seven_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("7");
            Phone_Number[phone_digits] = 55;
            phone_digits++;
            x += 18;
        }   
    }

    if (eight_btn.justReleased()) {
        eight_btn.drawButton();
    }
  
    if (eight_btn.justPressed()) {
        eight_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("8");
            Phone_Number[phone_digits] = 56;
            phone_digits++;
            x += 18;
        }   
    }

    if (nine_btn.justReleased()) {
        nine_btn.drawButton();
    }
  
    if (nine_btn.justPressed()) {
        nine_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("9");
            Phone_Number[phone_digits] = 57;
            phone_digits++;
            x += 18;
        }     
    } 

    if (asterisk_btn.justReleased()) {
        asterisk_btn.drawButton();
    }
  
    if (asterisk_btn.justPressed()) {
        asterisk_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("*");
            Phone_Number[phone_digits] = 42;
            phone_digits++;
            x += 18;
        }        
    }

    if (zero_btn.justReleased()) {
        zero_btn.drawButton();
    }
  
    if (zero_btn.justPressed()) {
        zero_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("0");
            Phone_Number[phone_digits] = 48;
            phone_digits++;
            x += 18;
        }   
    }

    if (hashtag_btn.justReleased()) {
        hashtag_btn.drawButton();
    }
  
    if (hashtag_btn.justPressed()) {
        hashtag_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("#");
            Phone_Number[phone_digits] = 35;
            phone_digits++;
            x += 18;
        }      
    }  

    if (call_btn.justReleased()) {
        call_btn.drawButton();
    }
  
    if (call_btn.justPressed()) {
        call_btn.drawButton(true);
        Serial.println(Phone_Number);
        Serial.println("Number before call_number(): ");
        Serial.println(Phone_Number);
        Call_Number();
        Serial.println("Number After call_number(): ");
        Serial.println(Phone_Number);
        prev_screen_number = screen_number;
        screen_number = 3;
        call_screen_rendered = false;    
    }
 
    if (delete_btn.justReleased()) {
        delete_btn.drawButton();
    }
  
    if (delete_btn.justPressed()) {
        delete_btn.drawButton(true);
        phone_digits--;
        Phone_Number[phone_digits] = 32;
        if (x >= 18 && x <= 228) {
            x -= 18;
            tft.fillRect(x, y, 18, 22, BLACK);
        }    
    }
 
    if (plus_btn.justReleased()) {
        plus_btn.drawButton();
    }
  
    if (plus_btn.justPressed()) {
        plus_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("+");
            Phone_Number[phone_digits] = 43;
            phone_digits++;
            x += 18;
        }
    }

    if (home_btn.justReleased()) {
        home_btn.drawButton();
    }
  
    if (home_btn.justPressed()) {
        home_btn.drawButton(true);
        screen_number = 1;
        home_screen_rendered = false;
        keypad_screen_rendered = false;
    }
}

// Function to process the in call screen
void call_screen_processor() {
    if (!call_screen_rendered) {
        tft.setRotation(0);  //PORTRAIT
        tft.fillScreen(BLACK);

        end_call_btn.initButton(&tft, 122, 335, 80, 40, RED, RED, BLACK, "End", 3);
        end_call_btn.drawButton(false);

        mute_btn.initButton(&tft, 122, 135, 80, 40, BLUE,BLUE, BLACK, "Mute", 3);
        mute_btn.drawButton(false);

        Serial.print("In call screen before ");
        Serial.println(Phone_Number);
        
        tft.setTextColor(WHITE);
        tft.setTextSize(3); 
        tft.setCursor(30,65);
        tft.println(Phone_Number);

        Serial.print("In call screen after ");
        Serial.println(Phone_Number);
        
        call_screen_rendered = true;       
    }

    bool down = Touch_getXY();
    end_call_btn.press(down && end_call_btn.contains(pixel_x, pixel_y));
    mute_btn.press(down && end_call_btn.contains(pixel_x, pixel_y));

    if (end_call_btn.justReleased()) {
        end_call_btn.drawButton();
    }
  
    if (end_call_btn.justPressed()) {
        end_call_btn.drawButton(true);
        End_Call();
        memset(Phone_Number, 0, phone_digits);
        Serial.print("Number after end call: ");
        Serial.println(Phone_Number);
        
        if (prev_screen_number == 2) {
            screen_number = 2;
            keypad_screen_rendered = false;
        } else {
            screen_number = 1;
            home_screen_rendered = false;
        }
    }

    if (mute_btn.justReleased()) {
        mute_btn.drawButton();
    }
  
    if (mute_btn.justPressed()) {
        mute_btn.drawButton(true);
        if (mic_mute == false) {
            mic_mute == true;
            Serial2.write("AT+CMUT=1\r\n");
            Serial.println(_readSerial());
            delay(200);
        } else {
            mic_mute == false;
            Serial2.write("AT+CMUT=0\r\n");
            Serial.println(_readSerial());
            delay(200);
        }
    }
}

// Function to process the incoming call screen
void incoming_call_screen_processor() {
  if (!incoming_call_screen_rendered) {
      Serial.println("incoming call screen processor");
      tft.setRotation(0);  //PORTRAIT
      tft.fillScreen(BLACK);  

      decline_call_btn.initButton(&tft, 60, 335, 100, 40, RED, RED, BLACK, "Decline", 2);
      answer_call_btn.initButton(&tft, 180, 335, 100, 40, GREEN, GREEN, BLACK, "Answer", 2);
      
      decline_call_btn.drawButton(false);
      answer_call_btn.drawButton(false);
      
      incoming_call_screen_rendered = true;
  }
  
  if (strlen(Phone_Number) != 0 && phone_number_rendered == false) {
      tft.setTextColor(WHITE);
      if (strlen(Phone_Number) > 10) {
          tft.setTextSize(2);
      } else {
          tft.setTextSize(3);
      }    
      tft.setCursor(30,65);
      tft.println(Phone_Number);
      phone_number_rendered = true;
  }
  
  bool down = Touch_getXY();
  decline_call_btn.press(down && decline_call_btn.contains(pixel_x, pixel_y));
  answer_call_btn.press(down && answer_call_btn.contains(pixel_x, pixel_y));

  if (decline_call_btn.justReleased()) {
      decline_call_btn.drawButton();
  }
  
  if (decline_call_btn.justPressed()) {
      decline_call_btn.drawButton(true);
      End_Call();
      Serial.println(prev_screen_number);
      Serial.println(screen_number);
      if (prev_screen_number == 2) {
          screen_number = 2;
          keypad_screen_rendered = false;
      } else {
          screen_number = 1;
          home_screen_rendered = false;
      }
      incoming_call_screen_rendered = false;
  }

  if (answer_call_btn.justReleased()) {
      answer_call_btn.drawButton();
  }
  
  if (answer_call_btn.justPressed()) {
      answer_call_btn.drawButton(true);
      Answer_Call();
      screen_number = 3;
      call_screen_rendered = false;
      incoming_call_screen_rendered = false;
  }
}

// Function to process the phone number to message screen
void keyboard_screen_processor() {  
    if (!keyboard_screen_rendered) {
        tft.setRotation(0);  //PORTRAIT
        tft.fillScreen(BLACK);
        tft.setCursor(2,10);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        tft.print("Enter Phone Number To Message:");
        x = 12;
        tft.setCursor(x,y);
        Phone_Number[12] = {};
        phone_digits = 0;
        
        one_btn.initButton(&tft, 40, 135, 70, 40, WHITE, CYAN, BLACK, "1", 3);
        two_btn.initButton(&tft, 120, 135, 70, 40, WHITE, CYAN, BLACK, "2", 3);
        three_btn.initButton(&tft, 200, 135, 70, 40, WHITE, CYAN, BLACK, "3", 3);
        four_btn.initButton(&tft, 40, 185, 70, 40, WHITE, CYAN, BLACK, "4", 3);
        five_btn.initButton(&tft, 120, 185, 70, 40, WHITE, CYAN, BLACK, "5", 3);
        six_btn.initButton(&tft, 200, 185, 70, 40, WHITE, CYAN, BLACK, "6", 3);
        seven_btn.initButton(&tft, 40, 235, 70, 40, WHITE, CYAN, BLACK, "7", 3);
        eight_btn.initButton(&tft, 120, 235, 70, 40, WHITE, CYAN, BLACK, "8", 3);
        nine_btn.initButton(&tft, 200, 235, 70, 40, WHITE, CYAN, BLACK, "9", 3);
        asterisk_btn.initButton(&tft, 40, 285, 70, 40, WHITE, CYAN, BLACK, "*", 3);
        zero_btn.initButton(&tft, 120, 285, 70, 40, WHITE, CYAN, BLACK, "0", 3);
        hashtag_btn.initButton(&tft, 200, 285, 70, 40, WHITE, CYAN, BLACK, "#", 3);
        next_btn.initButton(&tft, 122, 335, 80, 40, GREEN, GREEN, BLACK, "Next", 3);
        delete_btn.initButton(&tft, 203, 335, 60, 40, RED, RED, BLACK, "X", 3);
        plus_btn.initButton(&tft, 40, 335, 70, 40, WHITE, CYAN, BLACK, "+", 3);
        home_btn.initButton(&tft, 120, 380, 65, 40, WHITE, CYAN, BLACK, "<-", 3);

        one_btn.drawButton(false);
        two_btn.drawButton(false);
        three_btn.drawButton(false);
        four_btn.drawButton(false);
        five_btn.drawButton(false);
        six_btn.drawButton(false);
        seven_btn.drawButton(false);
        eight_btn.drawButton(false);
        nine_btn.drawButton(false);
        asterisk_btn.drawButton(false);
        zero_btn.drawButton(false);
        hashtag_btn.drawButton(false);
        next_btn.drawButton(false);
        delete_btn.drawButton(false);
        plus_btn.drawButton(false);
        home_btn.drawButton(false);
    
        tft.drawRect(5, 50, 230, 45, CYAN);
        
        keyboard_screen_rendered = true;
    }

    bool down = Touch_getXY();
    one_btn.press(down && one_btn.contains(pixel_x, pixel_y));
    two_btn.press(down && two_btn.contains(pixel_x, pixel_y));
    three_btn.press(down && three_btn.contains(pixel_x, pixel_y));
    four_btn.press(down && four_btn.contains(pixel_x, pixel_y));
    five_btn.press(down && five_btn.contains(pixel_x, pixel_y));
    six_btn.press(down && six_btn.contains(pixel_x, pixel_y));
    seven_btn.press(down && seven_btn.contains(pixel_x, pixel_y));
    eight_btn.press(down && eight_btn.contains(pixel_x, pixel_y));
    nine_btn.press(down && nine_btn.contains(pixel_x, pixel_y));
    asterisk_btn.press(down && asterisk_btn.contains(pixel_x, pixel_y));
    zero_btn.press(down && zero_btn.contains(pixel_x, pixel_y));
    hashtag_btn.press(down && hashtag_btn.contains(pixel_x, pixel_y));
    next_btn.press(down && next_btn.contains(pixel_x, pixel_y));
    delete_btn.press(down && delete_btn.contains(pixel_x, pixel_y));
    plus_btn.press(down && plus_btn.contains(pixel_x, pixel_y));
    home_btn.press(down && home_btn.contains(pixel_x, pixel_y));

    if (one_btn.justReleased()) {
        one_btn.drawButton();
    }
  
    if (one_btn.justPressed()) {
        one_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("1");
            Phone_Number[phone_digits] = 49;
            phone_digits++;
            x += 18;
        }   
    }

    if (two_btn.justReleased()) {
        two_btn.drawButton();
    }
  
    if (two_btn.justPressed()) {
        two_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("2");
            Phone_Number[phone_digits] = 50;
            phone_digits++;
            x += 18;
        }   
    }

    if (three_btn.justReleased()) {
        three_btn.drawButton();
    }
  
    if (three_btn.justPressed()) {
        three_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("3");
            Phone_Number[phone_digits] = 51;
            phone_digits++;
            x += 18;
        }    
    }

    if (four_btn.justReleased()) {
        four_btn.drawButton();
    }
  
    if (four_btn.justPressed()) {
        four_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("4");
            Phone_Number[phone_digits] = 52;
            phone_digits++;
            x += 18;
        }   
    }

    if (five_btn.justReleased()) {
        five_btn.drawButton();
    }
  
    if (five_btn.justPressed()) {
        five_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("5");
            Phone_Number[phone_digits] = 53;
            phone_digits++;
            x += 18;
        }  
    }

    if (six_btn.justReleased()) {
        six_btn.drawButton();
    }
  
    if (six_btn.justPressed()) {
        six_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("6");
            Phone_Number[phone_digits] = 54;
            phone_digits++;
            x += 18;
        }   
    }

    if (seven_btn.justReleased()) {
        seven_btn.drawButton();
    }
  
    if (seven_btn.justPressed()) {
        seven_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("7");
            Phone_Number[phone_digits] = 55;
            phone_digits++;
            x += 18;
        }   
    }

    if (eight_btn.justReleased()) {
        eight_btn.drawButton();
    }
  
    if (eight_btn.justPressed()) {
        eight_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("8");
            Phone_Number[phone_digits] = 56;
            phone_digits++;
            x += 18;
        }   
    }

    if (nine_btn.justReleased()) {
        nine_btn.drawButton();
    }
  
    if (nine_btn.justPressed()) {
        nine_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("9");
            Phone_Number[phone_digits] = 57;
            phone_digits++;
            x += 18;
        }     
    } 

    if (asterisk_btn.justReleased()) {
        asterisk_btn.drawButton();
    }
  
    if (asterisk_btn.justPressed()) {
        asterisk_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("*");
            Phone_Number[phone_digits] = 42;
            phone_digits++;
            x += 18;
        }        
    }

    if (zero_btn.justReleased()) {
        zero_btn.drawButton();
    }
  
    if (zero_btn.justPressed()) {
        zero_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("0");
            Phone_Number[phone_digits] = 48;
            phone_digits++;
            x += 18;
        }   
    }

    if (hashtag_btn.justReleased()) {
        hashtag_btn.drawButton();
    }
  
    if (hashtag_btn.justPressed()) {
        hashtag_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("#");
            Phone_Number[phone_digits] = 35;
            phone_digits++;
            x += 18;
        }      
    } 

    if (delete_btn.justReleased()) {
        delete_btn.drawButton();
    } 

    if (delete_btn.justPressed()) {
        delete_btn.drawButton(true);
        phone_digits--;
        Phone_Number[phone_digits] = 32;
        if (x >= 18 && x <= 228) {
            x -= 18;
            tft.fillRect(x, y, 18, 22, BLACK);
        }    
    }
 
    if (plus_btn.justReleased()) {
        plus_btn.drawButton();
    }
  
    if (plus_btn.justPressed()) {
        plus_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (x < 228) {
            Cursor_Position(x);
            tft.setTextSize(3);
            tft.print("+");
            Phone_Number[phone_digits] = 43;
            phone_digits++;
            x += 18;
        }
    }

    if (home_btn.justReleased()) {
        home_btn.drawButton();
    }
  
    if (home_btn.justPressed()) {
        home_btn.drawButton(true);
        memset(Phone_Number, 0, phone_digits);
        screen_number = 1;
        home_screen_rendered = false;
        keyboard_screen_rendered = false;
    }

    if (next_btn.justReleased()) {
        next_btn.drawButton();
    }
  
    if (next_btn.justPressed()) {
        next_btn.drawButton(true);
        screen_number = 6;
        keyboard_screen_rendered = false;
    }
} 

// Function to process the message screen
void message_screen_processor() {  
    if (!message_screen_rendered) {
        tft.setRotation(0);  //PORTRAIT
        tft.fillScreen(BLACK);
        
        tft.setTextColor(WHITE);
        tft.setTextSize(2); 
        tft.setCursor(60,45);
        tft.println(Phone_Number);
        
        x = 12;
        y_message = 75;
        tft.setCursor(x,y_message);
        
        message_words[180] = {};
        message_char = 0;

        a_btn.initButton(&tft, 15, 190, 30, 40, WHITE, CYAN, BLACK, "a", 3);
        b_btn.initButton(&tft, 50, 190, 30, 40, WHITE, CYAN, BLACK, "b", 3);
        c_btn.initButton(&tft, 85, 190, 30, 40, WHITE, CYAN, BLACK, "c", 3);
        d_btn.initButton(&tft, 120, 190, 30, 40, WHITE, CYAN, BLACK, "d", 3);
        e_btn.initButton(&tft, 155, 190, 30, 40, WHITE, CYAN, BLACK, "e", 3);
        f_btn.initButton(&tft, 190, 190, 30, 40, WHITE, CYAN, BLACK, "f", 3);
        g_btn.initButton(&tft, 225, 190, 30, 40, WHITE, CYAN, BLACK, "g", 3);
        h_btn.initButton(&tft, 15, 235, 30, 40, WHITE, CYAN, BLACK, "h", 3);
        i_btn.initButton(&tft, 50, 235, 30, 40, WHITE, CYAN, BLACK, "i", 3);
        j_btn.initButton(&tft, 85, 235, 30, 40, WHITE, CYAN, BLACK, "j", 3);
        k_btn.initButton(&tft, 120, 235, 30, 40, WHITE, CYAN, BLACK, "k", 3);
        l_btn.initButton(&tft, 155, 235, 30, 40, WHITE, CYAN, BLACK, "l", 3);
        m_btn.initButton(&tft, 190, 235, 30, 40, WHITE, CYAN, BLACK, "m", 3);
        n_btn.initButton(&tft, 225, 235, 30, 40, WHITE, CYAN, BLACK, "n", 3);
        o_btn.initButton(&tft, 15, 280, 30, 40, WHITE, CYAN, BLACK, "o", 3);
        p_btn.initButton(&tft, 50, 280, 30, 40, WHITE, CYAN, BLACK, "p", 3);
        q_btn.initButton(&tft, 85, 280, 30, 40, WHITE, CYAN, BLACK, "q", 3);
        r_btn.initButton(&tft, 120, 280, 30, 40, WHITE, CYAN, BLACK, "r", 3);
        s_btn.initButton(&tft, 155, 280, 30, 40, WHITE, CYAN, BLACK, "s", 3);
        t_btn.initButton(&tft, 190, 280, 30, 40, WHITE, CYAN, BLACK, "t", 3);
        u_btn.initButton(&tft, 225, 280, 30, 40, WHITE, CYAN, BLACK, "u", 3);
        v_btn.initButton(&tft, 50, 325, 30, 40, WHITE, CYAN, BLACK, "v", 3);
        w_btn.initButton(&tft, 85, 325, 30, 40, WHITE, CYAN, BLACK, "w", 3);
        x_btn.initButton(&tft, 120, 325, 30, 40, WHITE, CYAN, BLACK, "x", 3);
        y_btn.initButton(&tft, 155, 325, 30, 40, WHITE, CYAN, BLACK, "y", 3);
        z_btn.initButton(&tft, 188, 325, 30, 40, WHITE, CYAN, BLACK, "z", 3);
        delete_btn.initButton(&tft, 222, 325, 35, 40, WHITE, CYAN, BLACK, "del", 2);
        space_btn.initButton(&tft, 98, 370, 110, 40, WHITE, CYAN, BLACK, "Space", 2);
        capital_btn.initButton(&tft, 15, 325, 30, 40, WHITE, CYAN, BLACK, "^", 3);
        other_char_btn.initButton(&tft, 20, 370, 40, 40, WHITE, CYAN, BLACK, "123", 2);
        return_btn.initButton(&tft, 198, 370, 85, 40, WHITE, CYAN, BLACK, "Return", 2);
        home_btn.initButton(&tft, 40, 20, 75, 30, RED, RED, BLACK, "<-", 2);
        send_msg_btn.initButton(&tft, 190, 20, 85, 30, GREEN, GREEN, BLACK, "Send", 2);

        a_btn.drawButton(false);
        b_btn.drawButton(false);
        c_btn.drawButton(false);
        d_btn.drawButton(false);
        e_btn.drawButton(false);
        f_btn.drawButton(false);
        g_btn.drawButton(false);
        h_btn.drawButton(false);
        i_btn.drawButton(false);
        j_btn.drawButton(false);
        k_btn.drawButton(false);
        l_btn.drawButton(false);
        m_btn.drawButton(false);
        n_btn.drawButton(false);
        o_btn.drawButton(false);
        p_btn.drawButton(false);
        q_btn.drawButton(false);
        r_btn.drawButton(false);
        s_btn.drawButton(false);
        t_btn.drawButton(false);
        u_btn.drawButton(false);
        v_btn.drawButton(false);
        w_btn.drawButton(false);
        x_btn.drawButton(false);
        y_btn.drawButton(false);
        z_btn.drawButton(false);
        delete_btn.drawButton(false);
        space_btn.drawButton(false);
        capital_btn.drawButton(false);
        other_char_btn.drawButton(false);
        return_btn.drawButton(false);
        home_btn.drawButton(false);
        send_msg_btn.drawButton(false);

        tft.drawRect(5, 70, 230, 90, CYAN);
        
        message_screen_rendered = true;
    }

    bool down = Touch_getXY();
    a_btn.press(down && a_btn.contains(pixel_x, pixel_y));
    b_btn.press(down && b_btn.contains(pixel_x, pixel_y));
    c_btn.press(down && c_btn.contains(pixel_x, pixel_y));
    d_btn.press(down && d_btn.contains(pixel_x, pixel_y));
    e_btn.press(down && e_btn.contains(pixel_x, pixel_y));
    f_btn.press(down && f_btn.contains(pixel_x, pixel_y));
    g_btn.press(down && g_btn.contains(pixel_x, pixel_y));
    h_btn.press(down && h_btn.contains(pixel_x, pixel_y));
    i_btn.press(down && i_btn.contains(pixel_x, pixel_y));
    j_btn.press(down && j_btn.contains(pixel_x, pixel_y));
    k_btn.press(down && k_btn.contains(pixel_x, pixel_y));
    l_btn.press(down && l_btn.contains(pixel_x, pixel_y));
    m_btn.press(down && m_btn.contains(pixel_x, pixel_y));
    n_btn.press(down && n_btn.contains(pixel_x, pixel_y));
    o_btn.press(down && o_btn.contains(pixel_x, pixel_y));
    p_btn.press(down && p_btn.contains(pixel_x, pixel_y));
    q_btn.press(down && q_btn.contains(pixel_x, pixel_y));
    r_btn.press(down && r_btn.contains(pixel_x, pixel_y));
    s_btn.press(down && s_btn.contains(pixel_x, pixel_y));
    t_btn.press(down && t_btn.contains(pixel_x, pixel_y));
    u_btn.press(down && u_btn.contains(pixel_x, pixel_y));
    v_btn.press(down && v_btn.contains(pixel_x, pixel_y));
    w_btn.press(down && w_btn.contains(pixel_x, pixel_y));
    x_btn.press(down && x_btn.contains(pixel_x, pixel_y));
    y_btn.press(down && y_btn.contains(pixel_x, pixel_y));
    z_btn.press(down && z_btn.contains(pixel_x, pixel_y));
    delete_btn.press(down && delete_btn.contains(pixel_x, pixel_y));
    space_btn.press(down && space_btn.contains(pixel_x, pixel_y));
    capital_btn.press(down && capital_btn.contains(pixel_x, pixel_y));
    other_char_btn.press(down && other_char_btn.contains(pixel_x, pixel_y));
    return_btn.press(down && return_btn.contains(pixel_x, pixel_y));
    home_btn.press(down && home_btn.contains(pixel_x, pixel_y));
    send_msg_btn.press(down && send_msg_btn.contains(pixel_x, pixel_y));

    if (home_btn.justReleased()) {
        home_btn.drawButton();
    }
  
    if (home_btn.justPressed()) {
        home_btn.drawButton(true);
        memset(Phone_Number, 0, phone_digits);
        screen_number = 5;
        message_screen_rendered = false;
    }

    if (a_btn.justReleased()) {
        a_btn.drawButton();
    }
  
    if (a_btn.justPressed()) {
        a_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("a");
            message_words[message_char] = 97;
            message_char++;
            x += 12;
        } 
    }

    if (b_btn.justReleased()) {
        b_btn.drawButton();
    }
  
    if (b_btn.justPressed()) {
        b_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("b");
            message_words[message_char] = 98;
            message_char++;
            x += 12;
        }   
    }

    if (c_btn.justReleased()) {
        c_btn.drawButton();
    }
  
    if (c_btn.justPressed()) {
        c_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("c");
            message_words[message_char] = 99;
            message_char++;
            x += 12;
        }   
    }

    if (d_btn.justReleased()) {
        d_btn.drawButton();
    }

    if (d_btn.justPressed()) {
        d_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("d");
            message_words[message_char] = 100;
            message_char++;
            x += 12;
        }   
    }

    if (e_btn.justReleased()) {
        e_btn.drawButton();
    }

    if (e_btn.justPressed()) {
        e_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("e");
            message_words[message_char] = 101;
            message_char++;
            x += 12;
        }   
    }

    if (f_btn.justReleased()) {
        f_btn.drawButton();
    }

    if (f_btn.justPressed()) {
        f_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("f");
            message_words[message_char] = 102;
            message_char++;
            x += 12;
        }   
    }

    if (g_btn.justReleased()) {
        g_btn.drawButton();
    }

    if (g_btn.justPressed()) {
        g_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("g");
            message_words[message_char] = 103;
            message_char++;
            x += 12;
        }   
    }

    if (h_btn.justReleased()) {
        h_btn.drawButton();
    }

    if (h_btn.justPressed()) {
        h_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("h");
            message_words[message_char] = 104;
            message_char++;
            x += 12;
        }   
    }

    if (i_btn.justReleased()) {
        i_btn.drawButton();
    }

    if (i_btn.justPressed()) {
        i_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("i");
            message_words[message_char] = 105;
            message_char++;
            x += 12;
        }   
    }

    if (j_btn.justReleased()) {
        j_btn.drawButton();
    }

    if (j_btn.justPressed()) {
        j_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("j");
            message_words[message_char] = 106;
            message_char++;
            x += 12;
        }   
    }

    if (k_btn.justReleased()) {
        k_btn.drawButton();
    }

    if (k_btn.justPressed()) {
        k_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("k");
            message_words[message_char] = 107;
            message_char++;
            x += 12;
        }   
    }

    if (l_btn.justReleased()) {
        l_btn.drawButton();
    }

    if (l_btn.justPressed()) {
        l_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("l");
            message_words[message_char] = 108;
            message_char++;
            x += 12;
        }   
    }

    if (m_btn.justReleased()) {
        m_btn.drawButton();
    }

    if (m_btn.justPressed()) {
        m_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("m");
            message_words[message_char] = 109;
            message_char++;
            x += 12;
        }   
    }

    if (n_btn.justReleased()) {
        n_btn.drawButton();
    }

    if (n_btn.justPressed()) {
        n_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("n");
            message_words[message_char] = 110;
            message_char++;
            x += 12;
        }   
    }

    if (o_btn.justReleased()) {
        o_btn.drawButton();
    }

    if (o_btn.justPressed()) {
        o_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("o");
            message_words[message_char] = 111;
            message_char++;
            x += 12;
        }   
    }

    if (p_btn.justReleased()) {
        p_btn.drawButton();
    }

    if (p_btn.justPressed()) {
        p_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("p");
            message_words[message_char] = 112;
            message_char++;
            x += 12;
        }   
    }

    if (q_btn.justReleased()) {
        q_btn.drawButton();
    }

    if (q_btn.justPressed()) {
        q_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("q");
            message_words[message_char] = 113;
            message_char++;
            x += 12;
        }   
    }

    if (r_btn.justReleased()) {
        r_btn.drawButton();
    }

    if (r_btn.justPressed()) {
        r_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("r");
            message_words[message_char] = 114;
            message_char++;
            x += 12;
        }   
    }

    if (s_btn.justReleased()) {
        s_btn.drawButton();
    }

    if (s_btn.justPressed()) {
        s_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("s");
            message_words[message_char] = 115;
            message_char++;
            x += 12;
        }   
    }

    if (t_btn.justReleased()) {
        t_btn.drawButton();
    }

    if (t_btn.justPressed()) {
        t_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("t");
            message_words[message_char] = 116;
            message_char++;
            x += 12;
        }   
    }

    if (u_btn.justReleased()) {
        u_btn.drawButton();
    }

    if (u_btn.justPressed()) {
        u_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("u");
            message_words[message_char] = 117;
            message_char++;
            x += 12;
        }   
    }

    if (v_btn.justReleased()) {
        v_btn.drawButton();
    }

    if (v_btn.justPressed()) {
        v_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("v");
            message_words[message_char] = 118;
            message_char++;
            x += 12;
        }   
    }

    if (w_btn.justReleased()) {
        w_btn.drawButton();
    }

    if (w_btn.justPressed()) {
        w_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("w");
            message_words[message_char] = 119;
            message_char++;
            x += 12;
        }   
    }

    if (x_btn.justReleased()) {
        x_btn.drawButton();
    }

    if (x_btn.justPressed()) {
        x_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("x");
            message_words[message_char] = 120;
            message_char++;
            x += 12;
        }   
    }

    if (y_btn.justReleased()) {
        y_btn.drawButton();
    }

    if (y_btn.justPressed()) {
        y_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("y");
            message_words[message_char] = 121;
            message_char++;
            x += 12;
        }   
    }

    if (z_btn.justReleased()) {
        z_btn.drawButton();
    }
    
    if (z_btn.justPressed()) {
        z_btn.drawButton(true);
        tft.setTextColor(WHITE);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            tft.setTextSize(2);
            tft.print("z");
            message_words[message_char] = 122;
            message_char++;
            x += 12;
        }   
    }

    if (delete_btn.justReleased()) {
        delete_btn.drawButton();
    }
  
    if (delete_btn.justPressed()) {
        delete_btn.drawButton(true);
        message_char--;
        message_words[message_char] = 32;
        if (x >= 12 && x <= 228) {
            if (x == 12 && y_message > 75) {
                y_message -= 15;
                x = 216;
                tft.fillRect(x, y_message, 12, 22, BLACK);
            } else {
                x -= 12;
                tft.fillRect(x, y_message, 12, 22, BLACK);    
            }     
        }    
    }

    if (space_btn.justReleased()) {
        space_btn.drawButton();
    }

    if (space_btn.justPressed()) {
        space_btn.drawButton(true);
        if (y_message <= 135) {
            int new_x = Message_Cursor_Position(x);
            if (x > 216) {
                x = new_x;
            }
            message_words[message_char] = 32;
            message_char++;
            x += 12;
        }    
    }

    if (send_msg_btn.justReleased()) {
        send_msg_btn.drawButton();
    }
  
    if (send_msg_btn.justPressed()) {
        send_msg_btn.drawButton(true);

        Serial.print("Number to message ->(");
        Serial.print(Phone_Number);
        Serial.println(")");
        // AT command to set sms message format to text mode
        Serial2.write("AT+CMGF=1\r");
        delay(1000);
        // AT command to send sms message
        Serial2.write("AT+CMGS=\"");
        Serial2.write((char*) Phone_Number);
        Serial2.write("\"\r");
        delay(100);
        Serial2.write((char*) message_words);
        delay(100);
        Serial2.write(26);
   
        Serial.println(_readSerial());
        Serial.println("SMS sent");
        delay(100);
        screen_number = 7;
        message_screen_rendered = false;          
    }
    //delay(100);
} 

// Function to process the message sent screen
void sent_sms_screen_processor() {
    if (!sent_sms_screen_rendered) {
        tft.setRotation(0);  //PORTRAIT
        tft.fillScreen(BLACK);

        tft.fillRoundRect(8,45,225,140,15, CYAN);
        
        tft.setTextColor(BLACK);
        tft.setTextSize(2); 
        tft.setCursor(50,95);
        tft.println("SMS Sent To");
        tft.setCursor(45,115);
        tft.println(Phone_Number);

        next_btn.initButton(&tft, 120, 285, 85, 40, GREEN, GREEN, BLACK, "OK", 3);
        next_btn.drawButton(false);

        sent_sms_screen_rendered = true;
    }   

    bool down = Touch_getXY();
    next_btn.press(down && next_btn.contains(pixel_x, pixel_y));

    if (next_btn.justReleased()) {
        next_btn.drawButton();
    }
  
    if (next_btn.justPressed()) {
        next_btn.drawButton(true);
        memset(message_words, 0, message_char);
        screen_number = 6;
        sent_sms_screen_rendered = false;
    }
}

// Function to process the sms notification screen
void sms_notification_processor() {
    if (!sms_notification_rendered) {
        tft.setRotation(0);  //PORTRAIT

        tft.fillRoundRect(8,10,225,95,15, YELLOW);
        return_btn.initButton(&tft, 198, 86, 60, 35, WHITE, CYAN, BLACK, "OK", 2);
        next_btn.initButton(&tft, 135, 86, 60, 35, WHITE, CYAN, BLACK, "VIEW", 2);

        return_btn.drawButton(false);
        next_btn.drawButton(false);

        tft.setTextColor(BLACK);
        tft.setTextSize(2); 
        tft.setCursor(12,20);
        Serial.println("!!!!!!!");
        Serial.println(Sender_Phone_Number);
        tft.println(Sender_Phone_Number);

        tft.setCursor(12,45); 
        if (strlen(Sender_Message_Words) > 16) {
            strncpy(Short_Sender_Message, Sender_Message_Words, 16);
            tft.print(Short_Sender_Message);
            tft.print("...");
        }else {
            tft.print(Sender_Message_Words);
        }

        sms_notification_rendered = true;
    }

    bool down = Touch_getXY();
    return_btn.press(down && return_btn.contains(pixel_x, pixel_y));
    next_btn.press(down && next_btn.contains(pixel_x, pixel_y));

    if (return_btn.justReleased()) {
        return_btn.drawButton();
    }
  
    if (return_btn.justPressed()) {
        return_btn.drawButton(true);
        memset(Sender_Phone_Number, 0, sizeof(Sender_Phone_Number));
        Serial.println("Deleting message");
        // AT command to delete the message
        Serial2.write("AT+CMGD=1\r\n");
        Serial.println(_readSerial());
        delay(100);
        screen_number = prev_screen_number;
        Serial.println(screen_number);
        change_screen_number(screen_number);
        sms_notification_rendered = false;
    }  

    if (next_btn.justReleased()) {
        next_btn.drawButton();
    }
  
    if (next_btn.justPressed()) {
        next_btn.drawButton(true);
        screen_number = 9;
        message_viewer_rendered = false;
        sms_notification_rendered = false;
        
    }  
}

// Function to process the message viewer screen
void message_viewer_processor() {
    if (!message_viewer_rendered) {
        tft.setRotation(0);  //PORTRAIT
        tft.fillScreen(BLACK);

        tft.setTextColor(WHITE);
        tft.setTextSize(2); 
        tft.setCursor(12,45);
        tft.println(Sender_Phone_Number);
 
        tft.fillRoundRect(8,70,230,95,15, GREEN);
        tft.setTextColor(BLACK);
        tft.setCursor(12,80); 
        tft.print(Sender_Message_Words);    

        next_btn.initButton(&tft, 120, 285, 85, 40, GREEN, GREEN, BLACK, "OK", 3);
        next_btn.drawButton(false);

        message_viewer_rendered = true;  
    }  

    bool down = Touch_getXY();
    next_btn.press(down && next_btn.contains(pixel_x, pixel_y));

    if (next_btn.justReleased()) {
        next_btn.drawButton();
    }
  
    if (next_btn.justPressed()) {
        next_btn.drawButton(true);
        memset(Sender_Phone_Number, 0, sizeof(Sender_Phone_Number));
        memset(Sender_Message_Words, 0, sizeof(Sender_Message_Words));
        Serial.println("Deleting message");
        // AT command to delete the message
        Serial2.write("AT+CMGD=1\r\n");
        Serial.println(_readSerial());
        delay(100);
        screen_number = prev_screen_number;
        change_screen_number(screen_number);
        message_viewer_rendered = false;
    }
}
       
#endif