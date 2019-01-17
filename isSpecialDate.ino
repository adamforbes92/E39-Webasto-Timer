extern String combinedDayMonthYear; extern String combinedHourMinute; extern String currentDayText; 
extern bool allowStart;
extern int startCombinedStartTime1; extern int startCombinedStartTime2;

bool isSpecialDate() {
  switch (combinedDayMonthYear.toInt()) {
    //C drops leading zero.  Compare for special dates.  DayMonth.  So Jan 01 is 11, Jan 02 is 12
    case 112020:                   // is 01 01 2020
      return false; break;
    case 112021:                   // is 01 01 2021
      return false; break;
    case 112022:                   // is 01 01 2022
      return false; break;

    case 17122018:                   // is 17 12 2018
      return false; break;
    case 18122018:                   // is 18 12 2018
      return false; break;
    case 19122018:                   // is 19 12 2018
      return false; break;
    case 20122018:                   // is 20 12 2018
      return false; break;
    case 2112018:                   // is 21 12 2018
      return false; break;
    case 22122018:                   // is 22 12 2018
      return false; break;
    case 23122018:                   // is 23 12 2018
      return false; break;
    case 24122018:                   // is 24 12 2018
      return false; break;
    case 25122018:                   // is 25 12 2018
      return false; break;
    case 26122018:                   // is 26 12 2018
      return false; break;
    case 27122018:                   // is 27 12 2018
      return false; break;
    case 28122018:                   // is 28 12 2018
      return false; break;
    case 29122018:                   // is 29 12 2018
      return false; break;
    case 30122018:                   // is 30 12 2018
      return false; break;
    case 31122018:                   // is 31 12 2018
      return false; break;

    case 412019:                   // is 1 1 2019
      return false; break;
    case 212019:                   // is 2 1 2019
      return false; break;
    case 312019:                   // is 3 1 2019
      return false; break;

    default:
      if ((combinedHourMinute.toInt() == startCombinedStartTime1) or (combinedHourMinute.toInt() == startCombinedStartTime2)) {
        DEBUG_PRINTLN(F("current combined hour minute = start time 1 or start time 2"));
        return true;
      }
      else {
        DEBUG_PRINTLN("Current combined hour minute = " + combinedHourMinute);
        DEBUG_PRINTLN("Waiting for " + String(startCombinedStartTime1) + " or " + String(startCombinedStartTime2));
        return false;
      }
  }
}
