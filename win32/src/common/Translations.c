#include "Translations.h"

#include <stdio.h>
#include <wchar.h>

typedef struct {
    const wchar_t *de;
    const wchar_t *en;
    const wchar_t *fr;
    const wchar_t *it;
    const wchar_t *es;
} TranslationEntry;

/* Indexed by StringId. Most entries here were carried over verbatim from
 * the Linux build's reviewed Qt translations (translations/NetworkTool_*.ts) so both
 * ports read consistently in every language; a handful are Win32-only
 * (native ICMP/DNS/Netstat messages the Linux build doesn't have, since it
 * shells out to ping/traceroute/host instead) and were translated fresh. */
static const TranslationEntry kTranslations[STR_COUNT] = {
    [STR_MENU_FILE] = {L"Ablage", L"File", L"Fichier", L"File", L"Archivo"},
    [STR_MENU_FILE_NEW] = {L"Neues Fenster", L"New Window", L"Nouvelle fenêtre", L"Nuova finestra",
                            L"Nueva ventana"},
    [STR_MENU_FILE_CLOSE] = {L"Schliessen", L"Close", L"Fermer", L"Chiudi", L"Cerrar"},
    [STR_MENU_EDIT] = {L"Bearbeiten", L"Edit", L"Édition", L"Modifica", L"Editar"},
    [STR_MENU_EDIT_UNDO] = {L"Widerrufen", L"Undo", L"Annuler", L"Annulla", L"Deshacer"},
    [STR_MENU_EDIT_REDO] = {L"Wiederholen", L"Redo", L"Rétablir", L"Ripeti", L"Rehacer"},
    [STR_MENU_EDIT_CUT] = {L"Ausschneiden", L"Cut", L"Couper", L"Taglia", L"Cortar"},
    [STR_MENU_EDIT_COPY] = {L"Kopieren", L"Copy", L"Copier", L"Copia", L"Copiar"},
    [STR_MENU_EDIT_PASTE] = {L"Einfügen", L"Paste", L"Coller", L"Incolla", L"Pegar"},
    [STR_MENU_EDIT_DELETE] = {L"Löschen", L"Delete", L"Supprimer", L"Elimina", L"Eliminar"},
    [STR_MENU_EDIT_SELECTALL] = {L"Alles Auswählen", L"Select All", L"Tout sélectionner",
                                  L"Seleziona tutto", L"Seleccionar todo"},
    [STR_MENU_VIEW] = {L"Darstellung", L"View", L"Affichage", L"Visualizza", L"Ver"},
    [STR_MENU_VIEW_FULLSCREEN] = {L"Vollbildmodus", L"Full Screen", L"Plein écran", L"Schermo intero",
                                   L"Pantalla completa"},
    [STR_MENU_HELP] = {L"Hilfe", L"Help", L"Aide", L"Aiuto", L"Ayuda"},
    [STR_MENU_HELP_SETTINGS] = {L"Einstellungen", L"Settings", L"Paramètres", L"Impostazioni",
                                 L"Configuración"},
    [STR_MENU_HELP_HELP] = {L"Hilfe", L"Help", L"Aide", L"Aiuto", L"Ayuda"},
    [STR_MENU_HELP_ABOUT] = {L"Über", L"About", L"À propos", L"Informazioni", L"Acerca de"},
    [STR_HELP_TITLE] = {L"Hilfe", L"Help", L"Aide", L"Aiuto", L"Ayuda"},
    [STR_HELP_BODY] =
        {L"Wähle oben in der Leiste ein Werkzeug aus (Info, Netstat, Ping, Lookup, Traceroute, Whois, "
         L"Finger, Port Scan, Speed), gib die benötigten Angaben ein und starte die Aktion über den "
         L"jeweiligen Knopf. Die Ausgabe erscheint im Textfeld darunter.",
         L"Choose a tool from the bar above (Info, Netstat, Ping, Lookup, Traceroute, Whois, Finger, Port Scan, "
         L"Speed), enter the required details, and start the action with the corresponding button. The output "
         L"appears in the text field below.",
         L"Choisissez un outil dans la barre ci-dessus (Info, Netstat, Ping, Lookup, Traceroute, Whois, Finger, "
         L"Port Scan, Speed), saisissez les informations nécessaires et démarrez l'action avec le "
         L"bouton correspondant. Le résultat s'affiche dans le champ de texte ci-dessous.",
         L"Scegli uno strumento dalla barra in alto (Info, Netstat, Ping, Lookup, Traceroute, Whois, Finger, "
         L"Port Scan, Speed), inserisci i dati richiesti e avvia l'azione con il pulsante corrispondente. "
         L"L'output viene visualizzato nel campo di testo sottostante.",
         L"Elige una herramienta en la barra superior (Info, Netstat, Ping, Lookup, Traceroute, Whois, Finger, "
         L"Port Scan, Speed), introduce los datos necesarios e inicia la acción con el botón "
         L"correspondiente. La salida aparece en el campo de texto de abajo."},
    [STR_ABOUT_TITLE] = {L"Über Network Tool", L"About Network Tool", L"À propos de Network Tool",
                          L"Informazioni su Network Tool", L"Acerca de Network Tool"},
    [STR_ABOUT_VERSION] = {L"Version 1.0.0", L"Version 1.0.0", L"Version 1.0.0", L"Versione 1.0.0",
                            L"Versión 1.0.0"},
    [STR_ABOUT_LICENSE] = {L"Lizenz: MIT", L"License: MIT", L"Licence : MIT", L"Licenza: MIT",
                            L"Licencia: MIT"},
    [STR_ABOUT_DISCLAIMER_TITLE] = {L"Haftungsausschluss", L"Disclaimer", L"Clause de non-responsabilité",
                                     L"Esclusione di responsabilità", L"Descargo de responsabilidad"},
    [STR_ABOUT_DISCLAIMER_BODY] =
        {L"Diese Software wird ohne jegliche Garantie oder Gewährleistung bereitgestellt, weder "
         L"ausdrücklich noch stillschweigend, einschließlich, aber nicht beschränkt auf die "
         L"Gewährleistung der Marktgängigkeit, der Eignung für einen bestimmten Zweck und der "
         L"Nichtverletzung von Rechten Dritter. Die Nutzung erfolgt auf eigenes Risiko. Der Autor übernimmt "
         L"keine Haftung für Schäden oder Folgeschäden, die durch die Nutzung oder "
         L"Unmöglichkeit der Nutzung dieser Software entstehen, und lehnt jegliche Haftung ab.",
         L"This software is provided without any warranty or guarantee of any kind, whether express or "
         L"implied, including but not limited to the warranties of merchantability, fitness for a particular "
         L"purpose, and non-infringement of third-party rights. Use is at your own risk. The author accepts no "
         L"liability for any damages or consequential damages arising from the use or inability to use this "
         L"software, and disclaims all liability.",
         L"Ce logiciel est fourni sans aucune garantie, expresse ou implicite, y compris, mais sans s'y "
         L"limiter, les garanties de qualité marchande, d'adéquation à un usage particulier et "
         L"de non-contrefaçon. L'utilisation se fait à vos propres risques. L'auteur décline "
         L"toute responsabilité pour les dommages résultant de l'utilisation ou de l'impossibilité "
         L"d'utiliser ce logiciel.",
         L"Questo software viene fornito senza alcuna garanzia, espressa o implicita, incluse ma non limitate "
         L"alle garanzie di commerciabilità, idoneità per uno scopo particolare e non violazione. "
         L"L'uso è a proprio rischio. L'autore non si assume alcuna responsabilità per danni derivanti "
         L"dall'uso o dall'impossibilità di utilizzare questo software.",
         L"Este software se proporciona sin garantía de ningún tipo, expresa o implícita, "
         L"incluyendo pero sin limitarse a las garantías de comerciabilidad, idoneidad para un propósito "
         L"particular y no infracción. El uso es bajo tu propio riesgo. El autor no acepta responsabilidad "
         L"por daños derivados del uso o la imposibilidad de usar este software."},

    [STR_INFO_SELECT] = {L"Bitte wähle eine Netzwerkschnittstelle für Informationen aus:",
                          L"Please select a network interface for information:",
                          L"Veuillez sélectionner une interface réseau pour obtenir des "
                          L"informations :",
                          L"Seleziona un'interfaccia di rete per le informazioni:",
                          L"Selecciona una interfaz de red para obtener información:"},
    [STR_INFO_GROUP_IFACE] = {L"Schnittstelleninformationen", L"Interface Information",
                               L"Informations sur l'interface", L"Informazioni sull'interfaccia",
                               L"Información de la interfaz"},
    [STR_INFO_MAC] = {L"Hardware-Adresse:", L"Hardware Address:", L"Adresse matérielle :",
                       L"Indirizzo hardware:", L"Dirección de hardware:"},
    [STR_INFO_IP] = {L"IP-Adresse(n):", L"IP Address(es):", L"Adresse(s) IP :", L"Indirizzo/i IP:",
                      L"Dirección(es) IP:"},
    [STR_INFO_LINKSPEED] = {L"Verbindungsgeschwindigkeit:", L"Link Speed:", L"Vitesse de liaison :",
                             L"Velocità del collegamento:", L"Velocidad de enlace:"},
    [STR_INFO_LINKSTATUS] = {L"Verbindungsstatus:", L"Link Status:", L"État de la liaison :",
                              L"Stato del collegamento:", L"Estado del enlace:"},
    [STR_INFO_MTU] = {L"MTU:", L"MTU:", L"MTU :", L"MTU:", L"MTU:"},
    [STR_INFO_GROUP_STATS] = {L"Übertragungsstatistik", L"Transfer Statistics",
                               L"Statistiques de transfert", L"Statistiche di trasferimento",
                               L"Estadísticas de transferencia"},
    [STR_INFO_RXPACKETS] = {L"Empfangene Pakete:", L"Packets Received:", L"Paquets reçus :",
                             L"Pacchetti ricevuti:", L"Paquetes recibidos:"},
    [STR_INFO_RXERR] = {L"Empfangsfehler:", L"Receive Errors:", L"Erreurs de réception :",
                         L"Errori di ricezione:", L"Errores de recepción:"},
    [STR_INFO_TXPACKETS] = {L"Gesendete Pakete:", L"Packets Sent:", L"Paquets envoyés :",
                             L"Pacchetti inviati:", L"Paquetes enviados:"},
    [STR_INFO_TXERR] = {L"Sendefehler:", L"Send Errors:", L"Erreurs d'envoi :", L"Errori di invio:",
                         L"Errores de envío:"},
    [STR_INFO_NONE] = {L"keine", L"none", L"aucune", L"nessuno", L"ninguno"},
    [STR_INFO_NA] = {L"N/A", L"N/A", L"N/A", L"N/D", L"N/D"},
    [STR_INFO_ACTIVE_OPERATIONAL] = {L"Aktiv (betriebsbereit)", L"Active (operational)", L"Actif (opérationnel)",
                                      L"Attivo (operativo)", L"Activo (operativo)"},
    [STR_INFO_ACTIVE_CONNECTED] = {L"Aktiv (verbunden)", L"Active (connected)", L"Actif (connecté)",
                                    L"Attivo (connesso)", L"Activo (conectado)"},
    [STR_INFO_CONNECTING] = {L"Verbindet...", L"Connecting", L"Connexion en cours",
                              L"Connessione in corso", L"Conectando"},
    [STR_INFO_INACTIVE_DISCONNECTED] = {L"Inaktiv (getrennt)", L"Inactive (disconnected)",
                                         L"Inactif (déconnecté)", L"Inattivo (disconnesso)",
                                         L"Inactivo (desconectado)"},
    [STR_INFO_INACTIVE_UNREACHABLE] = {L"Inaktiv (nicht erreichbar)", L"Inactive (unreachable)",
                                        L"Inactif (inaccessible)", L"Inattivo (non raggiungibile)",
                                        L"Inactivo (inaccesible)"},
    [STR_INFO_INACTIVE_NONOPERATIONAL] = {L"Inaktiv (nicht betriebsbereit)", L"Inactive (non-operational)",
                                           L"Inactif (non opérationnel)", L"Inattivo (non operativo)",
                                           L"Inactivo (no operativo)"},
    [STR_INFO_UNKNOWN] = {L"Unbekannt", L"Unknown", L"Inconnu", L"Sconosciuto", L"Desconocido"},

    [STR_NET_ROUTE] = {L"Routing-Tabelle anzeigen", L"Display routing table information",
                        L"Afficher les informations de la table de routage",
                        L"Mostra le informazioni della tabella di routing",
                        L"Mostrar información de la tabla de enrutamiento"},
    [STR_NET_STATS] = {L"Umfassende Netzwerkstatistiken für jedes Protokoll anzeigen",
                        L"Display comprehensive network statistics for each protocol",
                        L"Afficher des statistiques réseau complètes pour chaque protocole",
                        L"Mostra statistiche di rete complete per ogni protocollo",
                        L"Mostrar estadísticas de red completas para cada protocolo"},
    [STR_NET_CONN] = {L"Aktive TCP-/UDP-Verbindungen anzeigen", L"Display active TCP/UDP connections",
                       L"Afficher les connexions TCP/UDP actives", L"Mostra le connessioni TCP/UDP attive",
                       L"Mostrar las conexiones TCP/UDP activas"},
    [STR_NET_BTN] = {L"Netstat", L"Netstat", L"Netstat", L"Netstat", L"Netstat"},
    [STR_NET_ROUTE_HEADER] = {L"Ziel               Netzmaske          Gateway            Schnittstelle  Metrik",
                               L"Destination        Netmask            Gateway            Interface  Metric",
                               L"Destination        Masque             Passerelle         Interface  Métrique",
                               L"Destinazione       Maschera           Gateway            Interfaccia  Metrica",
                               L"Destino            Máscara           Puerta de enlace   Interfaz  Métrica"},
    [STR_NET_NOROUTE] = {L"(keine Routing-Tabelle verfügbar)", L"(no routing table available)",
                          L"(aucune table de routage disponible)", L"(nessuna tabella di routing disponibile)",
                          L"(no hay tabla de enrutamiento disponible)"},
    [STR_NET_IPSTATS] = {L"-- IP-Statistiken --", L"-- IP statistics --", L"-- Statistiques IP --",
                          L"-- Statistiche IP --", L"-- Estadísticas IP --"},
    [STR_NET_PKT_RECEIVED] = {L"Empfangene Pakete:", L"Packets received:", L"Paquets reçus :",
                               L"Pacchetti ricevuti:", L"Paquetes recibidos:"},
    [STR_NET_PKT_FORWARDED] = {L"Weitergeleitete Pakete:", L"Packets forwarded:", L"Paquets transférés :",
                                L"Pacchetti inoltrati:", L"Paquetes reenviados:"},
    [STR_NET_PKT_DELIVERED] = {L"Zugestellte Pakete:", L"Packets delivered:", L"Paquets distribués :",
                                L"Pacchetti consegnati:", L"Paquetes entregados:"},
    [STR_NET_PKT_SENT] = {L"Gesendete Pakete:", L"Packets sent:", L"Paquets envoyés :",
                           L"Pacchetti inviati:", L"Paquetes enviados:"},
    [STR_NET_DISCARDED] = {L"Verworfen (ein/aus):", L"Discarded (in/out):", L"Rejetés (entr./sort.) :",
                            L"Scartati (in/out):", L"Descartados (ent./sal.):"},
    [STR_NET_TCPSTATS] = {L"-- TCP-Statistiken --", L"-- TCP statistics --", L"-- Statistiques TCP --",
                           L"-- Statistiche TCP --", L"-- Estadísticas TCP --"},
    [STR_NET_ACTIVE_OPENS] = {L"Aktive Öffnungen:", L"Active opens:", L"Ouvertures actives :",
                               L"Aperture attive:", L"Aperturas activas:"},
    [STR_NET_PASSIVE_OPENS] = {L"Passive Öffnungen:", L"Passive opens:", L"Ouvertures passives :",
                                L"Aperture passive:", L"Aperturas pasivas:"},
    [STR_NET_FAILED_CONN] = {L"Fehlgeschlagene Verbindungsversuche:", L"Failed connection attempts:",
                              L"Tentatives de connexion échouées :", L"Tentativi di connessione falliti:",
                              L"Intentos de conexión fallidos:"},
    [STR_NET_RESETS_SENT] = {L"Gesendete Resets:", L"Resets sent:", L"Réinitialisations envoyées :",
                              L"Reset inviati:", L"Reinicios enviados:"},
    [STR_NET_SEGMENTS_RECEIVED] = {L"Empfangene Segmente:", L"Segments received:", L"Segments reçus :",
                                    L"Segmenti ricevuti:", L"Segmentos recibidos:"},
    [STR_NET_SEGMENTS_SENT] = {L"Gesendete Segmente:", L"Segments sent:", L"Segments envoyés :",
                                L"Segmenti inviati:", L"Segmentos enviados:"},
    [STR_NET_SEGMENTS_RETRANS] = {L"Erneut gesendete Segmente:", L"Segments retransmitted:",
                                   L"Segments retransmis :", L"Segmenti ritrasmessi:",
                                   L"Segmentos retransmitidos:"},
    [STR_NET_CURRENT_CONN] = {L"Aktuelle Verbindungen:", L"Current connections:", L"Connexions actuelles :",
                               L"Connessioni attuali:", L"Conexiones actuales:"},
    [STR_NET_UDPSTATS] = {L"-- UDP-Statistiken --", L"-- UDP statistics --", L"-- Statistiques UDP --",
                           L"-- Statistiche UDP --", L"-- Estadísticas UDP --"},
    [STR_NET_DGRAMS_RECEIVED] = {L"Empfangene Datagramme:", L"Datagrams received:", L"Datagrammes reçus :",
                                  L"Datagrammi ricevuti:", L"Datagramas recibidos:"},
    [STR_NET_NOPORT] = {L"Kein Port / unzustellbar:", L"No port / undeliverable:", L"Aucun port / non "
                         L"remis :", L"Nessuna porta / non recapitabile:", L"Sin puerto / no entregable:"},
    [STR_NET_DGRAMS_SENT] = {L"Gesendete Datagramme:", L"Datagrams sent:", L"Datagrammes envoyés :",
                              L"Datagrammi inviati:", L"Datagramas enviados:"},
    [STR_NET_CONN_HEADER] = {L"Proto  Lokale Adresse          Remote-Adresse          Status",
                              L"Proto  Local Address           Remote Address          State",
                              L"Proto  Adresse locale           Adresse distante        État",
                              L"Proto  Indirizzo locale         Indirizzo remoto        Stato",
                              L"Proto  Dirección local          Dirección remota         Estado"},

    [STR_PING_LABEL] = {L"Gib eine Netzwerkadresse zum Pingen ein:", L"Enter a network address to ping:",
                         L"Saisissez une adresse réseau à pinguer :",
                         L"Inserisci un indirizzo di rete da pingare:",
                         L"Introduce una dirección de red para hacer ping:"},
    [STR_PING_BTN] = {L"Ping", L"Ping", L"Ping", L"Ping", L"Ping"},
    [STR_PING_SENDONLY] = {L"Nur senden", L"Send only", L"Envoyer seulement", L"Solo invio", L"Solo enviar"},
    [STR_PING_PINGS] = {L"Pings", L"pings", L"pings", L"ping", L"pings"},
    [STR_PING_STOP] = {L"Stopp", L"Stop", L"Arrêter", L"Interrompi", L"Detener"},
    [STR_PING_NOADDR] = {L"Bitte gib eine Netzwerkadresse ein.", L"Please enter a network address.",
                          L"Veuillez saisir une adresse réseau.", L"Inserisci un indirizzo di rete.",
                          L"Introduce una dirección de red."},
    [STR_PING_NOTFOUND] = {L"Ping: Host konnte nicht gefunden werden %s.", L"Ping request could not find host %s.",
                            L"Ping : hôte introuvable %s.", L"Ping: host non trovato %s.",
                            L"Ping: no se pudo encontrar el host %s."},
    [STR_PING_NOICMP] = {L"ICMP-Handle konnte nicht erstellt werden.", L"Could not create ICMP handle.",
                          L"Impossible de créer le descripteur ICMP.",
                          L"Impossibile creare l'handle ICMP.", L"No se pudo crear el identificador ICMP."},
    [STR_PING_TIMEDOUT] = {L"Zeitüberschreitung.", L"Request timed out.", L"Délai d'attente dépassé.",
                            L"Richiesta scaduta.", L"Tiempo de espera agotado."},
    [STR_PING_FAILED] = {L"Ping fehlgeschlagen (Fehler %lu).", L"Ping failed (error %lu).",
                          L"Échec du ping (erreur %lu).", L"Ping non riuscito (errore %lu).",
                          L"Ping fallido (error %lu)."},
    [STR_PING_REPLY] = {L"Antwort von %s: Bytes=%lu Zeit=%lums TTL=%u",
                         L"Reply from %s: bytes=%lu time=%lums TTL=%u",
                         L"Réponse de %s : octets=%lu temps=%lums TTL=%u",
                         L"Risposta da %s: byte=%lu durata=%lums TTL=%u",
                         L"Respuesta desde %s: bytes=%lu tiempo=%lums TTL=%u"},

    [STR_LOOKUP_LABEL] = {L"Gib eine Internetadresse für die Abfrage ein:",
                           L"Enter an internet address to lookup:",
                           L"Saisissez une adresse internet à rechercher :",
                           L"Inserisci un indirizzo internet da cercare:",
                           L"Introduce una dirección de internet para buscar:"},
    [STR_LOOKUP_TYPELABEL] = {L"Datensatztyp:", L"Record type:", L"Type d'enregistrement :", L"Tipo di record:",
                               L"Tipo de registro:"},
    [STR_LOOKUP_BTN] = {L"Abfragen", L"Lookup", L"Rechercher", L"Cerca", L"Buscar"},
    [STR_LOOKUP_NOADDR] = {L"Bitte gib eine Internetadresse ein.", L"Please enter an internet address.",
                            L"Veuillez saisir une adresse internet.", L"Inserisci un indirizzo internet.",
                            L"Introduce una dirección de internet."},
    [STR_LOOKUP_FAILED] = {L"Abfrage fehlgeschlagen.", L"Lookup failed.", L"Échec de la recherche.",
                            L"Ricerca non riuscita.", L"Error en la búsqueda."},
    [STR_LOOKUP_TYPE_DEFAULT] = {L"Standard (A/AAAA)", L"Default (A/AAAA)", L"Par défaut (A/AAAA)",
                                  L"Predefinito (A/AAAA)", L"Predeterminado (A/AAAA)"},
    [STR_LOOKUP_TYPE_A] = {L"IPv4-Adresse (A)", L"IPv4 Address (A)", L"Adresse IPv4 (A)",
                            L"Indirizzo IPv4 (A)", L"Dirección IPv4 (A)"},
    [STR_LOOKUP_TYPE_AAAA] = {L"IPv6-Adresse (AAAA)", L"IPv6 Address (AAAA)", L"Adresse IPv6 (AAAA)",
                               L"Indirizzo IPv6 (AAAA)", L"Dirección IPv6 (AAAA)"},
    [STR_LOOKUP_TYPE_MX] = {L"Mail-Exchanger (MX)", L"Mail Exchanger (MX)", L"Serveur de messagerie (MX)",
                             L"Server di posta (MX)", L"Servidor de correo (MX)"},
    [STR_LOOKUP_TYPE_NS] = {L"Nameserver (NS)", L"Name Server (NS)", L"Serveur de noms (NS)",
                             L"Server dei nomi (NS)", L"Servidor de nombres (NS)"},
    [STR_LOOKUP_TYPE_TXT] = {L"Text (TXT)", L"Text (TXT)", L"Texte (TXT)", L"Testo (TXT)", L"Texto (TXT)"},
    [STR_LOOKUP_TYPE_CNAME] = {L"Kanonischer Name (CNAME)", L"Canonical Name (CNAME)", L"Nom canonique (CNAME)",
                                L"Nome canonico (CNAME)", L"Nombre canónico (CNAME)"},
    [STR_LOOKUP_TYPE_SOA] = {L"Start of Authority (SOA)", L"Start of Authority (SOA)",
                              L"Start of Authority (SOA)", L"Start of Authority (SOA)",
                              L"Start of Authority (SOA)"},

    [STR_TRACE_LABEL] = {L"Gib eine Netzwerkadresse für Traceroute ein:",
                          L"Enter a network address to traceroute:",
                          L"Saisissez une adresse réseau pour le traceroute :",
                          L"Inserisci un indirizzo di rete per il traceroute:",
                          L"Introduce una dirección de red para el traceroute:"},
    [STR_TRACE_BTN] = {L"Verfolgen", L"Trace", L"Tracer", L"Traccia", L"Rastrear"},
    [STR_TRACE_STOP] = {L"Stopp", L"Stop", L"Arrêter", L"Interrompi", L"Detener"},
    [STR_TRACE_NOADDR] = {L"Bitte gib eine Netzwerkadresse ein.", L"Please enter a network address.",
                           L"Veuillez saisir une adresse réseau.", L"Inserisci un indirizzo di rete.",
                           L"Introduce una dirección de red."},
    [STR_TRACE_NORESOLVE] = {L"Host konnte nicht aufgelöst werden: %s", L"Could not resolve host: %s",
                              L"Impossible de résoudre l'hôte : %s", L"Impossibile risolvere l'host: %s",
                              L"No se pudo resolver el host: %s"},
    [STR_TRACE_NOICMP] = {L"ICMP-Handle konnte nicht erstellt werden.", L"Could not create ICMP handle.",
                           L"Impossible de créer le descripteur ICMP.",
                           L"Impossibile creare l'handle ICMP.", L"No se pudo crear el identificador ICMP."},
    [STR_TRACE_HEADER] = {L"Route zu %s wird ermittelt, maximal 64 Hops, 40-Byte-Pakete:",
                           L"Tracing route to %s, 64 hops max, 40 byte packets:",
                           L"Traçage de la route vers %s, 64 sauts max, paquets de 40 octets :",
                           L"Traccia del percorso verso %s, massimo 64 hop, pacchetti da 40 byte:",
                           L"Rastreando la ruta hacia %s, máximo 64 saltos, paquetes de 40 bytes:"},
    [STR_TRACE_DESTREACHED] = {L"Ziel erreicht.", L"Destination reached.", L"Destination atteinte.",
                                L"Destinazione raggiunta.", L"Destino alcanzado."},

    [STR_WHOIS_LABEL] = {L"Gib die Netzwerkadresse ein, die du per Whois abfragen möchtest:",
                          L"Enter the network address you would like to whois:",
                          L"Saisissez l'adresse réseau que vous souhaitez interroger via whois :",
                          L"Inserisci l'indirizzo di rete da interrogare con whois:",
                          L"Introduce la dirección de red que deseas consultar con whois:"},
    [STR_WHOIS_SERVERLABEL] = {L"Whois-Server:", L"Whois server:", L"Serveur whois :", L"Server whois:",
                                L"Servidor whois:"},
    [STR_WHOIS_BTN] = {L"Whois", L"Whois", L"Whois", L"Whois", L"Whois"},
    [STR_WHOIS_NOADDR] = {L"Bitte gib eine Netzwerkadresse ein.", L"Please enter a network address.",
                           L"Veuillez saisir une adresse réseau.", L"Inserisci un indirizzo di rete.",
                           L"Introduce una dirección de red."},
    [STR_WHOIS_AUTOMATIC] = {L"Automatisch (empfohlen)", L"Automatic (recommended)", L"Automatique (recommandé)",
                              L"Automatico (consigliato)", L"Automático (recomendado)"},
    [STR_WHOIS_QUERYING] = {L"Frage %s ab...", L"Querying %s...", L"Interrogation de %s...",
                             L"Interrogazione di %s...", L"Consultando %s..."},

    [STR_FINGER_LABEL] = {L"Gib user@host ein, um Finger auszuführen (oder nur einen Host, um dessen "
                           L"Benutzer aufzulisten):",
                           L"Enter a user@host to finger (or just a host to list its users):",
                           L"Saisissez un utilisateur@hôte pour finger (ou juste un hôte pour "
                           L"lister ses utilisateurs) :",
                           L"Inserisci un utente@host per finger (o solo un host per elencarne gli utenti):",
                           L"Introduce un usuario@host para finger (o solo un host para listar sus "
                           L"usuarios):"},
    [STR_FINGER_BTN] = {L"Finger", L"Finger", L"Finger", L"Finger", L"Finger"},
    [STR_FINGER_NOADDR] = {L"Bitte gib eine user@host-Adresse ein.", L"Please enter a user@host address.",
                            L"Veuillez saisir une adresse utilisateur@hôte.",
                            L"Inserisci un indirizzo utente@host.", L"Introduce una dirección "
                            L"usuario@host."},
    [STR_FINGER_NOHOST] = {L"Bitte gib einen Host an, z. B. user@example.com.",
                            L"Please include a host, e.g. user@example.com.",
                            L"Veuillez indiquer un hôte, p. ex. user@example.com.",
                            L"Specifica un host, es. user@example.com.",
                            L"Incluye un host, p. ej. user@example.com."},
    [STR_FINGER_CONNECTING] = {L"Verbinde mit %s:79...", L"Connecting to %s:79...", L"Connexion à %s:79...",
                                L"Connessione a %s:79...", L"Conectando a %s:79..."},

    [STR_SCAN_LABEL] = {L"Gib die Netzwerkadresse ein, die du scannen möchtest:",
                         L"Enter the network address you would like to scan:",
                         L"Saisissez l'adresse réseau que vous souhaitez analyser :",
                         L"Inserisci l'indirizzo di rete da scansionare:",
                         L"Introduce la dirección de red que deseas escanear:"},
    [STR_SCAN_BETWEEN] = {L"Nur Ports testen zwischen", L"Only test ports between",
                           L"Tester uniquement les ports entre", L"Testa solo le porte tra",
                           L"Probar solo los puertos entre"},
    [STR_SCAN_AND] = {L"und", L"and", L"et", L"e", L"y"},
    [STR_SCAN_BTN] = {L"Scannen", L"Scan", L"Analyser", L"Scansiona", L"Escanear"},
    [STR_SCAN_STOP] = {L"Stopp", L"Stop", L"Arrêter", L"Interrompi", L"Detener"},
    [STR_SCAN_WARNING] =
        {L"Hinweis: Portscans können in manchen Ländern eingeschränkt oder strafbar sein. "
         L"Scanne nur Systeme, für die du eine Erlaubnis hast.",
         L"Note: Port scanning may be restricted or illegal in some countries. Only scan systems you have "
         L"permission to test.",
         L"Remarque : l'analyse de ports peut être restreinte ou illégale dans certains pays. "
         L"N'analysez que les systèmes pour lesquels vous avez une autorisation.",
         L"Nota: la scansione delle porte potrebbe essere limitata o illegale in alcuni paesi. Esegui la "
         L"scansione solo su sistemi per cui hai l'autorizzazione.",
         L"Nota: el escaneo de puertos puede estar restringido o ser ilegal en algunos países. Escanea "
         L"únicamente sistemas para los que tengas autorización."},
    [STR_SCAN_NOADDR] = {L"Bitte gib eine Netzwerkadresse ein.", L"Please enter a network address.",
                          L"Veuillez saisir une adresse réseau.", L"Inserisci un indirizzo di rete.",
                          L"Introduce una dirección de red."},
    [STR_SCAN_NORESOLVE] = {L"Host konnte nicht aufgelöst werden.", L"Could not resolve host.",
                             L"Impossible de résoudre l'hôte.", L"Impossibile risolvere l'host.",
                             L"No se pudo resolver el host."},
    [STR_SCAN_SCANNING] = {L"Scanne %s, Ports %d-%d...", L"Scanning %s, ports %d-%d...",
                            L"Analyse de %s, ports %d-%d...", L"Scansione di %s, porte %d-%d...",
                            L"Escaneando %s, puertos %d-%d..."},
    [STR_SCAN_OPENPORT] = {L"Offener TCP-Port: %d", L"Open TCP Port: %d", L"Port TCP ouvert : %d",
                            L"Porta TCP aperta: %d", L"Puerto TCP abierto: %d"},
    [STR_SCAN_FINISHED] = {L"Scan abgeschlossen: %d offene(r) Port(s) von %d gescannten gefunden.",
                            L"Scan finished: %d open port(s) found out of %d scanned.",
                            L"Analyse terminée : %d port(s) ouvert(s) trouvé(s) sur %d "
                            L"analysés.",
                            L"Scansione completata: %d porta/e aperta/e trovata/e su %d scansionate.",
                            L"Escaneo finalizado: %d puerto(s) abierto(s) encontrado(s) de %d escaneados."},

    [STR_SPEED_INTRO] = {L"Miss die Latenz, Download- und Upload-Geschwindigkeit deiner Netzwerkverbindung.",
                          L"Measure the latency, download and upload speed of your network connection.",
                          L"Mesurez la latence, la vitesse de téléchargement et d'envoi de votre "
                          L"connexion réseau.",
                          L"Misura la latenza, la velocità di download e upload della tua connessione "
                          L"di rete.",
                          L"Mide la latencia, la velocidad de descarga y subida de tu conexión de red."},
    [STR_SPEED_PING] = {L"Ping", L"Ping", L"Ping", L"Ping", L"Ping"},
    [STR_SPEED_DOWNLOAD] = {L"Download", L"Download", L"Téléchargement", L"Download", L"Descarga"},
    [STR_SPEED_UPLOAD] = {L"Upload", L"Upload", L"Envoi", L"Upload", L"Subida"},
    [STR_SPEED_READY] = {L"Bereit.", L"Ready.", L"Prêt.", L"Pronto.", L"Listo."},
    [STR_SPEED_STARTTEST] = {L"Test starten", L"Start Test", L"Démarrer le test", L"Avvia test",
                              L"Iniciar prueba"},
    [STR_SPEED_STOP] = {L"Stopp", L"Stop", L"Arrêter", L"Interrompi", L"Detener"},
    [STR_SPEED_STARTING] = {L"Wird gestartet...", L"Starting...", L"Démarrage...", L"Avvio in corso...",
                             L"Iniciando..."},
    [STR_SPEED_MEASURING] = {L"Latenz wird gemessen...", L"Measuring latency...", L"Mesure de la latence...",
                              L"Misurazione della latenza...", L"Midiendo la latencia..."},
    [STR_SPEED_TESTDOWN] = {L"Download-Geschwindigkeit wird getestet...", L"Testing download speed...",
                             L"Test de la vitesse de téléchargement...",
                             L"Test della velocità di download...", L"Probando la velocidad de "
                             L"descarga..."},
    [STR_SPEED_TESTUP] = {L"Upload-Geschwindigkeit wird getestet...", L"Testing upload speed...",
                           L"Test de la vitesse d'envoi...", L"Test della velocità di upload...",
                           L"Probando la velocidad de subida..."},
    [STR_SPEED_COMPLETE] = {L"Test abgeschlossen.", L"Test complete.", L"Test terminé.",
                             L"Test completato.", L"Prueba completada."},
    [STR_SPEED_NETERROR] = {L"Netzwerkfehler: %s", L"Network error: %s", L"Erreur réseau : %s",
                             L"Errore di rete: %s", L"Error de red: %s"},
    [STR_SPEED_DASH_MS] = {L"-- ms", L"-- ms", L"-- ms", L"-- ms", L"-- ms"},
    [STR_SPEED_DASH_MBPS] = {L"-- Mbps", L"-- Mbps", L"-- Mbps", L"-- Mbps", L"-- Mbps"},

    [STR_SETTINGS_TITLE] = {L"Einstellungen", L"Settings", L"Paramètres", L"Impostazioni",
                             L"Configuración"},
    [STR_SETTINGS_LANGUAGE] = {L"Sprache", L"Language", L"Langue", L"Lingua", L"Idioma"},
    [STR_SETTINGS_CLOSE] = {L"Schliessen", L"Close", L"Fermer", L"Chiudi", L"Cerrar"},
    [STR_SETTINGS_RESTART_TITLE] = {L"Neustart erforderlich", L"Restart Required", L"Redémarrage requis",
                                     L"Riavvio necessario", L"Reinicio necesario"},
    [STR_SETTINGS_RESTART_BODY] =
        {L"Die neue Sprache wird nach einem Neustart der Anwendung übernommen. Jetzt neu starten?",
         L"The new language will take effect after restarting the application. Restart now?",
         L"La nouvelle langue sera appliquée après le redémarrage de l'application. "
         L"Redémarrer maintenant ?",
         L"La nuova lingua verrà applicata dopo il riavvio dell'applicazione. Riavviare ora?",
         L"El nuevo idioma se aplicará después de reiniciar la aplicación. ¿Reiniciar "
         L"ahora?"},
    [STR_SETTINGS_SYSTEM_LANG] = {L"Systemsprache", L"System language", L"Langue du système",
                                   L"Lingua di sistema", L"Idioma del sistema"},
};

static LanguageId g_currentLanguage = LANG_EN;

typedef struct {
    const wchar_t *code;
    const wchar_t *nativeName;
} LanguageOption;

/* index 0 = "system" (resolved at Init time to one of the 5 below);
 * indices 1..5 map 1:1 to LanguageId. */
static const LanguageOption kOptions[] = {
    {L"system", NULL /* filled from STR_SETTINGS_SYSTEM_LANG at query time */},
    {L"de", L"Deutsch"},
    {L"en", L"English"},
    {L"fr", L"Français"},
    {L"it", L"Italiano"},
    {L"es", L"Español"},
};
#define OPTION_COUNT (int)(sizeof(kOptions) / sizeof(kOptions[0]))

static const wchar_t *kRegistryKeyPath = L"Software\\NetworkTool";
static const wchar_t *kRegistryValueName = L"Language";

void Translations_GetSavedLanguageCode(wchar_t *outCode, int outCodeSize) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegistryKeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0;
        DWORD size = (DWORD)(outCodeSize * sizeof(wchar_t));
        if (RegQueryValueExW(hKey, kRegistryValueName, NULL, &type, (LPBYTE)outCode, &size) == ERROR_SUCCESS &&
            type == REG_SZ) {
            RegCloseKey(hKey);
            return;
        }
        RegCloseKey(hKey);
    }
    wcsncpy(outCode, L"system", outCodeSize - 1);
    outCode[outCodeSize - 1] = L'\0';
}

void Translations_SaveLanguageCode(const wchar_t *code) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegistryKeyPath, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) ==
        ERROR_SUCCESS) {
        RegSetValueExW(hKey, kRegistryValueName, 0, REG_SZ, (const BYTE *)code,
                        (DWORD)((wcslen(code) + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
}

static LanguageId LanguageIdFromCode(const wchar_t *code) {
    if (_wcsicmp(code, L"de") == 0) return LANG_DE;
    if (_wcsicmp(code, L"en") == 0) return LANG_EN;
    if (_wcsicmp(code, L"fr") == 0) return LANG_FR;
    if (_wcsicmp(code, L"it") == 0) return LANG_IT;
    if (_wcsicmp(code, L"es") == 0) return LANG_ES;
    return LANG_EN;
}

/* GetUserDefaultLocaleName() needs Vista+; GetLocaleInfoW's ISO-639 query
 * has worked since Windows 2000, so it's the compatible choice for a
 * ReactOS target. */
static LanguageId ResolveSystemLanguage(void) {
    wchar_t iso639[16];
    if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, iso639, 16) > 0) {
        if (_wcsicmp(iso639, L"de") == 0) return LANG_DE;
        if (_wcsicmp(iso639, L"fr") == 0) return LANG_FR;
        if (_wcsicmp(iso639, L"it") == 0) return LANG_IT;
        if (_wcsicmp(iso639, L"es") == 0) return LANG_ES;
    }
    return LANG_EN;
}

void Translations_Init(void) {
    wchar_t code[16];
    Translations_GetSavedLanguageCode(code, 16);
    if (_wcsicmp(code, L"system") == 0) {
        g_currentLanguage = ResolveSystemLanguage();
    } else {
        g_currentLanguage = LanguageIdFromCode(code);
    }
}

LanguageId Translations_Current(void) {
    return g_currentLanguage;
}

const wchar_t *T(StringId id) {
    const TranslationEntry *entry = &kTranslations[id];
    switch (g_currentLanguage) {
        case LANG_DE: return entry->de;
        case LANG_FR: return entry->fr;
        case LANG_IT: return entry->it;
        case LANG_ES: return entry->es;
        case LANG_EN:
        default: return entry->en;
    }
}

int Translations_OptionCount(void) {
    return OPTION_COUNT;
}

const wchar_t *Translations_OptionNativeName(int index) {
    if (index == 0) {
        return T(STR_SETTINGS_SYSTEM_LANG);
    }
    return kOptions[index].nativeName;
}

const wchar_t *Translations_OptionCode(int index) {
    return kOptions[index].code;
}
