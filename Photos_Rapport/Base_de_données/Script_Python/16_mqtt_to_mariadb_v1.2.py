import paho.mqtt.client as mqtt
import mysql.connector
import sys

# ------------------------------
# Paramètres de connexion à MariaDB
# ------------------------------
db_config = {
    "host": "localhost",      # Adresse du serveur MariaDB
    "user": "user-db",        # Nom d'utilisateur MariaDB
    "password": "***********",  # Mot de passe de l'utilisateur
    "database": "capteurs_temp" # Nom de la base de données
}

# ------------------------------
# Fonction de connexion à MariaDB
# ------------------------------
def connect_db():
    try:
        conn = mysql.connector.connect(**db_config)  # Connexion à la base de données
        return conn
    except mysql.connector.Error as e:
        print(f"Erreur lors de la connexion à MariaDB : {e}")
        sys.exit(1)  # Quitte le script en cas d'erreur critique

# ------------------------------
# Fonction pour vérifier si une chaîne peut être convertie en float
# ------------------------------
def is_float(value):
    try:
        float(value)
        return True
    except ValueError:
        return False

# ------------------------------
# Callback appelé lorsqu'un message MQTT est reçu
# ------------------------------
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8")  # Décodage du message reçu
        data = payload.split(",")  # Séparation des valeurs par virgules (si plusieurs valeurs sont envoyées)

        # Vérifie que la première valeur est un nombre flottant
        if len(data) >= 1 and is_float(data[0]):
            temperature = float(data[0])  # Convertit la valeur en float

            # Connexion à la base de données et insertion de la température
            conn = connect_db()
            cursor = conn.cursor()
            cursor.execute("INSERT INTO mesures_temperatures (temperature) VALUES (%s)", (temperature,))
            conn.commit()

            # Fermeture de la connexion
            cursor.close()
            conn.close()
            print(f"Donnée insérée : {temperature}°C")
    except Exception as e:
        print(f"Erreur lors de l'insertion : {e}")

# ------------------------------
# Configuration du client MQTT
# ------------------------------
mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message  # Associe la fonction on_message aux messages reçus

# Connexion au broker MQTT (Mosquitto) et abonnement au topic
mqtt_client.connect("localhost", 1883, 60)  # Se connecte au broker sur localhost, port 1883
mqtt_client.subscribe("temp")  # S'abonne au topic "temp" pour recevoir les données du capteur

# ------------------------------
# Boucle infinie pour écouter les messages MQTT
# ------------------------------
mqtt_client.loop_forever()  # Execute la boucle MQTT en continue
