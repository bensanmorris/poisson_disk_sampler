#include <benchmark/benchmark.h>
#include "poisson.h"

using namespace poisson;

static void BM_poisson(benchmark::State& s)
{
    struct MyFilter : public PoissonDiskMultiSampler::RealFunction2D
    {
        MyFilter(){}
        virtual bool placeObject(int layerIndex, float x, float y)
        {
            // NB. this is a callback you can register with the sampler to filter out placed objects
            return true;
        }
    };
    MyFilter distribution;

    std::vector<float> minDist = { 4.f, 12.f, 36.f };
    std::vector<float> minRadi = { 1.f, 3.f,  9.f  };
    std::vector<float> maxRadi = { 2.f, 6.f,  18.f };

    for(auto _ : s)
    {
        int tileSize = static_cast<int>(s.range(0));
        PoissonDiskMultiSampler sampler(-(tileSize / 2.f),
                                        -(tileSize / 2.f),
                                         (tileSize / 2.f),
                                         (tileSize / 2.f),
                                          minDist,
                                          minRadi,
                                          maxRadi,
                                          5*tileSize, // NB. set to zero to fill until full
                                          distribution,
                                          minDist.size() > 1,
                                          10);
        PoissonDiskMultiSampler::PointListArray layers;
        sampler.sample(layers);
    }
}
BENCHMARK(BM_poisson)->Unit(benchmark::kMillisecond)->RangeMultiplier(2)->Range(8, 8<<6);
BENCHMARK_MAIN();
