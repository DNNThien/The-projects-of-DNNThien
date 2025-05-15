#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#define rows 4
#define columns 4

// 2D array representing the layout of the 4x4 keypad.
// Each element corresponds to a key at the intersection of a row and column.
// Example: keys[0][0] is '1', keys[1][2] is '6', etc.
char keys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},
};

// Array of GPIO pins connected to the rows of the keypad.
// The order should match the rows in the 'keys' array.
byte rowPins[rows] = {13, 12, 14, 27};

// Array of GPIO pins connected to the columns of the keypad.
// The order should match the columns in the 'keys' array.
byte columnPins[columns] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, columnPins, rows, columns);
LiquidCrystal_I2C lcd(0x27, 20, 4);

int startPin = 35;

void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(startPin, INPUT);
  delay(500);
  displayToLcd(1, 7, 1, "HELLO");
}

void displayToLcd(int clear, int col, int row, String str)
{
  if(clear) lcd.clear();
  lcd.setCursor(col, row);
  lcd.print(str);
}

char readKey()
{
  char key = keypad.getKey();
  return (key) ? key : '\0';
}

void readStr(int LENGTH)
{
  displayToLcd(1, 5, 1, "ENTER DATA");
  String str = "";
  String remain = "[";
  for(int i=0;i<LENGTH;i++)
    remain += '_';
  remain += ']';
  int colStart = (20 - LENGTH) / 2;
  displayToLcd(0, colStart - 1, 2, remain);
  int len = LENGTH;
  while(len)
  {
    char key = readKey();
    if(key == 'A' || key == 'C')
      break;
    if(key == 'B')
    {
      str = "";
      len = LENGTH;
      displayToLcd(1, 3, 1, "Re-ENTER DATA");
      displayToLcd(0, colStart - 1, 2, remain);
    }
    if(key >= '0' && key <= '9')
    {
      str += key;
      len--;
      displayToLcd(0, colStart, 2, str);
    }
    delay(100);
  }
  delay(1000);
  Serial.printf("Your string is '%s'\nLength of your string is %d\n", str, str.length());
  displayToLcd(1, 4, 1, "YOUR  STRING");
  displayToLcd(0, (20 - str.length()) / 2, 2, str);
  delay(5000);
}

void loop()
{
  if(digitalRead(startPin) == LOW)
  {
    while(digitalRead(startPin) == LOW);
    int LENGTH = 10;
    readStr(LENGTH);
    displayToLcd(1, 7, 1, "HELLO");
  }
  delay(500);
}