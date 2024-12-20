import paho.mqtt.client as mqtt
import mysql.connector
import json

# Konfigurasi database MySQL
db = mysql.connector.connect(
    host="localhost",
    user="root",      # Ganti dengan username MySQL Anda
    password="",      # Ganti dengan password MySQL Anda
    database="data_sensor_test"
)
cursor = db.cursor()

# Callback saat terhubung ke broker
def on_connect(client, userdata, flags, rc):
    print("Terhubung ke broker MQTT")
    client.subscribe("iot/sensor")  # Topik yang ingin disubscribe

# Callback saat pesan diterima
def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())  # Data dalam format JSON
        rollTemp = data["rollTemp"]
        dryZoneTemp = data["dryZoneTemp"]
        roll1Dist = data["roll1Dist"]
        roll2Dist = data["roll2Dist"]
        roll3Dist = data["roll3Dist"]

        # Simpan data ke database
        query = "INSERT INTO data_sensor (rollTemp, dryZoneTemp, roll1Dist, roll2Dist, roll3Dist) VALUES (%s, %s, %s, %s, %s)"
        cursor.execute(query, (rollTemp, dryZoneTemp, roll1Dist, roll2Dist, roll3Dist))
        db.commit()

        print(f"Data disimpan: {rollTemp} - {dryZoneTemp} - {roll1Dist} - {roll2Dist} - {roll3Dist}")
    except Exception as e:
        print(f"Error: {e}")

# Konfigurasi MQTT
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)  # Koneksi ke broker lokal
client.loop_forever()