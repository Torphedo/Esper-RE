#pragma once

int WINAPI FileSelectDialog(const COMDLG_FILTERSPEC* fileTypes);

const COMDLG_FILTERSPEC fileTypes[] = { L"Deck File", L"*.*;", L"ALR File", L"*.alr;" };
