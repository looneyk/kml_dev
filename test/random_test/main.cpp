//------------------------------------------------------------------------------
// random_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/math/Random.hpp"
#include "quetzal/math/Xoshiro256StarStar.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <format>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    using engine_type = math::Xoshiro256StarStar;

    //--------------------------------------------------------------------------
    vector<uint64_t> draw(engine_type& engine, size_t count)
    {
        vector<uint64_t> values;
        for (size_t i = 0; i < count; ++i)
        {
            values.push_back(engine());
        }
        return values;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("xoshiro256**")
    {
        CHECK_CASE("is a uniform random bit generator", uniform_random_bit_generator<engine_type>);

        engine_type from_state(engine_type::state_type{1, 2, 3, 4});
        CHECK_CASE_EQUAL("from the state 1, 2, 3, 4, draws the reference implementation's values", draw(from_state, 10),
            (vector<uint64_t>{11520, 0, 1509978240, 1215971899390074240, 1216172134540287360, 607988272756665600, 16172922978634559625ull,
            8476171486693032832, 10595114339597558777ull, 2904607092377533576}));

        engine_type seeded(0);
        CHECK_CASE("seeded with 0, its state is splitmix64's first four values from 0",
            seeded == engine_type(engine_type::state_type{0xe220a8397b1dcdaf, 0x6e789e6aa1b965f4, 0x06c45d188009454f, 0xf88bb8a8724c81ec}));
        CHECK_CASE_EQUAL("seeded with 0, draws the values of that state", draw(seeded, 5),
            (vector<uint64_t>{11091344671253066420ull, 13793997310169335082ull, 1900383378846508768, 7684712102626143532, 13521403990117723737ull}));

        CHECK_CASE("a default-constructed engine is seeded with the default seed", engine_type() == engine_type(engine_type::default_seed));

        engine_type a(20261022);
        engine_type b(20261022);
        CHECK_CASE("two engines seeded alike draw alike", draw(a, 1000) == draw(b, 1000));

        a.seed(20261022);
        b.seed(20261022);
        CHECK_CASE("seeding again restarts the sequence", draw(a, 10) == draw(b, 10));

        CHECK_CASE("two seeds draw differently", engine_type(1)() != engine_type(2)());

        engine_type saved(7);
        draw(saved, 3);
        ostringstream os;
        os << saved;
        engine_type restored;
        istringstream is(os.str());
        is >> restored;
        CHECK_CASE("the state written and read back is equal", restored == saved);
        CHECK_CASE("and continues the same sequence", draw(restored, 10) == draw(saved, 10));

        engine_type unchanged(7);
        istringstream malformed("1 2 three 4");
        malformed >> unchanged;
        CHECK_CASE("reading a malformed state fails and leaves the engine unchanged", !malformed && unchanged == engine_type(7));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("Random")
    {
        using random_type = math::Random<>;

        CHECK_CASE("its default random engine is the portable random engine", (is_same_v<random_type, math::Random<math::portable_random_engine_type>>));

        random_type sequence(20261022);
        vector<int> integers;
        for (int i = 0; i < 5; ++i)
        {
            integers.push_back(sequence.random(10));
        }
        vector<double> reals;
        for (int i = 0; i < 3; ++i)
        {
            reals.push_back(sequence.random());
        }
        vector<int> closed;
        for (int i = 0; i < 5; ++i)
        {
            closed.push_back(sequence.random_closed(-5, 5));
        }
        vector<double> triangulars;
        for (int i = 0; i < 3; ++i)
        {
            triangulars.push_back(sequence.triangular(0.0, 1.0));
        }
        vector<int> shuffled{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        sequence.shuffle(shuffled);
        CHECK_CASE("seeded with 20261022, draws the values of the reference computation",
            integers == vector<int>{7, 0, 0, 6, 6}
            && reals == vector<double>{0.6576664257676362, 0.10932904142476729, 0.676894204682186}
            && closed == vector<int>{-1, 5, 1, 4, 3}
            && triangulars == vector<double>{0.11601881464091612, -0.39400299462655497, 0.5319746677851547}
            && shuffled == vector<int>{9, 6, 0, 2, 8, 1, 3, 5, 7, 4});

        random_type closed_sequence(20261023);
        vector<double> closed_reals;
        for (int i = 0; i < 3; ++i)
        {
            closed_reals.push_back(closed_sequence.random_closed(0.0, 1.0));
        }
        for (int i = 0; i < 2; ++i)
        {
            closed_reals.push_back(closed_sequence.random_closed(2.5));
        }
        vector<int> mixed_integers;
        for (int i = 0; i < 3; ++i)
        {
            mixed_integers.push_back(closed_sequence.random_closed(-5, 5));
        }
        for (int i = 0; i < 3; ++i)
        {
            mixed_integers.push_back(closed_sequence.random(-5, 5));
        }
        for (int i = 0; i < 2; ++i)
        {
            mixed_integers.push_back(closed_sequence.random_closed(10));
        }
        CHECK_CASE("seeded with 20261023, draws the closed and half-open values of the reference computation",
            closed_reals == vector<double>{0.7597855801818301, 0.5594030035670937, 0.13107050224712602, 0.59324933165946, 0.9919538129350103}
            && mixed_integers == vector<int>{5, 3, 2, 0, -5, 2, 9, 2});

        random_type random(1);
        constexpr int draw_count = 100000;

        array<int, 7> below_counts{};
        bool below_bounded = true;
        for (int i = 0; i < draw_count; ++i)
        {
            int value = random.random(7);
            below_bounded = below_bounded && value >= 0 && value < 7;
            if (below_bounded)
            {
                ++below_counts[static_cast<size_t>(value)];
            }
        }
        CHECK_CASE("random(n) draws integers in [0, n), each about equally often", below_bounded && ranges::all_of(below_counts, [](int count)
        {
            return count > draw_count / 7 * 9 / 10 && count < draw_count / 7 * 11 / 10;
        }));

        array<int, 7> half_open_counts{};
        bool half_open_bounded = true;
        for (int i = 0; i < draw_count; ++i)
        {
            int value = random.random(-3, 4);
            half_open_bounded = half_open_bounded && value >= -3 && value < 4;
            if (half_open_bounded)
            {
                ++half_open_counts[static_cast<size_t>(value + 3)];
            }
        }
        CHECK_CASE("random(first, last) draws integers in [first, last), each about equally often", half_open_bounded && ranges::all_of(half_open_counts, [](int count)
        {
            return count > draw_count / 7 * 9 / 10 && count < draw_count / 7 * 11 / 10;
        }));

        array<int, 7> closed_counts{};
        bool closed_bounded = true;
        for (int i = 0; i < draw_count; ++i)
        {
            int value = random.random_closed(-3, 3);
            closed_bounded = closed_bounded && value >= -3 && value <= 3;
            if (closed_bounded)
            {
                ++closed_counts[static_cast<size_t>(value + 3)];
            }
        }
        CHECK_CASE("random_closed(first, last) draws integers in [first, last], each about equally often", closed_bounded && ranges::all_of(closed_counts, [](int count)
        {
            return count > draw_count / 7 * 9 / 10 && count < draw_count / 7 * 11 / 10;
        }));

        array<int, 7> closed_below_counts{};
        bool closed_below_bounded = true;
        for (int i = 0; i < draw_count; ++i)
        {
            int value = random.random_closed(6);
            closed_below_bounded = closed_below_bounded && value >= 0 && value <= 6;
            if (closed_below_bounded)
            {
                ++closed_below_counts[static_cast<size_t>(value)];
            }
        }
        CHECK_CASE("random_closed(n) draws integers in [0, n], each about equally often", closed_below_bounded && ranges::all_of(closed_below_counts, [](int count)
        {
            return count > draw_count / 7 * 9 / 10 && count < draw_count / 7 * 11 / 10;
        }));

        bool full_range_drawn = true;
        for (int i = 0; i < 100; ++i)
        {
            random.random_closed(numeric_limits<int64_t>::min(), numeric_limits<int64_t>::max());
            random.random(numeric_limits<int64_t>::min(), numeric_limits<int64_t>::max());
            uint8_t byte = random.random_closed(uint8_t(0), uint8_t(255));
            full_range_drawn = full_range_drawn && byte <= 255;
        }
        CHECK_CASE("random and random_closed draw over a type's whole range", full_range_drawn);

        CHECK_CASE("random_closed of an interval of one value draws that value", random.random_closed(4, 4) == 4 && random.random_closed(2.5, 2.5) == 2.5
            && random.random_closed(0) == 0 && random.random_closed(0.0) == 0.0);

        bool reals_bounded = true;
        for (int i = 0; i < draw_count; ++i)
        {
            double unit = random.random();
            double below = random.random(2.5);
            float between = random.random(1.0f, 2.0f);
            reals_bounded = reals_bounded && unit >= 0.0 && unit < 1.0 && below >= 0.0 && below < 2.5 && between >= 1.0f && between < 2.0f;
        }
        CHECK_CASE("the half-open real draws keep to their intervals", reals_bounded);

        bool closed_reals_bounded = true;
        for (int i = 0; i < draw_count; ++i)
        {
            double below = random.random_closed(2.5);
            float between = random.random_closed(1.0f, 2.0f);
            double wide = random.random_closed(-1.0e300, 1.0e300);
            closed_reals_bounded = closed_reals_bounded && below >= 0.0 && below <= 2.5 && between >= 1.0f && between <= 2.0f
                && wide >= -1.0e300 && wide <= 1.0e300;
        }
        CHECK_CASE("the closed real draws keep to their intervals", closed_reals_bounded);

        double sum = 0.0;
        double sum_squares = 0.0;
        for (int i = 0; i < draw_count; ++i)
        {
            double value = random.gaussian(10.0, 2.0);
            sum += value;
            sum_squares += value * value;
        }
        double mean = sum / draw_count;
        double sd = sqrt(sum_squares / draw_count - mean * mean);
        CHECK_CASE("gaussian draws with the mean and standard deviation it is given", abs(mean - 10.0) < 0.05 && abs(sd - 2.0) < 0.05,
            format("mean {}, standard deviation {}", mean, sd));

        double triangular_sum = 0.0;
        bool triangular_bounded = true;
        for (int i = 0; i < draw_count; ++i)
        {
            double value = random.triangular(5.0, 2.0);
            triangular_bounded = triangular_bounded && value >= 3.0 && value <= 7.0;
            triangular_sum += value;
        }
        CHECK_CASE("triangular keeps to [mode - halfwidth, mode + halfwidth] about its mode",
            triangular_bounded && abs(triangular_sum / draw_count - 5.0) < 0.02);

        vector<int> permutation(50);
        iota(permutation.begin(), permutation.end(), 0);
        random.shuffle(permutation);
        vector<int> sorted = permutation;
        ranges::sort(sorted);
        vector<int> identity(50);
        iota(identity.begin(), identity.end(), 0);
        CHECK_CASE("shuffle permutes the range", sorted == identity && permutation != identity);

        vector<int> empty;
        vector<int> single{7};
        random.shuffle(empty);
        random.shuffle(single);
        CHECK_CASE("shuffle leaves an empty range and a range of one unchanged", empty.empty() && single == vector<int>{7});

        vector<int> partial{0, 1, 2, 3, 4, 5};
        random.shuffle(partial.begin() + 3, partial.end());
        CHECK_CASE("shuffle of part of a range leaves the rest unchanged", partial[0] == 0 && partial[1] == 1 && partial[2] == 2);

        vector<int> choices{1, 2, 3};
        int& chosen = random.choose(choices);
        chosen = 10;
        CHECK_CASE("choose returns a reference to an element of the range", ranges::count(choices, 10) == 1);

        CHECK_CASE("random_seed draws from the platform's entropy source", math::random_seed() != math::random_seed());

        random_type seeding(7);
        random_type full_range(7);
        CHECK_CASE("seed draws a seed from the whole range of seed_type",
            seeding.seed() == full_range.random_closed(numeric_limits<random_type::seed_type>::max()));

        random_type saved(7);
        saved.random(100);
        const string state = saved.state();
        vector<int> after_saving;
        for (int i = 0; i < 5; ++i)
        {
            after_saving.push_back(saved.random(100));
        }
        random_type restored(8);
        restored.set_state(state);
        vector<int> after_restoring;
        for (int i = 0; i < 5; ++i)
        {
            after_restoring.push_back(restored.random(100));
        }
        CHECK_CASE("a random engine restored from a saved state draws what the saved one drew after saving", after_restoring == after_saving,
            format("state \"{}\"", state));

        random_type unchanged(9);
        const string unchanged_state = unchanged.state();
        CHECK_THROWS_AS(unchanged.set_state("1 2 three 4"), invalid_argument);
        CHECK_THROWS_AS(unchanged.set_state("1 2 3 4 5"), invalid_argument);
        CHECK_CASE("a state that is not the random engine's is rejected, and leaves its state unchanged", unchanged.state() == unchanged_state);
        return;
    }

} // namespace
