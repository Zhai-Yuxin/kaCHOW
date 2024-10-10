# Labels
classes = ('left','right','forward','reverse','stop') # 5 classes

# Load Images

from keras.preprocessing import image

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

from tensorflow.keras.utils import to_categorical
from sklearn.model_selection import train_test_split

x_train, x_test, y_train, y_test = train_test_split(x, y, stratify=y, test_size=0.3, random_state=0)

x_train_norm = np.array(x_train) / 255
x_test_norm = np.array(x_test) / 255

y_train_encoded = to_categorical(y_train)
y_test_encoded = to_categorical(y_test)

# CNN

from keras.models import Sequential
from keras.layers import Conv2D, MaxPooling2D
from keras.layers import Flatten, Dense

class CNNModel():
    def forward(self):
        model = Sequential()
        model.add(Conv2D(32, (3, 3), activation='relu', input_shape=(224, 224, 3)))  # 1st conv layer
        model.add(MaxPooling2D(2, 2))
        model.add(Conv2D(128, (3, 3), activation='relu')) # 2nd conv layer
        model.add(MaxPooling2D(2, 2))
        model.add(Conv2D(128, (3, 3), activation='relu')) # 3rd conv layer
        model.add(MaxPooling2D(2, 2))
        model.add(Conv2D(128, (3, 3), activation='relu')) # 4th conv layer
        model.add(MaxPooling2D(2, 2))
        model.add(Flatten())
        model.add(Dense(1024, activation='relu'))
        model.add(Dense(4, activation='softmax'))
        model.compile(optimizer='adam', loss='categorical_crossentropy', metrics=['accuracy'])
        return model

# Transfer learning

from tensorflow.keras.applications import MobileNetV2
from tensorflow.keras.applications.mobilenet import preprocess_input

base_model = MobileNetV2(weights='imagenet', include_top=False, input_shape=(224, 224, 3))

x_train_norm = preprocess_input(np.array(x_train))
x_test_norm = preprocess_input(np.array(x_test))

train_features = base_model.predict(x_train_norm)
test_features = base_model.predict(x_test_norm)

class MLP():
    def forward(self):
        model = Sequential()
        model.add(Flatten(input_shape=train_features.shape[1:]))
        model.add(Dense(1024, activation='relu'))
        model.add(Dense(4, activation='softmax'))
        model.compile(optimizer='adam', loss='categorical_crossentropy', metrics=['accuracy'])
        return model
    
# Test model on given sample

from numpy import np

base_model = MobileNetV2(weights='imagenet', include_top=False, input_shape=(224, 224, 3))

x = image.load_img('Spectrograms/sample1.png', target_size=(224, 224))
x = image.img_to_array(x)
x = np.expand_dims(x, axis=0)
x = preprocess_input(x)

y = base_model.predict(x)
predictions = MLP.forward() \
.fit(x_train_norm, y_train_encoded, validation_data=(x_test_norm, y_test_encoded), batch_size=10, epochs=10) \
.predict(y)

for i, label in enumerate(classes):
    print(f'{label}: {predictions[0][i]}')


for i, label in enumerate(classes):
    if predictions([0][i]) == max(predictions([0])):
        print(label)