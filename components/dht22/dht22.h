#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

#define SENSOR_TYPE DHT_TYPE_AM2301
#define DHT_GPIO GPIO_NUM_21

void dht22_init(void);
bool dht22_read(float *temperature, float *humidity);

#endif // DHT22_H