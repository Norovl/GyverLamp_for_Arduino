boolean brightDirection, speedDirection, scaleDirection;
//byte numHold;

void buttonTick() {
  touch.tick();

  if (touch.isSingle()) {
    //    if (dawnFlag) {
    //      //manualOff = true;
    //      dawnFlag = false;
    //      loadingFlag = true;
    //      FastLED.setBrightness(modes[currentMode].brightness);
    //      changePower();
    //    } else
    {
      if (ONflag) {
        ONflag = false;
        changePower();
      } else {
        ONflag = true;
        changePower();
      }
    }
  }

if (ONflag) {                 // если включено
  if (touch.isDouble()) {
    if (++currentMode >= MODE_AMOUNT) currentMode = 0;
    FastLED.setBrightness(modes[currentMode].brightness);
    loadingFlag = true;
    //settChanged = true;
    FastLED.clear();
    delay(1);
  }
  if (touch.isTriple()) {
    if (--currentMode < 0) currentMode = MODE_AMOUNT - 1;
    FastLED.setBrightness(modes[currentMode].brightness);
    loadingFlag = true;
    //settChanged = true;
    FastLED.clear();
    delay(1);
  }

    if ((touch.hasClicks()) && (touch.getClicks() == 5)) {      // если было пятикратное нажатие на кнопку, то производим сохранение параметров // && (touch.hasClicks())
    if (EEPROM.read(0) != 102) EEPROM.write(0, 102);
    if (EEPROM.read(1) != currentMode) EEPROM.write(1, currentMode);  // запоминаем текущий эфект
    for (byte x = 0; x < MODE_AMOUNT; x++) {                          // сохраняем настройки всех режимов
      if (EEPROM.read(x * 3 + 11) != modes[x].brightness) EEPROM.write(x * 3 + 11, modes[x].brightness);
      if (EEPROM.read(x * 3 + 12) != modes[x].speed) EEPROM.write(x * 3 + 12, modes[x].speed);
      if (EEPROM.read(x * 3 + 13) != modes[x].scale) EEPROM.write(x * 3 + 13, modes[x].scale);
    }
    // индикация сохранения
    ONflag = false;
    changePower();
    delay(200);
    ONflag = true;
    changePower();
  }

  if (touch.isHolded()) {  // изменение яркости при удержании кнопки
    brightDirection = !brightDirection;
    numHold = 1;
  }

  if (touch.isHolded2()) {  // изменение скорости "speed" при двойном нажатии и удержании кнопки
    speedDirection = !speedDirection;
    numHold = 2;
  }

  if (touch.isHolded3()) {  // изменение масштаба "scale" при тройном нажатии и удержании кнопки
    scaleDirection = !scaleDirection;
    numHold = 3;
  }

  if (touch.isStep()) {
//    Serial.print(numHold);
//    Serial.print(" brightness:");
//    Serial.print(modes[currentMode].brightness);
//    Serial.print(" speed:");
//    Serial.print(modes[currentMode].speed);
//    Serial.print(" scale:");
//    Serial.println(modes[currentMode].scale);
    if (numHold != 0) numHold_Timer = millis(); loadingFlag = true;
    switch (numHold) {
      case 1:
        //                if (brightDirection) {
        //                  if (modes[currentMode].brightness < 10) modes[currentMode].brightness += 1;
        //                  else if (modes[currentMode].brightness < 250) modes[currentMode].brightness += 5;
        //                  else modes[currentMode].brightness = 255;
        //                } else {
        //                  if (modes[currentMode].brightness > 15) modes[currentMode].brightness -= 5;
        //                  else if (modes[currentMode].brightness > 1) modes[currentMode].brightness -= 1;
        //                  else modes[currentMode].brightness = 1;
        //                }
        modes[currentMode].brightness = constrain(modes[currentMode].brightness + (modes[currentMode].brightness / 25 + 1) * (brightDirection * 2 - 1), 1 , 255);
        //        byte x = sqrt(modes[currentMode].brightness);
        //        for (byte y = 0; y < HEIGHT - 1; y++) {
        //          if (x < y) drawPixelXY(1, y, CHSV(10,200,200));
        //             else drawPixelXY(1, y, CHSV(1,0,0));
        //        }
        break;

      case 2:
        //                if (speedDirection) {
        //                  if (modes[currentMode].speed < 10) modes[currentMode].speed += 1;
        //                  else if (modes[currentMode].speed < 250) modes[currentMode].speed += 5;
        //                  else modes[currentMode].speed = 255;
        //                } else {
        //                  if (modes[currentMode].speed > 15) modes[currentMode].speed -= 5;
        //                  else if (modes[currentMode].speed > 1) modes[currentMode].speed -= 1;
        //                  else modes[currentMode].speed = 1;
        //                }
        modes[currentMode].speed = constrain(modes[currentMode].speed + (modes[currentMode].speed / 25 + 1) * (speedDirection * 2 - 1), 1 , 255);
        break;

      case 3:
        //                if (scaleDirection) {
        //                  if (modes[currentMode].scale < 10) modes[currentMode].scale += 1;
        //                  else if (modes[currentMode].scale < 250) modes[currentMode].scale += 5;
        //                  else modes[currentMode].scale = 255;
        //                } else {
        //                  if (modes[currentMode].scale > 15) modes[currentMode].scale -= 5;
        //                  else if (modes[currentMode].scale > 1) modes[currentMode].scale -= 1;
        //                  else modes[currentMode].scale = 1;
        //                }
        modes[currentMode].scale = constrain(modes[currentMode].scale + (modes[currentMode].scale / 25 + 1) * (scaleDirection * 2 - 1), 1 , 255);
        break;
    }
  }
    if ((millis() - numHold_Timer) > numHold_Time) {
      numHold = 0;
      numHold_Timer = millis();
    }
    FastLED.setBrightness(modes[currentMode].brightness);
    //settChanged = true;
  }
}
