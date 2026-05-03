# Voice-Recognition-Project

A Convolutional Neural Network trained on 30000 samples of a one second audio of someone saying a one digit number. A MCU was used to grab the audio sample from a user and send it to the CNN where a prediction is made.

This project is a hardware to software voice recognition system. It samples audio live from the microcontroller. The microcontroller performs digital signal processing to convert the audio format, sends it to a PC via the COM port, and uses a custom trained CNN to classify spoken digits (0-9) in real time with a 95% accuracy.

## **SYSTEM ARCHITECTURE**

- **Microcontroller (C)**: Captures PDM audio, filters it to PCM, and transmits it.
- **Model Training (Pytorch)**: Processes audio datasets into Specotrgrams and trains a 2D CNN.
- **Live Inference (Python)**: Listens to the serial port, generates a spectrogram of the live audio, and predicts the spoken word.

## **CORRESPONDING FILES FOR EACH PART OF THE ARCHITECTURE**
- **Microcontroller (C)**: you can find this under file **Voice_Recognition**. To find the .c and .h files (bulk of code):
  - **main.c**: Voice_Recognition->Core->Src->main.c
  - **main.h**: Voice_Recognition->Core->Inc->main.h

### **Hardware and Data Acquisition (MCU)**

Written in C for the STM32F4. Handles strict real time constraints that come from audio processing.

- **Double-Buffering DMA**: Audio is sampled for a total of one second using a double-buffering technique via the Direct Memory Access (DMA). While one buffer is actively filling with data from the microphone using the Serial Peripheral Interface, the CPU converts the other buffer to pcm format.
- **PDM to PCM Conversion**: The microphone outputs Pulse Density Modulation signals (PDM). These signals are not compatible with a machine learning model thus a conversion is necessary. The data is processed through a third order Cascaded Integrator-Comb (CIC) Filter with a decimation factor of 32 to decrease the frequency of the sampled audio. The audio is being sampled at a rate of 1Mhz, so a decimation factor of 32 decreases it down to 31.25KHz. 
- **Hardware Filtering**: An IIR High pass filter removes the DC offset, and a simple exponential smoothing low-pass filter removes high-frequency jitter.
- **Transmission**: Once a full 1 second sample of audio is captured and processed, it is shipped to the host PC via Universal Asynchronous Receiver/Transmitter (UART) DMA. The UART is a peripheral in the STM32 used for serial communication.

### ** Neural Network Training (Pytorch) **

A custom CNN was designed and trained from scratch to recognize spoken digits (1-9).

- **Dataset Pipeline**: .wav audio files are preprocessed using librosa. They are truncuated to a fixed length (30,800 samples at 31.25Khz) so that the data the CNN is trained on handles are audio data fine. These are then converted to 64 band Mel-Spectrogram. These are saved as a PyTorch .pt tensor files to make loading data faster during training.
- **Model Architecture**: 
  - *Conv2D* layers with dilation and grouped convolutions to reduce computational cost.
  - *BatchNorm2d* and *LeakyReLU* activations for stability.
  - *MaxPool2d* and *AdaptiveAvgPool2d* for spatial downsampling
  - *Dropout* to prevent overfitting
- **Performance**: Trained using the *Adam optimizer* and *CrossEntropyLoss*, the model achieves an accuracy of 95%

### **Live Inference (Python)**

A python script acting as the bridge between the hardware (MCU) and the CNN. Outputs the real time prediction of what the user said.

- **Serial Communication**: The script reads the incoming PCM bytes from the MCU over the COM port.
- **Filtering**: It applies a Butterworth high-pass filter (60Hz cutoff) to clean up any remaining low frequency noise.
- **Feature Extraction**: The raw audio is transformed into a Mel-Spectrogram, exactly matching the format the CNN expects.
- **Prediction**: The spectrogram tensor is fed into the loaded .pth PyTorch model, which outputs the predicted class (spoken digit).
















