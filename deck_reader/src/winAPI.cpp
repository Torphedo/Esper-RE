#include <shobjidl.h>

extern "C" {
    char *FileSelectDialog() {
        char *filename = nullptr;
        HRESULT hr;
        IFileOpenDialog *pFileOpen;
        const COMDLG_FILTERSPEC fileTypes[] = {L"Deck File", L"*.*;"};

        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog,
                              reinterpret_cast<void **>(&pFileOpen));

        if (SUCCEEDED(hr)) {
            // Show the Open dialog box.
            hr = pFileOpen->SetFileTypes(1, fileTypes);
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (hr == 0x800704c7) // ERROR_CANCELLED
            {
                return nullptr;
            }
            if (SUCCEEDED(hr)) {
                IShellItem *pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr)) {
                        size_t length = wcslen(pszFilePath);
                        filename = (char *) malloc(length);
                        for (unsigned long long i = 0; i < length; i++) {
                            filename[i] = pszFilePath[i];
                        }
                        filename[length] = 0x0; // Null terminator

                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
        return filename;
    }
}
