# Labels

classes = ('backward', 'go', 'left', 'right', 'stop') # 5 classes

# Load Images

from keras.preprocessing import image
import os

def load_images_from_path(path, label):
    images = []
    labels = []

    for file in os.listdir(path):
        images.append(image.img_to_array(image.load_img(os.path.join(path, file), target_size=(224, 224, 3))))
        labels.append((label))
        
    return images, labels
        
x = []
y = []
index = 0

for label in classes:
    images, labels = load_images_from_path(f'Spectrograms/{label}', index)
    x += images
    y += labels
    index += 1  

# Train-test split

import numpy as np
import tensorflow as tf
from tensorflow.keras.utils import to_categorical
from sklearn.model_selection import train_test_split

x_train, x_test, y_train, y_test = train_test_split(x, y, stratify=y, test_size=0.3, random_state=0)

x_train_norm = np.array(x_train) / 255
x_test_norm = np.array(x_test) / 255

y_train_encoded = to_categorical(y_train)
y_test_encoded = to_categorical(y_test)

# Transfer learning

from keras.models import Sequential
from keras.layers import Conv2D, MaxPooling2D
from keras.layers import Flatten, Dense
import keras

# CNN

class CNN():
    def __init__(self):
        self.model = Sequential([
            Conv2D(32, (3, 3), activation='relu', input_shape=(224, 224, 3)),
            MaxPooling2D(2, 2),
            Conv2D(128, (3, 3), activation='relu'),
            MaxPooling2D(2, 2),
            Conv2D(128, (3, 3), activation='relu'),
            MaxPooling2D(2, 2),
            Conv2D(128, (3, 3), activation='relu'),
            MaxPooling2D(2, 2),
            Flatten(),
            Dense(1024, activation='relu'),
            Dense(5, activation='log_softmax')
        ])
    
    def forward(self):
        optimizer = keras.optimizers.SGD(learning_rate=0.1, momentum=0.9)
        self.model.compile(optimizer=optimizer, loss='categorical_crossentropy', metrics=['accuracy'])
        return self.model

with tf.device('GPU/1'):
    model = CNN().forward()
    hist = model.fit(x_train_norm, y_train_encoded, validation_data=(x_test_norm, y_test_encoded), batch_size=64, epochs=1)

# Save model

model.save('model3.keras')