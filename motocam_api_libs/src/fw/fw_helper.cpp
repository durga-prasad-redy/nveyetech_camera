#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "fw/fw_system.h"
#include "fw/fw_sensor.h"
#define IR_FILE "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define SENSOR_FILE "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define ISP_FILE "/sys/class/thermal/thermal_zone0/temp"

// Structure representing a temperature and its corresponding volume level
typedef struct
{
  uint8_t temperature;  // Temperature in degrees Celsius
  uint16_t sensorlevel; // Corresponding sensor level
} TempSensorData;

// Constant data table
const TempSensorData tempSensorTable[] = {
    {25, 2712}, {27, 2639}, {29, 2566}, {31, 2492}, {33, 2418}, {35, 2344}, {37, 2269}, {39, 2195}, {41, 2121}, {43, 2048}, {45, 1975}, {47, 1904}, {49, 1833}, {51, 1764}, {53, 1696}, {55, 1630}, {57, 1565}, {59, 1502}, {61, 1440}, {63, 1381}, {65, 1323}, {67, 1267}, {69, 1213}, {71, 1161}, {73, 1111}, {75, 1062}, {77, 1016}, {79, 971}, {81, 928}, {83, 887}, {85, 847}, {87, 809}, {89, 773}, {91, 739}, {93, 706}, {95, 674}};

// Optional: number of entries
const size_t tempSensorTableSize =
    sizeof(tempSensorTable) / sizeof(tempSensorTable[0]);

uint16_t get_sysfs_read_val(const char *module)
{
  int val = 0;
  FILE *file = fopen(module, "r");

  if (file == NULL)
  {
    perror("Error opening file");
    return 1;
  }

  if (fscanf(file, "%u", &val) != 1)
  {
    fprintf(stderr, "Failed to read value\n");
    fclose(file);
    return 1;
  }

  fclose(file);

  return val;
}

uint8_t find_temperature(uint16_t target_sensor_value)
{
  size_t i;
  uint8_t closest_temp =
      tempSensorTable[0].temperature; // Sentinel for "Not Found"

  for (i = 0; i < tempSensorTableSize; ++i)
  {
    uint16_t sensor_val = tempSensorTable[i].sensorlevel;

    if (sensor_val > target_sensor_value)
    {
      continue;
    }
    else
    {
      closest_temp = tempSensorTable[i].temperature;
      break;
    }
  }

  return closest_temp;
}

int8_t get_ir_temp(uint8_t *temp)
{
  uint16_t val;

  pthread_mutex_lock(&lock);

  /* Get IR Temp details */
  val = get_sysfs_read_val(IR_FILE);

  *temp = find_temperature(val);

  pthread_mutex_unlock(&lock);

  return 0;
}

int8_t get_sensor_temp(uint8_t *temp)
{
  uint16_t val;

  pthread_mutex_lock(&lock);

  /* Get sensor Temp details */
  val = get_sysfs_read_val(SENSOR_FILE);

  *temp = find_temperature(val);

  printf("Sensor Temp: %d, Sensor Value: %d\n", *temp, val);
  pthread_mutex_unlock(&lock);

  return 0;
}

int8_t get_isp_temp(uint8_t *temp)
{
  pthread_mutex_lock(&lock);

  /* Get ISP Temp details */
  *temp = (uint8_t)get_sysfs_read_val(ISP_FILE);

  pthread_mutex_unlock(&lock);

  return 0;
}
