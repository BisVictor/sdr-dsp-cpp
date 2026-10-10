#include <cmath>
#include <iostream>
#include <vector>

// Функция генерации коэффициентов ФНЧ (Аналог fir1 из MATLAB)
std::vector<float> design_lowpass_fir(int num_taps, float sample_rate,
                                      float cutoff_freq) {
  std::vector<float> coeffs(num_taps);

  // Вычисляем нормализованную круговую частоту (omega c)
  float nyquist = sample_rate / 2.0f;
  float norm_cutoff = cutoff_freq / nyquist;
  float omega_c = M_PI * norm_cutoff;  // M_PI — это число Пи из <cmath>

  int center =
      (num_taps - 1) / 2;  // Центральный индекс фильтра (для симметрии)
  float sum = 0.0f;        // Переменная для последующей нормализации усиления

  for (int i = 0; i < num_taps; i++) {
    int n = i - center;

    // 1. Расчет идеального ФНЧ (функция sinc)
    if (n == 0) {
      // Защита от деления на ноль в самом центре фильтра
      coeffs[i] = norm_cutoff;
    } else {
      coeffs[i] = std::sin(omega_c * n) / (M_PI * n);
    }

    // 2. Наложение сглаживающего Окна Хемминга (Hamming Window)
    // Формула окна Хемминга: w(n) = 0.54 + 0.46 * cos(2*pi*n / (M-1))
    float hamming_window =
        0.54f + 0.46f * std::cos((2.0f * M_PI * n) / (num_taps - 1));

    // Умножаем идеальный фильтр на окно
    coeffs[i] *= hamming_window;

    // Копим сумму всех коэффициентов, чтобы нормировать усиление к 1.0 (0 дБ)
    sum += coeffs[i];
  }

  // 3. Нормализация: делим каждый коэффициент на общую сумму.
  // Если этого не сделать, фильтр будет искажать общую громкость (амплитуду)
  // сигнала.
  for (int i = 0; i < num_taps; i++) {
    coeffs[i] /= sum;
  }

  return coeffs;
}

int main() {
  // Рассчитываем те же 401 коэффициент для ACARS прямо в памяти!
  std::vector<float> my_coeffs = design_lowpass_fir(401, 1024000.0f, 7500.0f);

  std::cout << "Сгенерировано " << my_coeffs.size() << " коэффициентов в C++."
            << std::endl;
  for (size_t i = 0; i < my_coeffs.size(); i++) {
    std::cout << my_coeffs[i] << std::endl;
  }

  return 0;
}