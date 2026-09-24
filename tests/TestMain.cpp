#include <gtest/gtest.h>

#include "platform/AudioService.h"

// Общая подготовка тестов: без звука. Тесты включают и выключают читы
// десятками — щелчки ни к чему, а на машине без звуковой карты (CI) движок
// звука и вовсе может не подняться.
namespace
{
    class QuietEnvironment : public ::testing::Environment
    {
    public:
        void SetUp() override { AudioService::Instance().SetEnabled(false); }
    };

    const auto* const g_quiet = ::testing::AddGlobalTestEnvironment(new QuietEnvironment);
}
