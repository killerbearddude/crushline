// Application entry point and command-line test dispatcher. Runtime mode
// starts the SDL/OpenGL app, while test flags call focused graph/catalog tests
// without launching the interactive editor loop.

#include <SDL3/SDL_main.h>

#include "app/App.h"

#include <charconv>
#include <iostream>
#include <string_view>
#include <system_error>

int RunGraphSerializerRoundTripTest();
int RunGraphEvaluatorTest();
int RunGraphConnectionTest();
int RunProductionCatalogTest();
int RunGraphRecipeNodeTest();
int RunScenarioCatalogTest();
int RunProductionEvaluatorTest();

namespace
{
bool ParsePositiveInt(std::string_view text, int& value)
{
    const char* begin = text.data();
    const char* end = begin + text.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc{} && ptr == end && value > 0;
}

void PrintUsage(const char* executable)
{
    std::cout
        << "Usage: " << executable << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --smoke-test       Run a short startup/render loop and exit.\n"
        << "  --serializer-roundtrip-test  Verify graph JSON save/load roundtrip.\n"
        << "  --graph-evaluator-test       Verify graph simulation metrics.\n"
        << "  --graph-connection-test      Verify graph connection validation rules.\n"
        << "  --production-catalog-test    Verify production resource/machine/recipe catalogs.\n"
        << "  --graph-recipe-node-test     Verify recipe-driven graph node port generation.\n"
        << "  --scenario-catalog-test      Verify scenario objectives and Tier 0 goals.\n"
        << "  --production-evaluator-test  Verify MVP production target evaluation.\n"
        << "  --frames <count>   Run for a fixed positive number of frames.\n"
        << "  --help, -h         Show this help text.\n";
}
}

int main(int argc, char** argv)
{
    AppConfig config;

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            PrintUsage(argv[0]);
            return 0;
        }

        if (arg == "--smoke-test")
        {
            config.maxFrames = 3;
            config.logEveryNFrames = 1;
            continue;
        }

        if (arg == "--serializer-roundtrip-test")
        {
            return RunGraphSerializerRoundTripTest();
        }

        if (arg == "--graph-evaluator-test")
        {
            return RunGraphEvaluatorTest();
        }

        if (arg == "--graph-connection-test")
        {
            return RunGraphConnectionTest();
        }

        if (arg == "--production-catalog-test")
        {
            return RunProductionCatalogTest();
        }

        if (arg == "--graph-recipe-node-test")
        {
            return RunGraphRecipeNodeTest();
        }

        if (arg == "--scenario-catalog-test")
        {
            return RunScenarioCatalogTest();
        }

        if (arg == "--production-evaluator-test")
        {
            return RunProductionEvaluatorTest();
        }

        if (arg == "--frames")
        {
            if (i + 1 >= argc || !ParsePositiveInt(argv[i + 1], config.maxFrames))
            {
                std::cerr << "Invalid or missing frame count for --frames.\n";
                return 2;
            }

            ++i;
            continue;
        }

        std::cerr << "Unknown argument: " << arg << "\n";
        PrintUsage(argv[0]);
        return 2;
    }

    App app;

    if (!app.Initialize(config))
    {
        std::cerr << "Failed to initialize app.\n";
        return 1;
    }

    while (!app.ShouldClose())
    {
        app.RunFrame();
    }

    app.Shutdown();
    return 0;
}
