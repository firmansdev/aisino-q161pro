#ifndef MQTT_H
#define MQTT_H

#include "MQTTClient.h"
int mQTTMainThread(char *param, int *singleRun);
int mqttMainThreadV2();

extern MQTTClient g_mqttClient;
extern Network g_network;

#endif // MQTT_H

