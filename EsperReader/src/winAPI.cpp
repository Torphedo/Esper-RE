#include <Windows.h>
#include <shobjidl.h>

#include <winAPI.h>
#include <main.h>

// ===== Windows File Dialogs =====

std::string PWSTR_to_string(PWSTR ws) {
    std::string result;
    result.reserve(wcslen(ws));
    for (; *ws; ws++)
        result += (char)*ws;
    return result;
}

int WINAPI FileSelectDialog(const COMDLG_FILTERSPEC* fileTypes)
{
    HRESULT hr;
    IFileOpenDialog* pFileOpen;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
        // Show the Open dialog box.
        hr = pFileOpen->SetFileTypes(2, fileTypes);
        hr = pFileOpen->Show(NULL);

        // Get the file name from the dialog box.
        if (hr == 0x800704c7) // ERROR_CANCELLED
        {
            return -1;
        }
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                // Display the file name to the user.
                if (SUCCEEDED(hr))
                {
                    filepath = PWSTR_to_string(pszFilePath);
                    filepathptr = const_cast<char*>(filepath.c_str());
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    CoUninitialize();
    return 0;
}
