#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <stdbool.h>

// Intervalo para re-sincronización en segundos (por ejemplo, 1 hora)
#define NTP_SYNC_INTERVAL 3600
#define NTP_SERVER "pool.ntp.org"  // Servidor NTP por defecto

// Inicializa el cliente NTP al arranque
void ntp_client_init(const char *ntp_server);

// Verifica si la sincronización NTP sigue siendo válida
bool ntp_client_needs_resync(void);

// Fuerza la re-sincronización con el servidor NTP si es necesario
void ntp_client_resync(const char *ntp_server);

bool ntp_client_is_synced(void);

#endif // NTP_CLIENT_H