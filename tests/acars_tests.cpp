#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <vector>

// Подключаем ваши заголовочные файлы (замените на реальные пути)
// #include "acars_generator.h"
// #include "fir_filter.h"
// #include "acars_parser.h"

// --- ТЕСТЫ ДЛЯ ГЕНЕРАТОРА СИГНАЛА ---

TEST(Acars_Test, GenerateBitLengthAndPhase) {
  AcarsConfig cfg;
  cfg.sampleRate = 48000.0;
  cfg.baudRate = 2400.0;

  // Вычисляем, сколько семплов должно быть в одном бите: 48000 / 2400 = 20
  // семплов
  size_t expected_samples = static_cast<size_t>(cfg.sampleRate / cfg.baudRate);

  double currentPhase = 0.0;
  bool test_bit = true;

  // Вызываем вашу функцию генерации бита
  std::vector<double> signal = generateBit(test_bit, currentPhase, cfg);

  // Проверяем, что размер вектора совпадает с расчетным
  ASSERT_EQ(signal.size(), expected_samples);

  // Проверяем, что фаза изменилась и не осталась нулевой
  EXPECT_NE(currentPhase, 0.0);
}