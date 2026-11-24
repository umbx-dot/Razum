#include <stdio.h>
#include <wchar.h>

#include "setup.h"

static int failures = 0;

static void expect_text(const wchar_t *actual, const wchar_t *expected, const char *what) {
    if (wcscmp(actual, expected) != 0) {
        printf("FAIL %s: got [%ls], expected [%ls]\n", what, actual, expected);
        failures++;
    }
}

static void expect_true(bool actual, bool expected, const char *what) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", what, actual, expected);
        failures++;
    }
}

int main(void) {
    wchar_t out[256];
    const wchar_t *bin = L"C:\\Users\\a\\AppData\\Local\\Programs\\razum\\bin";

    expect_true(path_list_contains(L"", bin), false, "empty list");
    expect_true(path_list_contains(L"C:\\x;c:\\users\\A\\appdata\\local\\programs\\razum\\bin\\;D:\\y", bin), true, "case and trailing slash");
    expect_true(path_list_contains(L"C:\\x;C:\\Users\\a\\AppData\\Local\\Programs\\razum\\binary", bin), false, "longer entry");

    path_list_add(L"", bin, out, 256);
    expect_text(out, bin, "add to empty");
    path_list_add(L"C:\\x", L"D:\\y", out, 256);
    expect_text(out, L"C:\\x;D:\\y", "add after entry");
    path_list_add(L"C:\\x;", L"D:\\y", out, 256);
    expect_text(out, L"C:\\x;D:\\y", "add after separator");
    path_list_add(L"C:\\x;D:\\Y\\", L"d:\\y", out, 256);
    expect_text(out, L"C:\\x;D:\\Y\\", "add existing");
    expect_true(path_list_add(L"C:\\x", L"D:\\y", out, 6), false, "add overflow");

    path_list_remove(L"C:\\x;D:\\y\\;E:\\z", L"d:\\Y", out, 256);
    expect_text(out, L"C:\\x;E:\\z", "remove middle");
    path_list_remove(L"D:\\y;;C:\\x;D:\\y", L"D:\\y", out, 256);
    expect_text(out, L"C:\\x", "remove repeated and empty");
    path_list_remove(L"C:\\x", L"D:\\y", out, 256);
    expect_text(out, L"C:\\x", "remove absent");

    printf(failures == 0 ? "ok   installer path list\n" : "FAIL installer path list\n");
    return failures;
}
