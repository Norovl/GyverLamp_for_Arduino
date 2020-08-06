uint32_t effTimer;
byte ind;
void effectsTick() { { if (ONflag && millis() - effTimer >= ((currentMode < 5 || currentMode > 13) ? modes[currentMode].Speed : 50) ) {effTimer = millis(); switch (currentMode) {
//|номер   |название функции эффекта     |тоже надо|
   case 0 : sparklesRoutine();             break;
   case 1 : fireRoutine();                 break;
   case 3 : rainbowVertical();             break;
   case 4 : rainbowHorizontal();           break;
   case 5 : rainbowDiagonalRoutine();      break;
   case 6 : madnessNoise();                break;
   case 7 : cloudNoise();                  break;
   case 8 : lavaNoise();                   break;
   case 9 : plasmaNoise();                 break;
   case 10: rainbowNoise();                break;
   case 11: rainbowStripeNoise();          break;
   case 12: zebraNoise();                  break;
   case 13: forestNoise();                 break;
   case 14: oceanNoise();                  break;
   case 15: heatNoise();                   break;
   case 16: smokeNoise();                  break;
   case 17: lavLampNoise();                break; 
   case 18: colorRoutine();                break;
   case 19: colorsRoutine();               break;
   case 20: whiteLamp();                   break;
   case 21: matrixRoutine();               break;
   case 22: snowRoutine();                 break;
   case 23: stormRoutine2(true);           break;
   case 24: stormRoutine2(false);          break;
   case 25: ballRoutine();                 break;
   case 26: ballsRoutine();                break;
}
      switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
        case 1:
          ind = sqrt(modes[currentMode].Brightness + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(0, y, CHSV(10, 255, 255));
            else drawPixelXY(0, y,  0);
          }
          break;
        case 2:
          ind = sqrt(modes[currentMode].Speed - 1);
          for (byte y = 0; y <= HEIGHT ; y++) {
            if (ind <= y) drawPixelXY(0, 15 - y, CHSV(100, 255, 255));
            else drawPixelXY(0, 15 - y,  0);
          }
          break;
        case 3:
          ind = sqrt(modes[currentMode].Scale + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(0, y, CHSV(150, 255, 255));
            else drawPixelXY(0, y,  0);
          }
          break;

      }
      FastLED.show();
    }
  }  
}

void changePower() {    // плавное включение/выключение
  if (ONflag) {
    effectsTick();
    for (int i = 0; i < modes[currentMode].Brightness; i += 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.setBrightness(modes[currentMode].Brightness);
    delay(2);
    FastLED.show();
  } else {
    effectsTick();
    for (int i = modes[currentMode].Brightness; i > 8; i -= 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.clear();
    delay(2);
    FastLED.show();
  }
} 
