**Smart Navigation Cart for Visually Impaired Using ESP32

A smart assistive robotic cart designed to help visually impaired users navigate supermarkets independently.
The system uses voice commands, RFID-based navigation, obstacle detection, and audio guidance to guide users to specific product sections.

**Project Overview

The Smart Navigation Cart allows visually impaired users to select a product category using voice commands via Bluetooth.
The cart then autonomously navigates through a predefined path using line following and RFID tags placed at junctions and destinations.

Audio feedback is provided through a DFPlayer Mini module to inform the user about navigation actions such as starting movement, turning directions, obstacle detection, and arrival at the destination.

**Key Features

Voice command input using Bluetooth
Autonomous navigation using ESP32
RFID-based junction and destination detection
Line-following navigation system
Obstacle detection using ultrasonic sensor
Audio guidance using DFPlayer Mini
Inventory validation for product categories
Real-time user feedback through audio alerts

**Supported Product Categories

The system currently supports the following inventory destinations:
Books
Cleanser
Vegetables
Checkout
Groceries
Toys
Beverages
Snacks
Dairy
Electronics
If a user requests an item outside this list, the system provides an invalid item audio alert.

**Hardware Components

ESP32 Development Board
L298N Motor Driver
RFID Reader (MFRC522)
Ultrasonic Sensor (HC-SR04)
IR Line Following Sensors
DFPlayer Mini MP3 Module
Speaker
Bluetooth Communication
DC Motors with Wheels
Robot Chassis
Power Supply

**System Architecture

The navigation system works using the following sequence:
User gives a voice command via Bluetooth.
The command is converted to a 3-letter code.
The system validates the request against the inventory list.
If valid, the robot starts navigation.
The robot follows a line path using IR sensors.
RFID tags determine junction directions and destinations.
The ultrasonic sensor detects obstacles.
The DFPlayer module provides audio feedback.
The robot stops when the destination tag is detected.

**Pin Configuration
>>Motor Driver (L298N)
Pin	ESP32
ENA	GPIO 14
IN1	GPIO 12
IN2	GPIO 13
ENB	GPIO 32
IN3	GPIO 33
IN4	GPIO 25

>>IR Array Sensors
Sensor	ESP32
Left	GPIO 34
Center	GPIO 35
Right	GPIO 26

>>Ultrasonic Sensor
Pin	ESP32
Trigger	GPIO 27
Echo	GPIO 22

>>RFID Reader
Pin	ESP32
SDA	GPIO 5
SCK	GPIO 18
MOSI	GPIO 23
MISO	GPIO 19
RST	GPIO 2

>>DFPlayer Mini
Pin	ESP32
RX	GPIO 4
TX	GPIO 21 
