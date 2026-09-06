#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// --- Configuration ---
// I2C LCD Address - check your specific LCD for the correct address (0x27 or 0x3F are common)
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Pin Definitions
#define EXTERNAL_LED_PIN 13 // Built-in LED or an external indicator LED (HIGH = Active)
#define CUTOFF_PIN 8        // Relay or Transistor control pin (HIGH = System ON, LOW = System OFF)

// Default session duration (in minutes). Must be between 1 and 60.
#define SESSION_MIN 1

// --- Global Variables ---
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

bool isSessionActive = false;
unsigned long sessionDuration; // Duration in milliseconds
unsigned long sessionStart = 0; // Time when the session started
bool wasSessionActive = false; // Flag to force an LCD update when transitioning out of session

// --- Function Prototypes ---
void lcdPrintCenteredLine(int row, const char* text);
void startSession();
void endSession();
void updateDisplay(unsigned long elapsed);
void handleSerialCommands();

// --- Utility Function: Centered LCD Printing ---
void lcdPrintCenteredLine(int row, const char* text) {
  lcd.setCursor(0, row);
  for (int i = 0; i < LCD_COLS; i++) {
    lcd.print(' '); // Clear the line
  }
  lcd.setCursor((LCD_COLS - strlen(text)) / 2, row);
  lcd.print(text);
}

// --- Logic Function: Starting the Timer ---
void startSession() {
  if (!isSessionActive) {
    sessionStart = millis();
    isSessionActive = true;
    // Activate the main system (e.g., turn on a fan or a desk lamp)
    digitalWrite(CUTOFF_PIN, HIGH); 
    lcd.clear();
    lcdPrintCenteredLine(0, "SESSION STARTED");
    
    // Show duration on second line
    long durationMin = sessionDuration / (60 * 1000);
    char durationMsg[17];
    sprintf(durationMsg, "Duration: %d min", durationMin);
    lcdPrintCenteredLine(1, durationMsg);
    
    delay(2000); // Show the message for 2 seconds
    
    // Clear and show timer display
    lcd.clear();
    lcdPrintCenteredLine(0, "TIME REMAINING:");
    
    Serial.println("SESSION_STARTED");
  }
}

// --- Logic Function: Ending the Timer ---
void endSession() {
  if (isSessionActive) {
    isSessionActive = false;
    // Deactivate the main system
    digitalWrite(CUTOFF_PIN, LOW);
    wasSessionActive = true; // Mark for idle screen update
    Serial.println("Session Ended by user command.");
  }
}

// --- Display Function: Update LCD with Timer ---
void updateDisplay(unsigned long elapsed) {
  unsigned long remainingTime = sessionDuration - elapsed;
  
  // Calculate Minutes and Seconds
  unsigned long totalSeconds = remainingTime / 1000;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  
  // Format the time string
  char timeString[6]; // MM:SS\0
  sprintf(timeString, "%02d:%02d", minutes, seconds);

  // Update display every second or when the display needs to change significantly
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000 || wasSessionActive) {
    // Update the timer display
    lcd.setCursor(0, 1); // Move to the second line
    lcd.print("                "); // Clear the line
    lcd.setCursor(0, 1); // Move to the second line again
    lcd.print(timeString); // Display the remaining time
    
    lastUpdate = millis();
    wasSessionActive = false; // Reset flag after update
  }
}

// --- Logic Function: Serial Command Handler ---
void handleSerialCommands() {
  while (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("SET_DURATION:")) {
      String durationStr = command.substring(13); // Length of "SET_DURATION:"
      int newDuration = durationStr.toInt();
      
      if (newDuration >= 1 && newDuration <= 60) { // Changed minimum to 1 minute
        sessionDuration = (unsigned long)newDuration * 60 * 1000; // Convert minutes to ms
        Serial.print("Duration set to: ");
        Serial.print(newDuration);
        Serial.println(" minutes.");
        lcd.clear();
        lcdPrintCenteredLine(0, "DURATION SET:");
        
        char durationStr[16];
        sprintf(durationStr, "%d minutes", newDuration);
        lcdPrintCenteredLine(1, durationStr);
        
        wasSessionActive = true; // Force update to idle screen after setting
        delay(2000);
      } else {
        Serial.println("Error: Duration must be between 1 and 60 minutes.");
      }
    } else if (command.startsWith("START:")) {
      // Handle START command with duration
      String durationStr = command.substring(6); // Length of "START:"
      int newDuration = durationStr.toInt();
      
      if (newDuration >= 1 && newDuration <= 60) { // Changed minimum to 1 minute
        sessionDuration = (unsigned long)newDuration * 60 * 1000; // Convert minutes to ms
        Serial.print("Starting session: ");
        Serial.print(newDuration);
        Serial.println(" minutes.");
        startSession();
      } else {
        Serial.println("Error: Duration must be between 1 and 60 minutes.");
      }
    } else if (command.equalsIgnoreCase("S")) {
      startSession();
    } else if (command.equalsIgnoreCase("E")) {
      endSession();
    } else if (command.startsWith("SET_CURRENT:")) {
      // Handle current setting if needed
      String currentStr = command.substring(12);
      Serial.print("Current set to: ");
      Serial.println(currentStr);
    }
  }
}

// ------------------------------------------------------------------
// --- SETUP ---
// ------------------------------------------------------------------
void setup() {
  // Initialize Serial communication
  Serial.begin(9600);

  // Configure Pins
  pinMode(EXTERNAL_LED_PIN, OUTPUT);
  pinMode(CUTOFF_PIN, OUTPUT);

  // Initial state of output pins
  digitalWrite(EXTERNAL_LED_PIN, LOW); // LED OFF when idle
  digitalWrite(CUTOFF_PIN, LOW);       // System OFF when idle

  // Set default duration (converted to milliseconds)
  sessionDuration = (unsigned long)SESSION_MIN * 60 * 1000; 
  
  // Initialize the LCD
  lcd.init();
  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.backlight();
  
  // Set initial state to force the first updateDisplay call in loop() to draw the idle screen.
  wasSessionActive = true; // Will trigger the initial idle screen draw

  Serial.println("--- Arduino tDCS Timer Ready ---");
  Serial.print("Default Duration: ");
  Serial.print(SESSION_MIN);
  Serial.println(" minutes.");
  Serial.println("Commands: 'START:X' (Start X minutes), 'E' (End), 'SET_DURATION:X' (1-60 mins).");
}

// ------------------------------------------------------------------
// --- LOOP ---
// ------------------------------------------------------------------
void loop() {
  handleSerialCommands();

  if (isSessionActive) {
    unsigned long elapsed = millis() - sessionStart;
    
    if (elapsed >= sessionDuration) {
      // Session completion logic
      isSessionActive = false;
      digitalWrite(CUTOFF_PIN, LOW); // System OFF
      digitalWrite(EXTERNAL_LED_PIN, LOW); // External LED OFF
      
      Serial.println("SESSION_TIME_COMPLETE");
      Serial.println("SESSION HAS ENDED");

      // Display session end message
      lcd.clear();
      lcdPrintCenteredLine(0, "SESSION COMPLETE");
      lcdPrintCenteredLine(1, "SUCESSFULLY"); 
      wasSessionActive = true; // Mark for idle screen update
      
      // Keep the completion message displayed for 5 seconds
      delay(5000);
    } else {
      // Session running logic
      digitalWrite(EXTERNAL_LED_PIN, HIGH); // External LED ON while running
      updateDisplay(elapsed); // Continually update the timer on the LCD
    }
  } else {
    // Session is inactive (Idle state)
    digitalWrite(EXTERNAL_LED_PIN, LOW); // External LED OFF
    
    // Only update the display when transitioning to idle or initially
    if (wasSessionActive) { 
      lcd.clear();
      lcdPrintCenteredLine(0, "NEURAEASE READY");
      
      // Convert session duration back to minutes for display
      long durationMin = sessionDuration / (60 * 1000); 
      char durationStr[16];
      sprintf(durationStr, "Set: %d minutes", durationMin);
      lcdPrintCenteredLine(1, durationStr);
      
      wasSessionActive = false; // Screen is now updated
    }
    // Small delay to prevent running the loop too fast when idle
    delay(100); 
  }
}