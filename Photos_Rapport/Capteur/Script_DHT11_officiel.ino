#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>         // Inclure les bibliothèques 

// Configuration du DHT11
#define DHTPIN 3                  // Définition de la broche de connexion du capteur DHT11
#define DHTTYPE DHT11             // Définition du type de capteur (DHT11)
DHT dht(DHTPIN, DHTTYPE);         // Initialisation du capteur DHT11

// Configuration du réseau Ethernet
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x6F, 0x4D }; // Adresse MAC unique pour l’Ethernet Shield
IPAddress ip(10, 129, 251, 1);                       // Adresse IP de l’Arduino
EthernetServer server(1904);                // Serveur web sur le port 1904

// Configuration du client Ethernet
EthernetClient ethernetClient; // Client Ethernet pour se connecter à MQTT

// Configuration du client MQTT
IPAddress mqttServer(10, 129, 251, 253); // Adresse IP du serveur MQTT
const int mqttPort = 1883;               // Port par défaut du serveur MQTT
PubSubClient clientMQTT(ethernetClient); // Initialisation du client MQTT avec EthernetClient

// Dernière température mesurée
float lastTemperature = 0.0;

// Variables pour contrôler la fréquence des mises à jour MQTT
unsigned long lastMQTTSendTime = 0;  // Temps de la dernière publication MQTT
const unsigned long mqttInterval = 5000; // Intervalle entre chaque publication (5 secondes)

void setup() {
  Serial.begin(9600);            // Initialisation de la communication série pour le débogage
  Serial.println("Initialisation...");
  dht.begin();                   // Initialisation du capteur DHT11
  Serial.println("Capteur DHT initialisé.");

  // Initialisation de l’Ethernet Shield avec l'adresse MAC et l'adresse IP
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Serveur web démarré sur IP : ");    // Afficher l'adresse IP de l'Arduino
  Serial.println(Ethernet.localIP());   // Affichage de l'adresse IP du serveur dans le moniteur série

  // Configurer le client MQTT
  clientMQTT.setServer(mqttServer, mqttPort);
}

void loop() {
  // Assurez-vous que la connexion MQTT est active
  if (!clientMQTT.connected()) {
    reconnectMQTT(); // Reconnexion au serveur MQTT si nécessaire
  }
  clientMQTT.loop(); // Maintenir la connexion avec le serveur MQTT

  // Vérifiez si le temps d'envoyer les données au MQTT est atteint
  if (millis() - lastMQTTSendTime > mqttInterval) {
    sendTemperatureToMQTT(); // Envoyer la température au serveur MQTT
    lastMQTTSendTime = millis();
  }
  handleWebServer();
}// Génération d'un identifiant unique pour le client MQTT, L'identifiant est important pour que le serveur MQTT puisse distinguer les clients

// Fonction pour envoyer la température au serveur MQTT
void sendTemperatureToMQTT() {
  // Lecture de la température depuis le capteur DHT11
  float temp = dht.readTemperature();  // Récupère la température mesurée

  if (!isnan(temp)) {  // Vérifie que la température est une valeur valide
    lastTemperature = temp;  // Stocke la dernière température valide

    // Convertir la température en chaîne de caractères pour l'envoi MQTT
    char tempStr[8];  // Tableau pour stocker la chaîne formatée
    dtostrf(temp, 2, 2, tempStr);  // Convertit le float en string (1 chiffre avant la virgule, 2 après)

    // Publier la température sur le topic "temp" du serveur MQTT
    if (clientMQTT.publish("temp", tempStr)) {  // Envoie la température
      Serial.println("Température de la salle serveur envoyée à MQTT : " + String(temp) + " °C"); 
    } else {
      Serial.println("Erreur lors de l'envoi de la température.");  // Affiche une erreur si l'envoi échoue
    }
  } else {
    Serial.println("Échec de la lecture du capteur DHT11 !");  // Affiche une erreur si la lecture est invalide
  }
}

// Fonction pour reconnecter le client MQTT si déconnecté
void reconnectMQTT() {
  while (!clientMQTT.connected()) {  // Boucle tant que le client MQTT n’est pas connecté
    Serial.print("Tentative de connexion MQTT...");  // Affiche un message de tentative

    String clientId = "arduinoClient-";  // Préfixe pour l’ID client MQTT
    clientId += String(random(0xffff), HEX);  // Ajoute une partie aléatoire pour éviter les conflits d’ID

    if (clientMQTT.connect(clientId.c_str())) {  // Tente de se connecter avec l’ID généré
      Serial.println("Connecté au serveur MQTT.");  // Affiche un message si la connexion réussit
    } else {
      Serial.print("Échec de la connexion, état : ");  // Affiche une erreur si la connexion échoue
      Serial.println(clientMQTT.state());  // Affiche le code d’état de la tentative
      delay(5000);  // Attend 5 secondes avant de réessayer
    }
  }
}

// Fonction pour gérer les requêtes HTTP et afficher une interface web
void handleWebServer() {
  // Vérifier si un client est connecté
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Client connecté !");
    
      // Envoi de l'entête HTTP 200 OK pour indiquer que la connexion a réussi
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html; charset=UTF-8");   // Définition du type de contenu HTML
      client.println("Connection: close");         // Indiquer que la connexion se ferme après la réponse
      client.println();

      // Génération du contenu HTML affiché sur la page web
      client.println("<html>");
      client.println("<head>");
      client.println("<title>Données du capteur DHT11</title>");
      client.println("<style>");
      client.println("body { background-color: #191998; font-family: Arial, sans-serif; text-align: center; padding: 50px; }");
      client.println("h1 { font-size: 45px; color: #FFFFFF; }");
      client.println("p { font-size: 40px; color: #FFFFFF; }");
      client.println("</style>");
      client.println("</head>");
      client.println("<body>");
      client.println("<h1>Données du capteur DHT11</h1>");
      client.print("<p>Il fait actuellement ");
      client.print(lastTemperature);  // Utilisation de la variable globale lastTemperature
      client.println(" °C dans la salle serveur du lycée Louis COUFFIGNAL</p>");
      client.println("</body>");
      client.println("</html>");
    
    delay(1);                  // Pause pour s'assurer que le client a bien reçu les données
    client.stop();             // Déconnexion du client
    Serial.println("Client déconnecté.");
  }

  delay(1000); // Attente de 1 seconde avant la prochaine lecture du capteur
}