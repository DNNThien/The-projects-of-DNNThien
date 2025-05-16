# ESP32 38-Pin Breakout Board

This is a custom **ESP32 breakout board** designed for the 38-pin version of the ESP32 development board. It provides easy access to all GPIO pins and includes extra features for prototyping and development.

## 🔧 Features

- Supports **ESP32 DevKit with 38 pins**
- Full **pinout breakout** with clearly labeled headers
- Includes **2 tactile push buttons** (connected to user-defined GPIOs)
- Includes **2 I2C headers** (SDA, SCL, VCC, GND) for connecting I2C peripherals

## 📌 Pinout

All 38 GPIO pins are broken out to 2.54mm headers, making it easy to connect jumper wires or interface with breadboards.

## 🔘 Push Buttons

- 2 onboard push buttons
- Can be connected to any available GPIOs (e.g., GPIO0, GPIO2)
- Useful for reset, user input, or other custom functions

## 📡 I2C Headers

- Two 4-pin headers for I2C devices
- Each header includes:
  - **SDA**
  - **SCL**
  - **VCC** (5V)
  - **GND**
- Makes it easy to connect displays, sensors, and other I2C modules without soldering

## ⚠️ Notes

- Ensure you configure the correct GPIOs in your firmware for the push buttons.
- I2C headers share the same bus (default: GPIO21 for SDA and GPIO22 for SCL on most ESP32 boards).

## 📷 Preview

- [Google Drive](https://drive.google.com/drive/folders/1R_RpOR28A1k-9gBzrjRCwLDj6inaBXSF?usp=sharing)

## 🛠️ Applications

- Rapid prototyping
- IoT projects
- Sensor interfacing
- Educational purposes
