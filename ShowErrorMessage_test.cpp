#include <gtest/gtest.h>
#include <windows.h>
#include <string>
#include "Memory_Functions.h"

// Вспомогательная функция для преобразования std::string в std::wstring
std::wstring StringToWString(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

TEST(ShowErrorMessageTest, ValidErrorMessage)
{
    SetLastError(ERROR_ACCESS_DENIED);
    testing::internal::CaptureStderr();  //Изменено на CaptureStderr для MessageBox
    ShowErrorMessage(NULL, L"Failed to open process.\r\nError code:");
    std::string output = testing::internal::GetCapturedStderr();  //Изменено на GetCaptureStderr для MessageBox
    std::wstring woutput = StringToWString(output);
    ASSERT_NE(woutput.find(L"Error code: 5"), std::wstring::npos);
}

TEST(ShowErrorMessageTest, UnknownError)
{
    SetLastError(9999); // Устанвливаем неизвестный код ошибки
    testing::internal::CaptureStderr();  //Изменено на CaptureStderr для MessageBox
    ShowErrorMessage(NULL, L"Unknown error occurred.\r\nError code:");
    std::string output = testing::internal::GetCapturedStderr();  //Изменено на GetCaptureStderr для MessageBox
    std::wstring woutput = StringToWString(output);
    ASSERT_NE(woutput.find(L"9999"), std::wstring::npos);
    ASSERT_NE(woutput.find(L"Unknown error"), std::wstring::npos);
}

TEST(ShowErrorMessageTest2, ValidErrorMessage2)
{
    SetLastError(ERROR_INVALID_ACCESS);
    testing::internal::CaptureStderr();  //Изменено на CaptureStderr для MessageBox
    ShowErrorMessage(NULL, L"Invalid access.\r\nError code:");
    std::string output = testing::internal::GetCapturedStderr();  //Изменено на GetCaptureStderr для MessageBox
    std::wstring woutput = StringToWString(output);
    ASSERT_NE(woutput.find(L"Error code: 12"), std::wstring::npos);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}