@echo off
REM Convert all MusicXML songs to Arduino format with articulation

cd /d "c:\Users\tegze\Documents\GitHub\202A\musescore-xml-arduino-converter"

REM Convert first song (creates arduino.ino)

node converter.js songs/olympic_fanfare.musicxml --articulation 2 arduino.ino



REM Append remaining songs
REM node converter.js songs/daisy_bell.musicxml --articulation 2 --append arduino.ino
REM node converter.js songs/the_happy_farmer.musicxml --articulation 2 --append arduino.ino
REM node converter.js songs/glasgow_city_police_pipers.musicxml --articulation 2 --append arduino.ino
REM node converter.js songs/homecoming.musicxml --articulation 2 --append arduino.ino
REM node converter.js songs/laundry_machine.musicxml --articulation 2 --append arduino.ino
REM node converter.js songs/simple_gifts.musicxml --articulation 2 --append arduino.ino

node converter.js songs/morning_mood.musicxml --articulation 2 --append arduino.ino
node converter.js songs/william_tell_overture.musicxml --articulation 2 --append arduino.ino
node converter.js songs/home_on_the_range.musicxml --articulation 2 --append arduino.ino
REM node converter.js songs/crossing_field.musicxml --articulation 2 --append arduino.ino
echo All songs converted and appended to arduino.ino
pause
