#ifndef DISPLAY_H

#define QRBMP "QRBMP.bmp"
#define TOTAL_MENU (sizeof(pszItems) / sizeof(pszItems[0]))

// #define DOMAIN "uruzpro.com"	// "cool.uruz.id" atau "hot.uruz.id" atau "uruzpro.com"
// #define PORT "443"	// 80 untuk "cool.uruz.id", 443 untuk "hot.uruz.id"
// #define PROTOCOL PROTOCOL_HTTPS // "PROTOCOL_HTTP" untuk "cool.uruz.id", "PROTOCOL_HTTPS" untuk "hot.uruz.id"

// #define DOMAIN "192.168.192.15"	// "cool.uruz.id" atau "hot.uruz.id" atau "uruzpro.com"
#define DOMAIN "cool.uruz.id"	// "cool.uruz.id" atau "hot.uruz.id" atau "uruzpro.com"
#define PORT "80"	// 80 untuk "cool.uruz.id", 443 untuk "hot.uruz.id"
#define PROTOCOL PROTOCOL_HTTP // "PROTOCOL_HTTP" untuk "cool.uruz.id", "PROTOCOL_HTTPS" untuk "hot.uruz.id"

extern cJSON *enabled_menu;
extern const char *pszItems[];
extern const int pszItemsCount;

extern int payment_amount;

int httpRequestJSON(int *packLen, char *extracted, const char *url, const char *method, cJSON *payload);
void CardNFC(const char *partnerReferenceNo, const int amount);

#endif // DISPLAY_H

