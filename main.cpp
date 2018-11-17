#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>


class TRandomNumberGenerator {
public:
	virtual ~TRandomNumberGenerator() = default;
	virtual double Generate() const = 0;
};

class PoissonGenerator : public TRandomNumberGenerator {
	mutable std::default_random_engine generator;
	mutable std::poisson_distribution<int> distribution;
public:
	PoissonGenerator(const double lambda) : distribution(lambda) {}
	double Generate() const final {
		return distribution(generator);
	}
};

class BernoulliGenerator : public TRandomNumberGenerator {
	mutable std::default_random_engine generator;
	mutable std::bernoulli_distribution distribution;
public:
	BernoulliGenerator(const double th) : distribution(th) {}
	double Generate() const final {
		return distribution(generator);
	}
};

class GeometricGenerator : public TRandomNumberGenerator {
	mutable std::default_random_engine generator;
	mutable std::geometric_distribution<int> distribution;
public:
	GeometricGenerator(const double th) : distribution(th) {}
	double Generate() const final {
		return distribution(generator);
	}
};

class FiniteGenerator : public TRandomNumberGenerator {
	mutable std::default_random_engine generator;
	mutable std::uniform_real_distribution<double> distribution;
	std::vector<double> values, probs;
public:
	FiniteGenerator(const std::vector<double>& v, const std::vector<double>& p) : values(v), probs(p), distribution(0.0, 1.0) {
		for (int i = 1; i < probs.size(); ++i) {
			probs[i] += probs[i - 1];
		}
	}
	double Generate() const final {
		const double val = distribution(generator);
		for (int i = 0; i < probs.size() - 1; ++i) {
			if (val > probs[i] && val <= probs[i + 1]) {
				return values[i];
			}
		}
		return values.back();
	}
};

double vector_sum(const std::vector<double>& v) {
	double res = 0;
	for (auto& x : v) {
		res += x;
	}
	return res;
}

class GeneratorFactory {
public:
	std::unique_ptr<TRandomNumberGenerator> createGenerator(const std::string& name, const double param) const {
		if (name == "poisson") {
			return std::make_unique<PoissonGenerator>(param);
		} else if (name == "bernoulli" && param >= 0 && param <= 1) {
			return std::make_unique<BernoulliGenerator>(param);
		} else if (name == "geometric" && param >= 0 && param <= 1) {
			return std::make_unique<BernoulliGenerator>(param);
		} else {
			return nullptr;
		}
	}
	std::unique_ptr<TRandomNumberGenerator> createGenerator(
		const std::string& name,
		const std::vector<double>& values,
		const std::vector<double>& probabilities
	) const {
		if (name == "finite"
			&& values.size()
			&& probabilities.size() == values.size()
			&& std::all_of(probabilities.begin(), probabilities.end(), [](double p){return 0 <= p && 1 >= p;})
			&& std::abs(vector_sum(probabilities) - 1) < 1e-9
		) {
			return std::make_unique<FiniteGenerator>(values, probabilities);
		} else {
			return nullptr;
		}
	}
};

int main() {
	std::array<double, 4> testLambdas = {1, 3.58, 5, 8};
	std::array<double, 4> testThresholds = {0, 1, 0.58, 0.83};
	std::array<std::pair<std::vector<double>, std::vector<double> >, 4> testSets = {
		std::make_pair(
			std::vector<double>({1, 2, 3}),
			std::vector<double>({0.3, 0.3, 0.4})
		),
		std::make_pair(
			std::vector<double>({1, 2}),
			std::vector<double>({0.3, 0.3, 0.4})
		),
		std::make_pair(
			std::vector<double>(),
			std::vector<double>()
		),
		std::make_pair(
			std::vector<double>({1, -1, 2, -2, 3, -3, 4, -4, 5, -5}),
			std::vector<double>({0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1})
		),
	};
	std::array<double, 4> testThresholds2 = {1, 0.58, 0.83, 0.35};

	GeneratorFactory factory;
	for (auto& lambda : testLambdas) {
		auto gen = factory.createGenerator("poisson", lambda);
		double mean = 0;
		int count = 100000;
		for (int i = 0; i < count; ++i) {
			mean += gen->Generate();
		}
		std::cout << "Poisson mean: " << lambda << "\nComputed: " << mean / count << std::endl;
	}
	for (auto& th : testThresholds) {
		auto gen = factory.createGenerator("bernoulli", th);
		double mean = 0;
		int count = 100000;
		for (int i = 0; i < count; ++i) {
			mean += gen->Generate();
		}
		std::cout << "Bernoulli mean: " << th << "\nComputed: " << mean / count << std::endl;
	}
	for (auto& th : testThresholds2) {
		auto gen = factory.createGenerator("geometric", th);
		double mean = 0;
		int count = 100000;
		for (int i = 0; i < count; ++i) {
			mean += gen->Generate();
		}
		std::cout << "Geometric mean: " << th << "\nComputed: " << mean / count << std::endl;
	}
	for (auto& [vals, prob] : testSets) {
		auto gen = factory.createGenerator("finite", vals, prob);
		if (!gen) {
			continue;
		}
		double realMean = 0;
		for (int i = 0; i < vals.size(); ++i) {
			realMean += vals[i] * prob[i];
		}
		double mean = 0;
		int count = 100000;
		for (int i = 0; i < count; ++i) {
			mean += gen->Generate();
		}
		std::cout << "Finite mean: " << realMean << "\nComputed: " << mean / count << std::endl;
	}
}