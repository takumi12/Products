#pragma once
#ifndef UTIL_PACK_H
#define UTIL_PACK_H

extern bool CreateFileDialog(std::string& FileLink, LPCWSTR pszTitle, REFCLSID rclsid, COMDLG_FILTERSPEC fileType, bool option /*= false*/, FILEOPENDIALOGOPTIONS fos /*= FOS_OVERWRITEPROMPT*/);

extern void ListDLLDependencies(const char* exePath, const char* outputFile);

extern void ListDLLDependencies(const char* exePath);

#endif