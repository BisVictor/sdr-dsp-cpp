#include "acars_modulator.h"

// Функция генерации одного бита (MSK / AFSK модуляция)
std::vector<double> generateBit(bool bit, double& currentPhase,
                                const AcarsConfig& cfg) {
  int samplesPerBit = static_cast<int>(cfg.sampleRate / cfg.baudRate);
  std::vector<double> signal(samplesPerBit);

  // Выбираем частоту в зависимости от бита
  double freq = bit ? cfg.freqMark : cfg.freqSpace;
  double phaseStep = 2.0 * M_PI * freq / cfg.sampleRate;

  for (int i = 0; i < samplesPerBit; ++i) {
    signal[i] = std::sin(currentPhase);
    currentPhase += phaseStep;

    // Ограничиваем фазу для точности
    if (currentPhase > 2.0 * M_PI) {
      currentPhase -= 2.0 * M_PI;
    }
  }
  return signal;
}

// Генерация целого пакета ACARS из вектора бит
std::vector<double> generateAcarsPacket(const std::vector<bool>& bits,
                                        const AcarsConfig& cfg) {
  std::vector<double> packet;
  double phase = 0.0;

  // В ACARS перед данными всегда идет преамбула (чередование 1 и 0 для
  // синхронизации)
  std::vector<bool> preamble = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};

  for (bool bit : preamble) {
    auto bitSignal = generateBit(bit, phase, cfg);
    packet.insert(packet.end(), bitSignal.begin(), bitSignal.end());
  }

  // Сами полезные данные
  for (bool bit : bits) {
    auto bitSignal = generateBit(bit, phase, cfg);
    packet.insert(packet.end(), bitSignal.begin(), bitSignal.end());
  }

  return packet;
}

void saveSignal(const std::vector<double>& signal, std::string filename) {
  std::ofstream file(filename);

  for (auto sample : signal) {
    file << sample << '\n';
  }
}

void saveSignal_float(const std::vector<float>& signal, std::string filename) {
  std::ofstream file(filename);

  for (auto sample : signal) {
    file << sample << '\n';
  }
}

/* int main(void) {
  double fase = 0.0;
  int bit1 = 1;
  int bit2 = 0;
  AcarsConfig cfg;
  std::vector<double> vector = generateBit(bit2, fase, cfg);

  for (auto n : vector) {
    std::cout << n << std::endl;
  }

  return 0;
} */
