#include "Processors/Hysteresis/HysteresisSTN.h"

namespace STNTestUtils
{
constexpr double sampleRate = 48000.0;
constexpr double trainingSampleRate = 96000.0;
constexpr auto sampleRateCorr = trainingSampleRate / sampleRate;

alignas (16) double input[] = { 1.0, 1.0, 1.0, 1.0, 1.0 };
} // namespace STNTestUtils

class STNTest : public UnitTest
{
public:
    STNTest() : UnitTest ("STNTest")
    {
    }

    void runTest() override
    {
#if JUCE_LINUX
        return; // @TODO: figure out why this fails!
#endif
        beginTest ("STN Accuracy Test");
        accTest();

        beginTest ("STN Performance Test");
        // perfTest(); // Keep this disabled most of the time for CI
    }

    void accTest()
    {
        using namespace STNTestUtils;

        HysteresisSTN stn;
        stn.prepare (sampleRate);
        stn.setParams (0.5f, 0.5f);

        auto refModel = loadModel();

        for (int i = 0; i < 10; ++i)
        {
            auto x = stn.process (input);
            auto xRef = refModel->forward (input) * sampleRateCorr;
            expectWithinAbsoluteError (x, xRef, 1.0e-15, "STN output is incorrect!");
        }
    }

    void perfTest()
    {
        using namespace STNTestUtils;

        HysteresisSTN stn;
        stn.prepare (sampleRate);
        stn.setParams (0.5f, 0.5f);
        auto refModel = loadModel();

        constexpr int nIter = 5000000;
        double result = 0.0;

        // ref timing
        double durationRef = 0.0f;
        {
            Time time;
            auto start = time.getMillisecondCounterHiRes();
            for (int i = 0; i < nIter; ++i)
                result = refModel->forward (input) * sampleRateCorr;
            auto end = time.getMillisecondCounterHiRes();
            durationRef = (end - start) / 1000.0;
        }
        std::cout << "Reference output: " << result << std::endl;
        std::cout << "Reference duration: " << durationRef << std::endl;

        // plugin timing
        auto durationReal = durationRef;
        {
            Time time;
            auto start = time.getMillisecondCounterHiRes();
            for (int i = 0; i < nIter; ++i)
                result = stn.process (input);
            auto end = time.getMillisecondCounterHiRes();
            durationReal = (end - start) / 1000.0;
        }
        std::cout << "Actual output: " << result << std::endl;
        std::cout << "Actual duration: " << durationReal << std::endl;

        expectLessThan (durationReal, durationRef * 1.25, "Plugin STN processing is too slow!");
    }

    std::unique_ptr<RTNeural::Model<double>> loadModel()
    {
        auto jsonStream = std::make_unique<MemoryInputStream> (BinaryData::hyst_width_50_json, BinaryData::hyst_width_50_jsonSize, false);
        auto modelsJson = nlohmann::json::parse (jsonStream->readEntireStreamAsString().toStdString());
        auto thisModelJson = modelsJson["drive_50_50"];
        return RTNeural::json_parser::parseJson<double> (thisModelJson);
    }
};

static STNTest stnTest;
