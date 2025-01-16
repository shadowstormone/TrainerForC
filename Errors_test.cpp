#include <gtest/gtest.h>
#include <windows.h>
#include <string>
#include "Memory_Functions.h"
#include "CavePatch.h"

// Вспомогательная функция для преобразования std::string в std::wstring
std::wstring StringToWString(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Тестовые адреса
uintptr_t fromAddress = 0x1000;
uintptr_t toAddress = 0x2000;

// Класс для тестов
class CavePatchTest : public ::testing::Test
{
protected:
    std::unique_ptr<CavePatch> patch;  // Умный указатель на CavePatch

    CavePatchTest()
    {
        patch = std::make_unique<CavePatch>(nullptr, L"", nullptr, 0);
    }

    virtual void SetUp()
    {
        // Инициализация ресурсов для тестов
    }

    virtual void TearDown()
    {
        // Очистка ресурсов
    }
};

TEST(ShowErrorMessageTest, ValidErrorMessage)
{
    SetLastError(ERROR_ACCESS_DENIED);
    DWORD errorCode = GetLastError();       //Сохраняем код ошибки до вызова функций, которые могут его изменить
    testing::internal::CaptureStderr();     //Изменено на CaptureStderr для MessageBox
    ShowErrorMessage(NULL, L"Failed to open process.\r\nError code: ", errorCode);
    std::string output = testing::internal::GetCapturedStderr();  //Изменено на GetCaptureStderr для MessageBox
    std::wstring woutput = StringToWString(output);
    ASSERT_NE(woutput.find(L"Error code: 5"), std::wstring::npos);
}

TEST(ShowErrorMessageTest, UnknownError)
{
    SetLastError(9999); // Устанвливаем неизвестный код ошибки
    testing::internal::CaptureStderr();  //Изменено на CaptureStderr для MessageBox
    ShowErrorMessage(NULL, L"Unknown error occurred.\r\nError code: ");
    std::string output = testing::internal::GetCapturedStderr();  //Изменено на GetCaptureStderr для MessageBox
    std::wstring woutput = StringToWString(output);
    ASSERT_NE(woutput.find(L"9999"), std::wstring::npos);
    ASSERT_NE(woutput.find(L"Unknown error"), std::wstring::npos);
}

TEST(ShowErrorMessageTest2, ValidErrorMessage2)
{
    SetLastError(ERROR_INVALID_ACCESS);
    DWORD errorCode = GetLastError();       // Сохраняем код ошибки до вызова функций, которые могут его изменить
    testing::internal::CaptureStderr();     //Изменено на CaptureStderr для MessageBox
    ShowErrorMessage(NULL, L"Invalid access.\r\nError code: ", errorCode);
    std::string output = testing::internal::GetCapturedStderr();  //Изменено на GetCaptureStderr для MessageBox
    std::wstring woutput = StringToWString(output);
    ASSERT_NE(woutput.find(L"Error code: 12"), std::wstring::npos);
}

// Тест для короткого перехода (расстояние меньше 2^31)
TEST_F(CavePatchTest, CalculateJumpBytesShortDistanceWithErrorHandling)
{
    BYTE outSize = 0;
    SetLastError(ERROR_ACCESS_DENIED);
    DWORD errorCode = GetLastError();  // Сохраняем код ошибки до вызова функций, которые могут его изменить
    testing::internal::CaptureStderr();
    ShowErrorMessage(NULL, L"Failed to calculate jump bytes.\r\nError code: ", errorCode);
    PBYTE result = patch->CalculateJumpBytes(reinterpret_cast<LPVOID>(fromAddress), reinterpret_cast<LPVOID>(toAddress), outSize);
    ASSERT_EQ(outSize, 5);          // Ожидается короткий переход (5 байт)
    ASSERT_EQ(result[0], 0xE9);     // E9 - код операции для близкого перехода
    int32_t expectedRelAddr = static_cast<int32_t>(toAddress - fromAddress - 5);
    int32_t actualRelAddr;
    std::memcpy(&actualRelAddr, &result[1], sizeof(actualRelAddr));
    ASSERT_EQ(expectedRelAddr, actualRelAddr);
    std::string capturedOutput = testing::internal::GetCapturedStderr();
    std::wstring woutput = StringToWString(capturedOutput);
    ASSERT_NE(woutput.find(L"Error code: 5"), std::wstring::npos); // Проверка, что в stderr появилось сообщение с установленным кодом ошибки
    delete[] result;
}

// Тест для длинного перехода (расстояние больше 2^31, меньше 2^63)
TEST_F(CavePatchTest, CalculateJumpBytesLongDistanceWithErrorHandling)
{
    BYTE outSize = 0;
    uintptr_t longFromAddress = 0x100000;
    uintptr_t longToAddress = 0x9000000000;
    SetLastError(ERROR_INVALID_ADDRESS);
    DWORD errorCode = GetLastError();  // Сохраняем код ошибки до вызова функций, которые могут его изменить
    testing::internal::CaptureStderr();
    ShowErrorMessage(NULL, L"Failed to calculate long distance jump bytes.\r\nError code: ", errorCode);
    PBYTE result = patch->CalculateJumpBytes(reinterpret_cast<LPVOID>(longFromAddress), reinterpret_cast<LPVOID>(longToAddress), outSize);
    ASSERT_EQ(outSize, 14);         // Ожидается длинная инструкция перехода (14 байт)
    ASSERT_EQ(result[0], 0xFF);     // FF 25 - код для дальнего перехода
    ASSERT_EQ(result[1], 0x25);
    std::string capturedOutput = testing::internal::GetCapturedStderr();
    std::wstring woutput = StringToWString(capturedOutput);
    ASSERT_NE(woutput.find(L"Error code: 487"), std::wstring::npos);  // ERROR_INVALID_ADDRESS = 487
    delete[] result;
}

// Тест для одинаковых адресов (без перехода)
TEST_F(CavePatchTest, CalculateJumpBytesSameAddressWithErrorHandling)
{
    BYTE outSize = 0;
    SetLastError(ERROR_SUCCESS);        // Ошибка отсутствует
    DWORD errorCode = GetLastError();   // Сохраняем код ошибки до вызова функций, которые могут его изменить
    testing::internal::CaptureStderr(); // Перехват вывода ошибок
    ShowErrorMessage(NULL, L"Same address jump bytes calculation.\r\nError code: ", errorCode);
    PBYTE result = patch->CalculateJumpBytes(reinterpret_cast<LPVOID>(fromAddress), reinterpret_cast<LPVOID>(fromAddress), outSize);
    ASSERT_EQ(outSize, 5);
    ASSERT_EQ(result[0], 0xE9);  // E9 - код для перехода
    int32_t expectedRelAddr = static_cast<int32_t>(0 - 5);
    int32_t actualRelAddr;
    std::memcpy(&actualRelAddr, &result[1], sizeof(actualRelAddr));
    ASSERT_EQ(expectedRelAddr, actualRelAddr);
    std::string capturedOutput = testing::internal::GetCapturedStderr();
    std::wstring woutput = StringToWString(capturedOutput);
    ASSERT_NE(woutput.find(L"Error code: 0"), std::wstring::npos); // Ошибки не должно быть (Error code: 0)
    delete[] result;
}

// Тест для граничного случая — переход ровно на 2^31 байтов
TEST_F(CavePatchTest, CalculateJumpBytesInvalidAddressWithErrorHandling)
{
    BYTE outSize = 0;
    SetLastError(ERROR_INVALID_ADDRESS);
    DWORD errorCode = GetLastError();   // Сохраняем код ошибки до вызова функций, которые могут его изменить
    testing::internal::CaptureStderr(); // Перехват вывода ошибок
    ShowErrorMessage(NULL, L"Invalid address range.\r\nError code: ", errorCode);
    ASSERT_THROW({patch->CalculateJumpBytes(reinterpret_cast<LPVOID>(static_cast<uintptr_t>(0xFFFFFFFF)), reinterpret_cast<LPVOID>(0x1), outSize);}, std::overflow_error);
    std::string capturedOutput = testing::internal::GetCapturedStderr();
    std::wstring woutput = StringToWString(capturedOutput);
    ASSERT_NE(woutput.find(L"Error code: 487"), std::wstring::npos);  // ERROR_INVALID_ADDRESS = 487
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}