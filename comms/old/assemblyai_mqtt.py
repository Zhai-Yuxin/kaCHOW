# mosquitto -c /etc/mosquitto/mosquitto.conf to begin to MQTT server

import paho.mqtt.client as mqtt
import requests


upload_endpoint = 'https://api.assemblyai.com/v2/upload'
transcript_endpoint = 'https://api.assemblyai.com/v2/transcript'

headers_auth_only = {'authorization': '62ad59fcaf7e46abb854282d715b98d3'}

headers = {
    "authorization": '62ad59fcaf7e46abb854282d715b98d3',
    "content-type": "application/json"
}

CHUNK_SIZE = 5_242_880  # 5MB

audio_data = bytearray()

def save_wav_file(file_path, audio_data):
    with open(file_path, 'wb') as wav_file:
        wav_file.write(audio_data)
    print(f"Audio saved to {file_path}")

def upload(filename):
    def read_file(filename):
        with open(filename, 'rb') as f:
            while True:
                data = f.read(CHUNK_SIZE)
                if not data:
                    break
                yield data

    upload_response = requests.post(upload_endpoint, headers=headers_auth_only, data=read_file(filename))
    return upload_response.json()['upload_url']


def transcribe(audio_url):
    transcript_request = {
        'audio_url': audio_url
    }

    transcript_response = requests.post(transcript_endpoint, json=transcript_request, headers=headers)
    return transcript_response.json()['id']

        
def poll(transcript_id):
    polling_endpoint = transcript_endpoint + '/' + transcript_id
    polling_response = requests.get(polling_endpoint, headers=headers)
    return polling_response.json()


def get_transcription_result_url(url):
    transcribe_id = transcribe(url)
    while True:
        data = poll(transcribe_id)
        if data['status'] == 'completed':
            return data['text']

def process():
    client.loop_stop()
    # Save the received audio data to a file
    save_wav_file('Sounds/recording.wav', audio_data)
    filename = "Sounds/recording.wav"
    audio_url = upload(filename)
    print(get_transcription_result_url(audio_url))
    audio_data.clear()
    client.loop()

def on_connect(client, userdata, flags, rc):
	print("Connected with result code: " + str(rc))
	client.subscribe("voice/#")

def on_message(client, userdata, message):
    topic = message.topic
    global audio_data
    # Append the received payload (audio chunk) to the buffer
    audio_data.extend(message.payload)
    print(f'message: ', message)
    if topic == "voice/stop":
        print(message.payload.decode('utf-8'))
        process()

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.loop_forever()
