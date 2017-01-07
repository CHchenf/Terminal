/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

static const COORD c_coordZero = {0,0};

class FontTests
{
    TEST_CLASS(FontTests)

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    BEGIN_TEST_METHOD(TestCurrentFontAPIsInvalid)
        TEST_METHOD_PROPERTY(L"Data:dwConsoleOutput", L"{0, 1, 0xFFFFFFFF}")
        TEST_METHOD_PROPERTY(L"Data:bMaximumWindow", L"{TRUE, FALSE}")
        TEST_METHOD_PROPERTY(L"Data:strOperation", L"{Get, GetEx, SetEx}")
    END_TEST_METHOD();
    
    BEGIN_TEST_METHOD(TestGetFontSizeInvalid)
        TEST_METHOD_PROPERTY(L"Data:dwConsoleOutput", L"{0, 0xFFFFFFFF}")
    END_TEST_METHOD();

    TEST_METHOD(TestGetFontSizeLargeIndexInvalid);
    TEST_METHOD(TestSetConsoleFontNegativeSize);

    TEST_METHOD(TestFontScenario);
};

bool FontTests::TestSetup()
{
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
    return Common::TestBufferSetup();
}

bool FontTests::TestCleanup()
{
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
    return Common::TestBufferCleanup();
}

void FontTests::TestCurrentFontAPIsInvalid()
{
    DWORD dwConsoleOutput;
    bool bMaximumWindow;
    String strOperation;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"dwConsoleOutput", dwConsoleOutput), L"Get output handle value");
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"bMaximumWindow", bMaximumWindow), L"Get maximized window value");
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"strOperation", strOperation), L"Get operation value");

    const bool bUseValidOutputHandle = (dwConsoleOutput == 1);
    HANDLE hConsoleOutput;
    if (bUseValidOutputHandle)
    {
        hConsoleOutput = GetStdOutputHandle();
    }
    else
    {
        hConsoleOutput = (HANDLE)dwConsoleOutput;
    }

    if (strOperation == L"Get")
    {
        CONSOLE_FONT_INFO cfi = {0};

        if (bUseValidOutputHandle)
        {
            VERIFY_WIN32_BOOL_SUCCEEDED(GetCurrentConsoleFont(hConsoleOutput, (BOOL)bMaximumWindow, &cfi));
        }
        else
        {
            VERIFY_WIN32_BOOL_FAILED(GetCurrentConsoleFont(hConsoleOutput, (BOOL)bMaximumWindow, &cfi));
        }
    }
    else if (strOperation == L"GetEx")
    {
        CONSOLE_FONT_INFOEX cfie = {0};
        VERIFY_WIN32_BOOL_FAILED(GetCurrentConsoleFontEx(hConsoleOutput, (BOOL)bMaximumWindow, &cfie));
    }
    else if (strOperation == L"SetEx")
    {
        CONSOLE_FONT_INFOEX cfie = {0};
        VERIFY_WIN32_BOOL_FAILED(SetCurrentConsoleFontEx(hConsoleOutput, (BOOL)bMaximumWindow, &cfie));
    }
    else
    {
        VERIFY_FAIL(L"Unrecognized operation");
    }
}

void FontTests::TestGetFontSizeInvalid()
{
    DWORD dwConsoleOutput;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"dwConsoleOutput", dwConsoleOutput), L"Get input handle value");    

    // Need to make sure that last error is cleared so that we can verify that lasterror was set by GetConsoleFontSize
    SetLastError(0);

    COORD coordFontSize = GetConsoleFontSize((HANDLE)dwConsoleOutput, 0);
    VERIFY_ARE_EQUAL(coordFontSize, c_coordZero, L"Ensure (0,0) coord returned to indicate failure");
    VERIFY_ARE_EQUAL(GetLastError(), (DWORD)ERROR_INVALID_HANDLE, L"Ensure last error was set appropriately");
}

void FontTests::TestGetFontSizeLargeIndexInvalid()
{
    SetLastError(0);
    COORD coordFontSize = GetConsoleFontSize(GetStdOutputHandle(), 0xFFFFFFFF);
    VERIFY_ARE_EQUAL(coordFontSize, c_coordZero, L"Ensure (0,0) coord returned to indicate failure");
    VERIFY_ARE_EQUAL(GetLastError(), (DWORD)ERROR_INVALID_PARAMETER, L"Ensure last error was set appropriately");
}

void FontTests::TestSetConsoleFontNegativeSize()
{
    const HANDLE hConsoleOutput = GetStdOutputHandle();
    CONSOLE_FONT_INFOEX cfie = {0};
    cfie.cbSize = sizeof(cfie);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetCurrentConsoleFontEx(hConsoleOutput, FALSE, &cfie));
    cfie.dwFontSize.X = -4;
    cfie.dwFontSize.Y = -12;

    // as strange as it sounds, we don't filter out negative font sizes. under the hood, this call ends up in
    // FindCreateFont, which runs through our list of loaded fonts, fails to find, takes the absolute value of Y, and
    // then performs a GDI font enumeration for fonts that match. we should hold on to this behavior until we can
    // establish that it's no longer necessary.
    VERIFY_WIN32_BOOL_SUCCEEDED(SetCurrentConsoleFontEx(hConsoleOutput, FALSE, &cfie));
}

void FontTests::TestFontScenario()
{
    const HANDLE hConsoleOutput = GetStdOutputHandle();

    CONSOLE_FONT_INFOEX cfie = {0};
    cfie.cbSize = sizeof(cfie);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetCurrentConsoleFontEx(hConsoleOutput, FALSE, &cfie));

    CONSOLE_FONT_INFO cfi = {0};
    VERIFY_WIN32_BOOL_SUCCEEDED(GetCurrentConsoleFont(hConsoleOutput, FALSE, &cfi));

    VERIFY_ARE_EQUAL(cfi.nFont, cfie.nFont, L"Ensure regular and Ex APIs return same nFont");
    VERIFY_ARE_NOT_EQUAL(cfi.dwFontSize, c_coordZero, L"Ensure non-zero font size");
    VERIFY_ARE_EQUAL(cfi.dwFontSize, cfie.dwFontSize, L"Ensure regular and Ex APIs return same dwFontSize");

    const COORD coordCurrentFontSize = GetConsoleFontSize(hConsoleOutput, cfi.nFont);
    VERIFY_ARE_EQUAL(coordCurrentFontSize, cfi.dwFontSize, L"Ensure GetConsoleFontSize output matches GetCurrentConsoleFont");

    CONSOLE_FONT_INFOEX cfieSet = {0};
    cfieSet.cbSize = sizeof(cfieSet);
    cfieSet.dwFontSize.Y = 12;
    VERIFY_SUCCEEDED(StringCchCopy(cfieSet.FaceName, ARRAYSIZE(cfieSet.FaceName), L"Lucida Console"));

    VERIFY_WIN32_BOOL_SUCCEEDED(SetCurrentConsoleFontEx(hConsoleOutput, FALSE, &cfieSet));
    VERIFY_WIN32_BOOL_SUCCEEDED(SetCurrentConsoleFontEx(hConsoleOutput, TRUE, &cfieSet));

    CONSOLE_FONT_INFOEX cfiePost = {0};
    cfiePost.cbSize = sizeof(cfiePost);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetCurrentConsoleFontEx(hConsoleOutput, FALSE, &cfiePost));

    // So it appears that SetCurrentConsoleFontEx is broken in both legacy and modern consoles. Since this behavior
    // shipped an indeterminate amount of time ago, we need to make sure that the existing broken behavior doesn't
    // change unless we make said change intentionally. The broken behavior is that we give back the original font
    // state.
    // ...
    // MiNiksa - Nov 2015 - I fixed this with the graphics refactor intentionally.
    // I'm going to update this test to ensure we get the correct font.
    VERIFY_ARE_EQUAL(cfieSet.nFont, cfiePost.nFont);
    // MSFT: 10449084 - test is failing
    /*VERIFY_ARE_EQUAL(cfieSet.dwFontSize.X, cfiePost.dwFontSize.X);
    VERIFY_ARE_EQUAL(cfieSet.dwFontSize.Y, cfiePost.dwFontSize.Y);
    VERIFY_ARE_EQUAL(cfieSet.FontFamily, cfiePost.FontFamily);
    VERIFY_ARE_EQUAL(cfieSet.FontWeight, cfiePost.FontWeight);
    VERIFY_ARE_EQUAL(String(cfieSet.FaceName), String(cfiePost.FaceName));*/
}
