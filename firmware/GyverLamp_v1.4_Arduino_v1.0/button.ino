boolean brightDirection, speedDirection, scaleDirection;
//byte numHold;
#ifdef DEBUG
void debugPrint(){
    Serial.print(numHold);
    Serial.print(F(" brightness:"));
    Serial.print(modes[currentMode].brightness);
    Serial.print(F(" speed:"));
    Serial.print(modes[currentMode].speed);
    Serial.print(F(" scale:"));
    Serial.print(modes[currentMode].scale);
    Serial.print(F(" numHold_Timer:"));
    Serial.println(numHold_Timer);
}
#endif
void changeDirection(byte numHold){
  switch(numHold){
    case 0: case 1: brightDirection = !brightDirection; break;
    case 2: speedDirection = !speedDirection; break;
    case 3: scaleDirection = !scaleDirection; break;
  }
  numHold_Timer = millis();
}

void buttonTick() {
  touch.tick();
  
  if (!ONflag) { // Обработка из выключенного состояния
    #ifdef DEBUG
    if(touch.isPress())
      Serial.println(F("Off state"));
    #endif
    
    if (touch.isDouble()) { // Демо-режим, с переключением каждые 30 секунд для двойного клика в выключенном состоянии
      numHold = 254;
      currentMode = random(0, 255)%17; // 17 скипаем
      //modes[currentMode].brightness = BRIGHTNESS/2; // в половину яркости
      FastLED.setBrightness(modes[currentMode].brightness);
      ONflag = true;
      userTimer = millis(); // момент включения для таймаута в DEMOTIME
      numHold_Timer = millis();
      //brightDirection = 0; // на уменьшение
      changePower();
      #ifdef DEBUG
        Serial.print(F("Demo mode: "));
        Serial.println(currentMode);
      #endif
    }
    
    if (touch.isHolded()) {
      #ifdef DEBUG
        Serial.println(F("Holdeded from offed state"));
      #endif
      currentMode = 17;
      modes[currentMode].brightness = BRIGHTNESS;
      FastLED.setBrightness(modes[currentMode].brightness);
      numHold = 255;
      ONflag = true;
      userTimer = millis(); // момент включения для таймаута в переключения в режим регулировки яркости
      numHold_Timer = millis();
      brightDirection = 0; // на уменьшение
      changePower();
    }
  }  

  if (touch.isSingle()) { // Включение/выключение одиночным

    {
      if (ONflag) {
        ONflag = false;
        #ifdef DEBUG
          Serial.println(F("Off lamp"));
        #endif
        changePower();
      } else {
        ONflag = true;
        #ifdef DEBUG
          Serial.println(F("On lamp"));
          debugPrint(); // отладка
        #endif
        
      }
    }
  }

if (ONflag) {                 // если включено
  if (touch.isDouble()) {
    #ifdef DEBUG
      Serial.println(F("Double click"));
      //debugPrint(); // отладка
    #endif
    if (++currentMode >= MODE_AMOUNT) currentMode = 0;
    FastLED.setBrightness(modes[currentMode].brightness);
    loadingFlag = true;
    //settChanged = true;
    FastLED.clear();
    delay(1);
  }
  if (touch.isTriple()) {
    #ifdef DEBUG
      Serial.println(F("Triple click"));
      //debugPrint(); // отладка
    #endif
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

  if (touch.isHolded() && numHold != 255) {  // изменение яркости при удержании кнопки
      if(modes[currentMode].brightness == BRIGHTNESS && numHold == 1){
        brightDirection = 0;
      } else if (modes[currentMode].brightness <= 1 && numHold == 1){
        brightDirection = 1;
      } else
        changeDirection(numHold);
      if(!numHold || numHold==254)
        numHold = 1;
  }

  if (touch.isHolded2()) {  // изменение скорости "speed" при двойном нажатии и удержании кнопки
      if(modes[currentMode].speed == 255 && numHold == 2){
        speedDirection = 0;
      } else if (modes[currentMode].speed <= 1 && numHold == 2){
        speedDirection = 1;
      } else
        changeDirection(numHold);
    if(!numHold)
      numHold = 2;
  }

  if (touch.isHolded3()) {  // изменение масштаба "scale" при тройном нажатии и удержании кнопки
      if(modes[currentMode].scale == 255 && numHold == 3){
        scaleDirection = 0;
      } else if (modes[currentMode].scale <= 1 && numHold == 3){
        scaleDirection = 1;
      } else
        changeDirection(numHold);
    if(!numHold)
      numHold = 3;
  }

  if (touch.isStep()) {
    #ifdef DEBUG
      debugPrint(); // отладка
    #endif
    if (numHold != 0 && numHold != 255) {
      numHold_Timer = millis();
      loadingFlag = true;
    }
    
    switch (numHold) {
      case 1:
  
          modes[currentMode].brightness = constrain(modes[currentMode].brightness + (modes[currentMode].brightness / 25 + 1) * (brightDirection * 2 - 1), MINBRIGHTNESS , BRIGHTNESS);
        break;

      case 2:

        modes[currentMode].speed = constrain(modes[currentMode].speed + (modes[currentMode].speed / 25 + 1) * (speedDirection * 2 - 1), 1 , 255);
        break;

      case 3:
    
        modes[currentMode].scale = constrain(modes[currentMode].scale + (modes[currentMode].scale / 25 + 1) * (scaleDirection * 2 - 1), 1 , 255);
        break;
    }
  }
    
    if ((millis() - userTimer) > numHold_Time && numHold>=250) {
        if(numHold == 255){
          numHold = 1;
          userTimer = millis();
          numHold_Timer = millis();
        }
    }
    
    if ((millis() - numHold_Timer) > numHold_Time && !touch.isHolded() && numHold<250) {
        numHold = 0;
        numHold_Timer = millis();
    }
    
    if((millis() - userTimer > DEMOTIME*1000) && (numHold == 254)){
      //FastLED.clear();
      //delay(2);
      for(byte i = 250; i>10; i-=10){
        fader(30);
        FastLED.delay(33);
        //FastLED.show();
      }
          
      if(RANDOM_DEMO)
        currentMode = random(0, 255)%17; // 17 скипаем
      else
        currentMode=(currentMode+1)%17; // 17 скипаем и идем по наростанию
      #ifdef DEBUG
        Serial.print(F("Demo mode: "));
        Serial.println(currentMode);
      #endif
      userTimer = millis(); 
    }
   
    FastLED.setBrightness(modes[currentMode].brightness);
  }
}
