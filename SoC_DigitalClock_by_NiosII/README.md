# Project: Real-Time Digital Clock using Nios II

## 🕒 Description

This project uses the **Nios II soft processor** to build a **real-time digital clock** with the following capabilities:

- Timekeeping based on the processor's hardware timer
- Displaying time in hour:minute:second format
- Allowing users to **modify the current time**
- Supporting **alarm setup and activation**
- Fully controlled using **hardware interrupts** from peripherals such as:
  - Push buttons
  - Switches
  - UART (serial communication)

## ⚙️ Hardware Used

- FPGA development board with **Nios II processor** (e.g., DE2, DE2-115)
- Peripherals:
  - Switches (PIO)
  - Buttons (PIO)
  - UART
  - LCD display
  - 7-segment LEDs
  - Timer
  - Red and green LEDs

## 🧠 Main Features

- ⏱ **Real-time clock display** on LCD or 7-segment LEDs
- 🔧 **Time-setting mode** via switches and buttons:
  - Increment/decrement hour, minute, second
  - Increment/decrement day, month, year
- ⏰ **Alarm function**:
  - Store alarm time and trigger when it matches the current time
  - Turn on LED and send alert via UART when alarm is triggered
- 🔄 **UART communication**:
  - Allows time adjustment from a PC using terminal software

## 🛠 System Operation

- **Timer interrupt**: Updates the second every 1 second
- **Button/switch interrupts**: Handles user input (e.g., select display mode, adjust time)
- **UART interrupt**: Receives commands from terminal to control time and alarm

## 🖥 UART Interface
- Using Hercules App

## 🧩 Hardware Diagram
- [Miro](https://miro.com/app/board/uXjVI-5-9uE=/?focusWidget=3458764625888600384)

## 🛠️ Quartus and Qsys
- [Google Drive](https://drive.google.com/drive/folders/1uxAxpu0XRx3GzTwCHQcGSDX4you7m7gd?usp=drive_link)

## 🎬 Video demo
- [Youtube](https://youtu.be/Ni0YKa5ghsg)