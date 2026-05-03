import serial
import numpy as np
import librosa
import matplotlib.pylab as plt
import torch
import torch.nn as nn
from scipy.signal import butter, lfilter

ser = serial.Serial('COM3', 115200, timeout=10)

print("Press the button now...")

# Read until we get a full recording (700 samples * 2 bytes * 44 buffers)
expected_bytes = 700 * 2 * 44
data = ser.read(expected_bytes)

# print(f"Received {len(data)} bytes")
print(f"Expected {expected_bytes}, got {len(data)}")  

samples = np.frombuffer(data, dtype='<i2')  # little-endian int16
print(f"Got {len(samples)} samples")

# After reading data, before converting
data = ser.read(expected_bytes)
samples = np.frombuffer(data, dtype='<i2')

# Check the actual bit density - should be ~50% for silence
print(f"Mean value: {samples.mean():.1f} (should be near 0 for silence)")
print(f"Std dev: {samples.std():.1f}")
print(f"Min: {samples.min()}, Max: {samples.max()}")

# Plot raw waveform
plt.figure(figsize=(12, 4))
plt.subplot(1, 2, 1)
plt.plot(samples[:1000])
plt.title("Raw PCM samples")
plt.subplot(1, 2, 2)
plt.hist(samples, bins=50)
plt.title("Sample distribution")
plt.tight_layout()
plt.show()


# Save to file for further analysis
np.save('audio.npy', samples)
ser.close()

# print(f"Min: {samples.min()}, Max: {samples.max()}, Std: {samples.std():.1f}")
# plt.plot(samples[:1000])
# plt.title("Raw waveform")
# plt.show()

#THIS

def butter_highpass_filter(data, cutoff, fs, order=4):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='high', analog=False)
    y = lfilter(b, a, data)
    return y

def process_mic_data(raw_buffer):
    # audio_data = np.frombuffer(raw_buffer, dtype = np.int16)
    audio_data = raw_buffer.astype(np.float32)/32768.0
    audio_data = butter_highpass_filter(audio_data, cutoff=60, fs=31250)
    spectrogram = librosa.feature.melspectrogram(y=audio_data, sr=31250, n_mels=64)
    return librosa.power_to_db(spectrogram, ref=np.max, top_db=80)

spectrogram = process_mic_data(samples)
spectrogram = spectrogram[:64, :]

plt.figure(figsize=(10, 4))
img = librosa.display.specshow(spectrogram, sr=31250, x_axis='time', y_axis='mel', fmax=8000)

plt.colorbar(img, format='%+2.0f dB')
plt.title('Mel-Spectrogram of Mic Input')
plt.tight_layout()

plt.show()


class CNN(nn.Module):
    def __init__(self):
        super(CNN, self).__init__()
        # self.conv1 = nn.Conv1d(in_channels=1,out_channels=16,kernel_size=3,padding=1)
        # self.conv2 = nn.Conv1d(in_channels=16,out_channels=32,kernel_size=3,padding=1)
        # self.conv3 = nn.Conv1d(in_channels=32,out_channels=64,kernel_size=3,padding=1)
        # self.pool = nn.MaxPool1d(kernel_size=3, stride=2)
        # self.gap = nn.AdaptiveAvgPool1d(1)
        # self.fc1 = nn.Linear(in_features=64*1,out_features=128) #use formula Wout = (Win-K+2P)/S + 1
        # self.fc2 = nn.Linear(in_features=128, out_features=10)
        # self.relu = nn.ReLU()
        # self.dropout = nn.Dropout(0.25)
        self.layer1 = nn.Sequential(
            nn.Conv2d(in_channels=1,out_channels=16,kernel_size=3,stride = 1, padding=1, dilation=2), #30800 --> 7700
            nn.BatchNorm2d(16),
            nn.LeakyReLU(),
            nn.MaxPool2d(kernel_size=2, stride=2),
            nn.Dropout(0.1)
        )
        self.layer2 = nn.Sequential(
            nn.Conv2d(in_channels=16,out_channels=16,kernel_size=3,padding=1, groups=16),
            nn.Conv2d(in_channels=16,out_channels=32,kernel_size=3),
            nn.BatchNorm2d(32),
            nn.LeakyReLU(),
            nn.MaxPool2d(kernel_size=2, stride=2),
            nn.Dropout(0.1)
        )
        self.layer3 = nn.Sequential(
            nn.Conv2d(in_channels=32,out_channels=32,kernel_size=3,padding=1, groups=32),
            nn.Conv2d(in_channels=32,out_channels=64,kernel_size=1),
            nn.BatchNorm2d(64),
            nn.LeakyReLU(),
            nn.AdaptiveAvgPool2d(1)
        )
        self.classifier = nn.Sequential(
            nn.Flatten(),
            nn.Linear(64, 32),
            nn.LeakyReLU(),
            nn.Dropout(0.25),
            nn.Linear(32, 9)
        )

    def forward(self, x):    
        # x = x.unsqueeze(1)
        x = self.layer1(x)
        x = self.layer2(x)
        x = self.layer3(x)
        x = self.classifier(x)
        return x


# class CNN(nn.Module):
#     def __init__(self):
#         super(CNN, self).__init__()
#         self.layer1 = nn.Sequential(
#             nn.Conv2d(in_channels=1,out_channels=16,kernel_size=3,stride = 1, padding=1, dilation=2), #30800 --> 7700
#             nn.BatchNorm2d(16),
#             nn.LeakyReLU(),
#             nn.MaxPool2d(kernel_size=2, stride=2),
#             nn.Dropout(0.1)
#         )
#         self.layer2 = nn.Sequential(
#             nn.Conv2d(in_channels=16,out_channels=32,kernel_size=3,padding=1, groups=4, dilation=2),
#             nn.BatchNorm2d(32),
#             nn.LeakyReLU(),
#             nn.MaxPool2d(kernel_size=2, stride=2),
#             nn.Dropout(0.3)
#         )
#         self.layer3 = nn.Sequential(
#             nn.Conv2d(in_channels=32,out_channels=64,kernel_size=3,padding=1, groups=4, dilation=2),
#             nn.BatchNorm2d(64),
#             nn.LeakyReLU(),
#             nn.AdaptiveAvgPool2d(1)
#         )
#         self.classifier = nn.Sequential(
#             nn.Flatten(),
#             nn.Linear(64, 32),
#             nn.LeakyReLU(),
#             nn.Dropout(0.5),
#             nn.Linear(32, 9)
#         )

#     def forward(self, x):    
#         # x = x.unsqueeze(1)
#         x = self.layer1(x)
#         x = self.layer2(x)
#         x = self.layer3(x)
#         x = self.classifier(x)
#         return x

model = CNN()


model.load_state_dict(torch.load("VoiceRecognition_2.pth", weights_only=True))
model.eval()


spec_tensor = torch.tensor(spectrogram, dtype=torch.float32).unsqueeze(0).unsqueeze(0)
print(spec_tensor.shape)

with torch.no_grad():
    output = model(spec_tensor)



predicted_class = torch.argmax(output, dim=1).item()
print(f"Predicted class: {predicted_class+1}")

