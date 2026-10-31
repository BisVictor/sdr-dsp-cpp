import numpy as np
import matplotlib.pyplot as plt

signal = np.loadtxt("test_env.csv")

signal = signal[251_000:258_000]

sample_rate = 48000
time = np.arange(len(signal)) / sample_rate

plt.figure(figsize=(32, 5))

plt.plot(time, signal, linewidth=0.7)

plt.title("ACARS Modulated Signal")
plt.xlabel("Time (seconds)")
plt.ylabel("Amplitude")

plt.ylim(-1.1, 1.1)
plt.grid(True)

plt.plot(
    time,
    signal,
    linewidth=0.7,
    marker='o',
    markersize=3,
    markerfacecolor='blue',
    markeredgecolor='blue'
)

plt.savefig("plot.png")