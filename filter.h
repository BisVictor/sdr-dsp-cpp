#include <cmath>
#include <iostream>
#include <vector>

// Функция генерации коэффициентов ФНЧ (Аналог fir1 из MATLAB)
std::vector<float> design_lowpass_fir(int num_taps, float sample_rate,
                                      float cutoff_freq);

std::vector<float> design_bandpass_fir(int num_taps, float sample_rate,
                                       float f_low, float f_high);