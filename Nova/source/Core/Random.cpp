#include "Core/Random.h"
#include "Core/Log.h"

namespace Nova::Random
{
    static bool is_initialized = false;
    static RandomState state;

    void Init()
    {
        if (is_initialized)
        {
            WARN("Random::Init - %s", "The random number generator cannot be initalized more than once");
            return;
        }

        state.engine = std::mt19937_64(state.device());
        state.uuidDistribution = std::uniform_int_distribution<UUID>(0, std::numeric_limits<UUID>::max());
        is_initialized = true;
    }

    UUID GenerateUUID()
    {
        ASSERT_RETURN(is_initialized, Stub_UUID, "Random::GenerateUUID - %s", "Random::Init must be called first");
        return state.uuidDistribution(state.engine);
    }

    u32 Generate(u32 min, u32 max)
    {
        std::uniform_int_distribution<u32> distribution(min, max);
        return distribution(state.engine);
    }

    float Generate(float min, float max)
    {
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(state.engine);
    }
}
