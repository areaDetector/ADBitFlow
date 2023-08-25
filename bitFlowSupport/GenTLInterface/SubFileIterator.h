/* FILE:        SubFileIterator.h
 * DATE:        4/21/2015
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2015, BitFlow, Inc.
 * DESCRIPTION: Class to iterate over the contents of a directory.
 */

#ifndef INCLUDED__SUB__FILE__ITERATOR__H
#define INCLUDED__SUB__FILE__ITERATOR__H

#include <string>

#if defined(_WIN32)

#   if defined(_DEBUG)
#       define _CRTDBG_MAP_ALLOC
#       include <stdlib.h>
#       include <crtdbg.h>
#   endif

#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>

class Win32SubFileIterator
{
public:
    static std::wstring cleanPath (std::wstring const& path)
    {
        const size_t lastValid = path.find_last_not_of(L"\\");
        std::wstring cPath = std::wstring::npos == lastValid ? path : path.substr(0, 1 + lastValid);

        for (size_t rAt = 0; (rAt = cPath.find(L"\\\\", rAt)) != std::wstring::npos; )
            cPath.erase(rAt, 1);

        for (auto &cChar : cPath)
        {
            if (L'/' == cChar)
                cChar = L'\\';
        }

        return cPath;
    }

private:
    const std::wstring  m_basePath;
    const std::wstring  m_mask;
    HANDLE              m_findHandle;
    WIN32_FIND_DATA     m_findData;

    // Illegal.
    Win32SubFileIterator (void);
    Win32SubFileIterator (Win32SubFileIterator const&);
    Win32SubFileIterator& operator= (Win32SubFileIterator const&);

public:
    Win32SubFileIterator (std::wstring const& basePath, std::wstring const& mask = std::wstring())
        : m_basePath ( cleanPath(basePath) )
        , m_mask     ( mask )
    {
        const std::wstring searchPath = m_basePath + L"\\" + m_mask;
        m_findHandle = FindFirstFile(searchPath.c_str(), &m_findData);
    }

    ~Win32SubFileIterator (void)
    {
        close();
    }
    
    inline bool good (void) const
    {
        return INVALID_HANDLE_VALUE != m_findHandle;
    }
    inline operator bool (void) const
    {
        return good();
    }
    inline bool operator! (void) const
    {
        return !good();
    }

    inline std::wstring base_path (void) const
    {
        return m_basePath;
    }
    inline std::wstring mask (void) const
    {
        return m_mask;
    }

    inline void close (void)
    {
        if (INVALID_HANDLE_VALUE != m_findHandle)
        {
            FindClose(m_findHandle);
            m_findHandle = INVALID_HANDLE_VALUE;
        }
    }

    inline Win32SubFileIterator& operator++ (void)
    {
        if (good() && !FindNextFile(m_findHandle, &m_findData))
            close();
        return *this;
    }
    inline Win32SubFileIterator& operator++ (int)
    {
        if (good() && !FindNextFile(m_findHandle, &m_findData))
            close();
        return *this;
    }
        
    inline bool is_directory (void) const
    {
        return good() && (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    inline bool is_file (void) const
    {
        return good() && !(m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    inline std::wstring file_name (void) const
    {
        return good() ? m_findData.cFileName : L"";
    }

    inline std::wstring file_path (void) const
    {
        return good() ? cleanPath(m_basePath + L"\\" + file_name()) : std::wstring();
    }
};

typedef Win32SubFileIterator SubFileIterator;

#elif defined(__GNUC__)

#include <glob.h>
#include <sys/stat.h>

class LinuxSubFileIterator
{
public:
    static std::string cleanPath (std::string const& path)
    {
        const size_t lastValid = path.find_last_not_of("/");
        std::string cPath = std::string::npos == lastValid ? path : path.substr(0, 1 + lastValid);

        for (size_t rAt = 0; (rAt = cPath.find("//", rAt)) != std::string::npos; )
            cPath.erase(rAt, 1);

        return cPath;
    }

private:
    const std::string   m_basePath;
    const std::string   m_mask;

    glob_t m_glob;
    size_t m_index;

    // Illegal.
    LinuxSubFileIterator (void);
    LinuxSubFileIterator (LinuxSubFileIterator const&);
    LinuxSubFileIterator& operator= (LinuxSubFileIterator const&);

public:
    LinuxSubFileIterator (std::string const& basePath, std::string const& mask = std::string())
        : m_basePath ( cleanPath(basePath) )
        , m_mask ( mask )
        , m_index (0)
    {
        const std::string searchPath = m_basePath + "/" + m_mask;
        glob(searchPath.c_str(), GLOB_TILDE, NULL, &m_glob);
    }

    ~LinuxSubFileIterator (void)
    {
        close();
    }

    inline bool good (void) const
    {
        return m_glob.gl_pathc > 0 && m_glob.gl_pathc > m_index;
    }
    inline operator bool (void) const
    {
        return good();
    }
    inline bool operator! (void) const
    {
        return !good();
    }

    inline std::string base_path (void) const
    {
        return m_basePath;
    }
    inline std::string mask (void) const
    {
        return m_mask;
    }

    inline void close (void)
    {
        if (m_glob.gl_pathc > 0)
            globfree(&m_glob);
        m_glob.gl_pathc = 0;
    }

    inline LinuxSubFileIterator& operator++ (void)
    {
        m_index++;
        if (!good())
            close();
        return *this;
    }
    inline LinuxSubFileIterator& operator++ (int)
    {
        m_index++;
        if (!good())
            close();
        return *this;
    }

    inline bool is_directory (void) const
    {
        if (!good())
            return false;

        struct stat pathStat;
        stat(m_glob.gl_pathv[m_index], &pathStat);
        return S_ISDIR(pathStat.st_mode);
    }
    inline bool is_file (void) const
    {
        if (!good())
            return false;

        struct stat pathStat;
        stat(m_glob.gl_pathv[m_index], &pathStat);
        return S_ISREG(pathStat.st_mode);
    }

    inline std::string file_name (void) const
    {
        if (!good())
            return std::string();

        std::string path = m_glob.gl_pathv[m_index];
        const size_t finalSeparator = path.rfind('/');
        return std::string::npos == finalSeparator ? path : path.substr(finalSeparator + 1);
    }

    inline std::string file_path (void) const
    {
        return good() ? m_glob.gl_pathv[m_index] : std::string();
    }
};

typedef LinuxSubFileIterator SubFileIterator;

#else
#   error Path functions have not been implemented on this platform.
#endif

#endif // INCLUDED__SUB__FILE__ITERATOR__H
