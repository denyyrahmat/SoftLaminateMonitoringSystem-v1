import paho.mqtt.client as mqtt

client = mqtt.Client()
client.connect("localhost", 1883, 60) 

topic_list = ["sensor/rollTemp", "sensor/dryZoneTemp", "sensor/roll1Dist", "sensor/roll2Dist", "sensor/roll3Dist"]

while True:
  for topics in topic_list:
    # payload = 