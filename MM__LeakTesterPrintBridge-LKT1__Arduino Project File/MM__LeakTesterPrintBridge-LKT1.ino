/*
 * 
 * LeakTesterPrintBridge
 * This program is designed for the Arduino Mega to use it as a bridge between the leak
 * tester and a printer. It uses off-the-shelf components such as
 * I2C LCD1602 Arduino LCD Display Module (Blue) https://www.dfrobot.com/product-1724.html
 * Membrane 3x4 Matrix Keypad https://www.adafruit.com/product/419
 * Other helpful links used in the making of this program.
 * http://forum.arduino.cc/index.php?topic=396450
 * https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/
 * https://www.instructables.com/id/HOW-TO-use-the-ARDUINO-SERIAL-MONITOR/
 * https://www.arduino.cc/reference/en/language/functions/communication/serial/readstringuntil/
 * 
 * Deployed 12-03-2018
 * Modifications and Updates
 * 12-04-2018 Removed "SV" initials from output
 * 
 */
#include <Wire.h>
#include <Keypad.h>
#include "DFRobot_LCD.h"
#define CLOCK_LENGTH 6
#define dateField 5
#define passField 11
#define programField 2
const byte numChars = 128;
char receivedChars[numChars];   // an array to store the received data - does not need to get everything, just what's needed
boolean newData = false;
char clock[CLOCK_LENGTH+10];    // using all 16 characters helps keep the LCD screen free of "garbage".
bool alset = false;             // flag for when all 6 clock numbers have been entered.
int cpos = 0;  // This is for scaleability - a chance that clock number lengths might increase in the future
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {8, 7, 6, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 3, 2}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
DFRobot_LCD lcd(16,2);  //16 characters and 2 lines of show
String P01 = "8M0109654 ";
String P02 = "8M0109466 ";
String P03 = "8M0135340B";  // two semi-hardcoded numbers for now, but if this expands so must the handling.
String P04 = "8M0146999 ";  // NOTE: allocate 10 spaces and always buffer or the label might corrupt
String activePart = "";
//int partdes = 0;    //  Double use: which part selected, or 0, flag that no part selected.
void setup() {
    lcd.init();
    lcd.setCursor(0, 0);   
    lcd.print("Enter Employee #");
    //lcd.setCursor(0, 0);
    //lcd.print("SEL: 1-" + pn1);
    //lcd.setCursor(0, 1);
    //lcd.print("      -" + pn2);
    memset(clock, ' ', CLOCK_LENGTH*sizeof(char));  // zero out the clock variable
    Serial1.begin(115200);  // The mega can come in this fast, but don't always expect output to be this fast. 
    //Serial.begin(115200);  // The mega can come in this fast, but don't always expect output to be this fast. 
    //Serial.println("deb100");
}
void loop() {
    char key = keypad.getKey();
   // Serial.print(String(key).c_str());//deb779
  //  if ((partdes == 0) && ((key == '1') || (key == '2'))){
  //    if (key == '1'){
  //      partdes = 1;
  //      activePart = pn1;
  //    } // if
  //    if (key == '2'){
  //      partdes = 2;
  //      activePart = pn2;
  //    } // if
  //    lcd.clear();
  //    lcd.setCursor(0, 0);   
  //    lcd.print("Enter Clock #");
  //    return;
  //  }// else {return;}
    //if (!partdes) {   //deb223
    //  lcd.clear();
    //  lcd.setCursor(0, 0);
    //  lcd.print("SEL: 1-" + pn1);
    //  lcd.setCursor(0, 1);
    //  lcd.print("      -" + pn2);
      
    //  return;
    //}  else {
//    }// if
    if (key == '#') {     // Reset entry
         memset(clock, ' ', (CLOCK_LENGTH+10)*sizeof(char)); // clear out clock variable
         cpos = 0;
         //partdes = 0;
         activePart = "";
         alset = false;

         lcd.clear();
         //lcd.setCursor(0, 0);
         //lcd.print("SEL: 1-" + pn1);
         //lcd.setCursor(0, 1);
         //lcd.print("      -" + pn2);
         lcd.setCursor(0, 0);   
         lcd.print("Enter Employee #");
         return;    
    } else if (key == '*') {     // Enter Clock number
         if (cpos != CLOCK_LENGTH) return; // Not enough
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Current Emp #:");
         lcd.setCursor(0, 1);
         lcd.print(clock);
         lcd.setCursor(7,1);
         lcd.print(activePart.c_str());   
         alset = true;
         return;
    } // if
    if ((key && !alset) && (cpos < CLOCK_LENGTH)){
      clock[cpos] = key;  
      lcd.setCursor(0, 1);
      lcd.print(clock);
      cpos++; 
      return; // necessary to avoid overwrite from next block
    }  // if
    
    handleComms();    // See if anything is coming in from the Serial Port
    handleNewData(); 
}
void handleComms() {
  
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    int mark = 0;
    while (Serial1.available() > 0 && newData == false) {
        rc = Serial1.read();
        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            } // if
        } else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }  // if
        if (mark++ > 128) break; // prevent runaway loops if the LF/CR is lost
    }  // while
}
void handleNewData() {
    //
    // Data comes in rather rough. Put variable blank spaces into single comma, then parse out looking for certain fields.
    if (newData == true) {
        
        String d = ",";
        size_t pos = 0;
        size_t i;
        String s = String(receivedChars);   // Put received characters into new buffer.
        //Serial.println("deb110 new data handling routine called..." + s);
        memset (receivedChars,' ',numChars*sizeof(char));// null out the reception buffer  
        int l = s.length();
        bool flag1 = false;         // This flag variable is only used to clean up the data, not to confuse with "pass" flag.
        String curated;        // hold the new reconstructed string more ready for parsing
        String date = "";
        String program = "";
        bool passflag = false;      // Flag for when the "P" indicating a passed test is found in the incoming stream         
        for (int incr = 0; incr < l; incr++) { // Turn first space into "," then disard the rest until a new char is hit
          if ((s.charAt(incr) == ' ') && (flag1)) {
            s.setCharAt(incr, ',');
            curated += ",";// s.charAt(incr); // And for first space save it as a comma, it's needed in curated string for parsing tokens
            flag1 = false;      // but dump the rest of the spaces
            continue;       // The next part of code in this loop is not needed when this happens for this iteration
          }   // if
          if (s.charAt(incr) != ' ') flag1 = true;  // Make sure we always get non space characters and catch the next space
          if (flag1) {
            curated += s.charAt(incr);
          }  // if
        } // for
        s = curated;
        //Serial.println("deb110 new data handling routine called..." + s);
        int ctr = 0;
        String token;
        while ((pos = s.indexOf(d)) < s.length()) {
          token = s.substring(0, pos);
          token.trim();
          s.remove(0, pos + d.length());
          s.trim();
          if (ctr == programField) { // program number field is expected in this position
            program = token;
          }  // if
          if (ctr == dateField) { // Date is expected to be in this position. 
            date = token;          
          } // if
          // if there is a failure this code is never even reached
          if (ctr == passField) {  
            //Serial.println("deb117 ctr = passfield " + token);
            if (token.equals("P")) {  // Here is where the pass or fail field is, look for "P"
              passflag = true;
              //Serial.println("deb120 setting pass flag to true");
              break;  // there is no reason to continue in this loop.
            } // if
          }  // if
          ctr++;         
        }  // while
        if (passflag) { // there is now a date and clearance to create a label... 
          if (cpos < CLOCK_LENGTH){ // Is a clock number entered? Cannot print a label without it.
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("NEED EMPL NUMBER");
              lcd.setCursor(0, 1);
              lcd.print("----------------");
          } else {
            // Here active part number is set based on which program number the tester is using.
            if (program.equals("P01")){
              activePart = P01;
            } else if (program.equals("P02")) {
              activePart = P02;
            } else if (program.equals("P03")) {
              activePart = P03;
            } else if (program.equals("P04")) {
              activePart = P04;
            }   // if
            printLabel(date, String(clock), activePart);  
            passflag = false; // done printing, set "is it good" flag back to 0
          }  // if
        } // if
    } // if
    newData = false; // having processed the now "old" data, set this to false. 
}
void printLabel(String date, String clocknum, String mercuryPN) {
  //Serial.println("deb200 print routine called");
  char pbuff[128];
  sprintf(pbuff, "^XA^LH1,1^FO1,40^AEN,21,10^FD%s PASSED %s^FS^FO1,100^AEN,21,10^FD%s    ID-P7^FS^XZ", date.c_str(), clocknum.c_str(), mercuryPN.c_str());
  //sprintf(pbuff, "^XA\n^FO10,20^AEN,35,20^FDPASSED^FS\n^FO10,60^AEN,35,20^FDSV ID-P7^FS\n^XZ", date, clocknum, mercuryPN);
  String zpl(pbuff);
  Serial2.begin(9600);
  Serial2.print(zpl);  
}
