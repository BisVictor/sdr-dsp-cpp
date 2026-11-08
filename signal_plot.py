import numpy as np
import matplotlib.pyplot as plt

signal = np.loadtxt("tests/signal_data/test8_after.csv")
# signal = signal[251_000:258_000]

sample_rate = 32000
num_points = len(signal)
duration = num_points / sample_rate
time = np.arange(num_points) / sample_rate

# Создаем фигуру и объект осей ax (нужен для точного позиционирования рамки)
fig, ax = plt.subplots(figsize=(16, 5))

# Строим график 
ax.plot(
    time,
    signal,
    linewidth=0.7,
    marker='o',
    markersize=3,
    markerfacecolor='blue',
    markeredgecolor='blue'
)

ax.set_title("Test8_design_bandpass_fir")
#ax.set_xlabel("Time (seconds)")
ax.set_ylabel("Amplitude")
ax.set_ylim(-1.1, 1.1)
ax.grid(True)

# --- ФОРМИРУЕМ КВАДРАТ С ДАННЫМИ ---
# Текст внутри блока
info_text = (
    f"Sample Rate: {sample_rate / 1e3:.1f} kHz\n"
    f"Total Points: {num_points:,}\n"
    f"Duration: {duration * 1000:.2f} ms\n"
)

# Настройки внешнего вида самой рамки
box_properties = dict(
    boxstyle='round,pad=0.5',  # скругленные углы и внутренний отступ
    facecolor='white',         # цвет фона
    edgecolor='gray',          # цвет рамки
    alpha=0.85                 # прозрачность, чтобы сетка слегка просвечивала
)

# Добавляем текст на график
# (0.98, 0.95) — это правый верхний угол в относительных координатах от 0 до 1
ax.text(
    0.02, 0.05,                # Координаты левого нижнего угла (с небольшим отступом от краев)
    info_text, 
    transform=ax.transAxes, 
    fontsize=10,
    fontfamily='monospace',    
    verticalalignment='bottom',  # Выравнивание по нижней границе рамки
    horizontalalignment='left',  # Выравнивание по левой границе рамки
    bbox=box_properties
)
# -----------------------------------

plt.savefig("tests/test_plots/plot_test8_after", bbox_inches='tight')