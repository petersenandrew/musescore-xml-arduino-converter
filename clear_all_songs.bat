@echo off
REM Clear all songs from arduino.ino

cd /d "c:\Users\tegze\Documents\GitHub\202A\musescore-xml-arduino-converter"

REM Clear all songs from arduino.ino
node clear_songs.js arduino.ino
echo All songs cleared from arduino.ino
pause
