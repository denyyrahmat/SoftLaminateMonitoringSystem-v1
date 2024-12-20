import paho.mqtt.client as mqtt
import json
import time

# Konfigurasi MQTT
client = mqtt.Client()
client.connect("localhost", 1883, 60)

# Kirim data sensor
while True:
    data = {
        "rollTemp" : 25,
        "dryZoneTemp" : 30,
        "roll1Dist" : 0.5,
        "roll2Dist" : 0.5,
        "roll3Dist" : 0.5,
    }
    client.publish("iot/sensor", json.dumps(data))  # Kirim data dalam format JSON
    print("Data terkirim:", data)
    time.sleep(5)  # Kirim data setiap 5 detik
