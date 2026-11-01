#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

// Структура для хранения параметров симулятора
struct AcarsConfig {
  double sampleRate = 48000.0;  // Частота дискретизации (f_s)
  double baudRate = 2400.0;     // Скорость ACARS (2400 бод)
  double freqMark = 1200.0;     // Логическая 1
  double freqSpace = 2400.0;    // Логический 0
};

// Функция генерации одного бита (MSK / AFSK модуляция)
std::vector<double> generateBit(bool bit, double& currentPhase,
                                const AcarsConfig& cfg);

// Генерация целого пакета ACARS из вектора бит
std::vector<double> generateAcarsPacket(const std::vector<bool>& bits,
                                        const AcarsConfig& cfg);

template <typename T>
void saveSignal(const std::vector<T>& signal, std::string filename) {
  std::ofstream file(filename);

  for (auto sample : signal) {
    file << std::abs(sample) << '\n';
  }
}
