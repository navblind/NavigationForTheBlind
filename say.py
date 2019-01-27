from gtts import gTTS
import time
import os


file = "/tmp/temp.mp3"


while True:
    with open("/tmp/write.txt","r") as f:
        s =  f.read().strip()
    tts = gTTS(s)
    tts.save(file)
    os.system(f"mpg123 {file}")
    time.sleep(1)

