#include <iostream>
#include <vector>
#include <random>
#include <mveqf/implicit_trie.h>

int main()
{
	std::mt19937_64 generator;
	generator.seed(1);
	std::uniform_int_distribution<int> dim_distr(4, 100);
	std::uniform_int_distribution<int> grid_distr(1, 20);
	std::uniform_int_distribution<int> count_distr(1, 10);
	std::uniform_real_distribution<float> bounds(-100.0f, 100.0f);

	size_t dimension = dim_distr(generator);

	std::vector<size_t> grid(dimension);
	for(auto & i : grid)
		i = grid_distr(generator);

	std::vector<float> lb(dimension); // lower bound
	std::vector<float> ub(dimension); // upper bound

	for(size_t i = 0; i != dimension; i++)
	{
		auto lower = bounds(generator);
		auto upper = bounds(generator);
		while(lower > upper)
		{
			lower = bounds(generator);
			upper = bounds(generator);
		}
		lb[i] = lower;
		ub[i] = upper;
	}

	auto sample = std::make_shared<mveqf::Trie<mveqf::NodeCount<int>,int>>();
	sample->set_dimension(dimension);

	size_t nsamples = 10000;
	for(size_t i = 0; i != nsamples; i++)
	{
		std::vector<int> point(dimension);
		for(size_t j = 0; j != point.size(); j++)
		{
			std::uniform_int_distribution<int> grid_distr(0, grid[j] - 1);
			point[j] = grid_distr(generator);
		}
		if(!sample->search(point))
			sample->insert(point, count_distr(generator)); // 1 for uniform
	}

	mveqf::ImplicitTrieQuantile<int, float> mveqfunc(lb, ub, grid);
	mveqfunc.set_sample_shared(sample);

	std::uniform_real_distribution<float> ureal01(0.0f, 1.0f);
	std::vector<float> values01(dimension);
	std::vector<float> sampled(dimension);
	size_t nsampled = 100;
	for(size_t i = 0; i != nsampled; i++)
	{
		for(auto & j : values01)
			j = ureal01(generator);

		mveqfunc.transform(values01, sampled);

		for(const auto & j : sampled)
			std::cout << std::fixed << j << '\t';
		std::cout << std::endl;
	}
}
