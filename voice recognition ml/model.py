import pandas as pd
import numpy as np
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report

# Audio Data Pre-processing

import librosa.display, os
import matplotlib.pyplot as plt

def create_spectrogram(audio_file, image_file):
    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    fig.subplots_adjust(left=0, right=1, bottom=0, top=1)

    y, sr = librosa.load(audio_file)
    ms = librosa.feature.melspectrogram(y=y, sr=sr)
    log_ms = librosa.power_to_db(ms, ref=np.max)
    librosa.display.specshow(log_ms, sr=sr)

    fig.savefig(image_file)
    plt.close(fig)
    
def create_pngs_from_wavs(input_path, output_path):
    if not os.path.exists(output_path):
        os.makedirs(output_path)

    dir = os.listdir(input_path)

    for i, file in enumerate(dir):
        input_file = os.path.join(input_path, file)
        output_file = os.path.join(output_path, file.replace('.wav', '.png'))
        create_spectrogram(input_file, output_file)

# Labels
classes = ('left','right','forward','reverse','stop') # 5 classes

# CNN Model

batch_size = 64
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

class CNNModel(nn.Module):
    def __init__(self, input_size):
        super(CNNModel, self).__init__()
        self.net = nn.Sequential(
            nn.Conv2d(in_channels = input_size, out_channels = 32, kernel_size=(3, 3)), # 1st conv layer
            nn.ReLU(),
            nn.MaxPool2d(kernel_size=(2, 2), stride=(2, 2)),
            nn.Conv2d(in_channels=32, out_channels=128, kernel_size=(3, 3)), # 2nd conv layer
            nn.ReLU(),
            nn.Conv2d(in_channels=128, out_channels=128, kernel_size=(3, 3)), # 3rd conv layer
            nn.ReLU(),
            nn.MaxPool2d(kernel_size=(2, 2), stride=(2, 2)),
            nn.Conv2d(in_channels=128, out_channels=128, kernel_size=(3, 3)), # 4th conv layer
            nn.ReLU(),
            nn.MaxPool2d(kernel_size=(2, 2), stride=(2, 2)),
            torch.flatten(),
            nn.Linear(in_features=576, out_features=1024),
            nn.ReLU(),
            nn.Dropout(0.1),
            nn.Linear(in_features=1024, out_features=5)
        )
    def forward(self, x):
        return self.net(x)
    
# class SimpleNN(nn.Module):
#     def __init__(self, input_size):
#         super().__init__()

        
#     def forward(self, X):
#         probs = self.net(X)
#         return probs

if __name__ == "__main__":
    # get data
    # split data

    model = CNNModel().to(device)
    num_epochs = 100
    criterion = nn.CrossEntropyLoss()
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1, momentum=0.9)

    # train model
    # test model

    #print(classification_report(y_test, y_pred))

# some points that can be considered: validation set, cross fold validation, using pipelining and grid search