#include <ctime>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_leap_year(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int get_days_in_month(int month, int year) {
  switch (month) {
  case 1:
    return 31;
  case 2:
    return is_leap_year(year) ? 29 : 28;
  case 3:
    return 31;
  case 4:
    return 30;
  case 5:
    return 31;
  case 6:
    return 30;
  case 7:
    return 31;
  case 8:
    return 31;
  case 9:
    return 30;
  case 10:
    return 31;
  case 11:
    return 30;
  case 12:
    return 31;
  default:
    return 0;
  }
}

// Returns 0 if valid, negative error code if invalid
// -1: Invalid format or length
// -2: Invalid numbering (day/month out of range)
// -3: Future date
int validate_date_string(const char *date_str) {
  if (strlen(date_str) != 10)
    return -1;
  if (date_str[2] != '-' || date_str[5] != '-')
    return -1;

  for (int i = 0; i < 10; i++) {
    if (i == 2 || i == 5)
      continue;
    if (!isdigit(date_str[i]))
      return -1;
  }

  int day = atoi(date_str);
  int month = atoi(date_str + 3);
  int year = atoi(date_str + 6);

  if (month < 1 || month > 12)
    return -2;
  if (day < 1 || day > get_days_in_month(month, year))
    return -2;

  time_t t = time(NULL);
  struct tm tm_buf;
  struct tm *now = localtime_r(&t, &tm_buf);
  if (!now)
    return -1;
  int current_year = now->tm_year + 1900;
  int current_month = now->tm_mon + 1;
  int current_day = now->tm_mday;

  if (year > current_year)
    return -3;
  if (year == current_year) {
    if (month > current_month)
      return -3;
    if (month == current_month && day > current_day)
      return -3;
  }

  return 0;
}
