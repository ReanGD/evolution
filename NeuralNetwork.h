#pragma once
#include <memory>
#include <vector>
#include "math.h"


class Neuron {
public:
  Neuron() = delete;
  Neuron(uint16_t inputCnt, Generator& generator);
public:
  float calc(const std::shared_ptr<float[]>& inputs);
private:
  uint16_t m_inputCnt;
  std::unique_ptr<float[]> m_weights;
};

class Layer {
public:
  Layer() = delete;
  Layer(uint16_t inputCnt, uint16_t neuronCnt, Generator& generator);
public:
  std::shared_ptr<float[]> calc(const std::shared_ptr<float[]>& inputs);
private:
  std::vector<Neuron> m_neurons;
};

class NeuralNetwork {
public:
  NeuralNetwork(uint16_t inputCnt, uint16_t neuronLayer0Cnt, Generator& generator);
public:
  float calc(const std::shared_ptr<float[]>& inputs);
private:
  Layer m_layer0;
  Neuron m_output;
};
