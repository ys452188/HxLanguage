#ifndef HXLANG_SCANNER_H
#define HXLANG_SCANNER_H
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef _WIN32
#include <stdint.h>
#include <string.h>

/*
  将 UTF-8 字节序列解码为 UTF-16（wchar_t），适配 Windows (wchar_t == 2) 和 Unix
  (wchar_t == 4)：
  - 输入: ptr 指向 UTF-8 数据，len 为字节长度
  - 输出: *out 为 null 结尾的 wchar_t* （由函数 malloc 出来，调用者负责
  free），返回 0 成功，非 0 失败
*/
static int utf8_to_wcs(const char* ptr, size_t len, wchar_t** out) {
  if (!ptr || !out) return -1;
  *out = NULL;

  // First pass: estimate number of wchar_t units needed.
  size_t i = 0;
  size_t needed = 0;
  while (i < len) {
    unsigned char c = (unsigned char)ptr[i];
    if (c < 0x80) {
      // ASCII -> one code unit
      needed += 1;
      i += 1;
    } else if ((c >> 5) == 0x6) {
      // 2-byte
      if (i + 1 >= len) return -1;
      needed += 1;
      i += 2;
    } else if ((c >> 4) == 0xE) {
      // 3-byte
      if (i + 2 >= len) return -1;
      needed += 1;
      i += 3;
    } else if ((c >> 3) == 0x1E) {
      // 4-byte -> may become surrogate pair if wchar_t==2
      if (i + 3 >= len) return -1;
      // if wchar_t is 2 bytes, this becomes 2 wchar_t units (surrogate pair)
      if (sizeof(wchar_t) == 2)
        needed += 2;
      else
        needed += 1;
      i += 4;
    } else {
      // invalid leading byte
      return -1;
    }
  }

  // allocate (needed + 1) wchar_t (for terminating NUL)
  wchar_t* wbuf = (wchar_t*)malloc((needed + 1) * sizeof(wchar_t));
  if (!wbuf) return -1;

  i = 0;
  size_t wi = 0;
  while (i < len) {
    uint32_t codepoint = 0;
    unsigned char c = (unsigned char)ptr[i];
    if (c < 0x80) {
      codepoint = c;
      i += 1;
    } else if ((c >> 5) == 0x6) {
      // 110x yyyy
      if (i + 1 >= len) {
        free(wbuf);
        return -1;
      }
      unsigned char c1 = (unsigned char)ptr[i + 1];
      if ((c1 >> 6) != 0x2) {
        free(wbuf);
        return -1;
      }
      codepoint = ((c & 0x1F) << 6) | (c1 & 0x3F);
      i += 2;
    } else if ((c >> 4) == 0xE) {
      if (i + 2 >= len) {
        free(wbuf);
        return -1;
      }
      unsigned char c1 = (unsigned char)ptr[i + 1];
      unsigned char c2 = (unsigned char)ptr[i + 2];
      if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2) {
        free(wbuf);
        return -1;
      }
      codepoint = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
      i += 3;
    } else if ((c >> 3) == 0x1E) {
      if (i + 3 >= len) {
        free(wbuf);
        return -1;
      }
      unsigned char c1 = (unsigned char)ptr[i + 1];
      unsigned char c2 = (unsigned char)ptr[i + 2];
      unsigned char c3 = (unsigned char)ptr[i + 3];
      if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2 || (c3 >> 6) != 0x2) {
        free(wbuf);
        return -1;
      }
      codepoint = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) |
                  ((c2 & 0x3F) << 6) | (c3 & 0x3F);
      i += 4;
    } else {
      free(wbuf);
      return -1;
    }

    if (sizeof(wchar_t) == 2) {
      if (codepoint <= 0xFFFF) {
        wbuf[wi++] = (wchar_t)codepoint;
      } else if (codepoint <= 0x10FFFF) {
        // surrogate pair
        codepoint -= 0x10000;
        wchar_t high = (wchar_t)((codepoint >> 10) + 0xD800);
        wchar_t low = (wchar_t)((codepoint & 0x3FF) + 0xDC00);
        wbuf[wi++] = high;
        wbuf[wi++] = low;
      } else {
        free(wbuf);
        return -1;
      }
    } else {
      // wchar_t >= 4
      wbuf[wi++] = (wchar_t)codepoint;
    }
  }

  wbuf[wi] = (wchar_t)0;
  *out = wbuf;
  return 0;
}

/*
  readSourceFile_no_windows:
  - path: 文件路径，使用 narrow C 字符串打开 (fopen, "rb")。
    若需要支持 UTF-16 路径（中文路径）请改用宽路径 _wfopen 并传入 wchar_t*。
  - src: 输出的 wchar_t* (null-terminated)，由函数分配，调用者负责 free。
  返回 0 成功，非 0 失败。
*/
int readSourceFile(const char* path, wchar_t** src) {
  if (!path || !src) return -1;
  *src = NULL;

  FILE* fp = fopen(path, "rb");
  if (!fp) return -1;

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return -1;
  }
  long lsize = ftell(fp);
  if (lsize < 0) {
    fclose(fp);
    return -1;
  }
  size_t size = (size_t)lsize;
  rewind(fp);

  char* buf = (char*)malloc(size + 1);
  if (!buf) {
    fclose(fp);
    return -1;
  }

  size_t r = fread(buf, 1, size, fp);
  fclose(fp);
  if (r != size) {
    free(buf);
    return -1;
  }
  buf[size] = '\0';

  // Detect BOMs
  size_t offset = 0;
  if (size >= 3 && (unsigned char)buf[0] == 0xEF &&
      (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF) {
    // UTF-8 BOM
    offset = 3;
    int rc = utf8_to_wcs(buf + offset, size - offset, src);
    free(buf);
    return rc;
  } else if (size >= 2 && (unsigned char)buf[0] == 0xFF &&
             (unsigned char)buf[1] == 0xFE) {
    // UTF-16 LE BOM: copy directly to wchar_t buffer (注意字节对齐)
    size_t bytes = size - 2;
    if (bytes % 2 != 0) {
      free(buf);
      return -1;
    }
    size_t wcnt = bytes / 2;
    wchar_t* out = (wchar_t*)malloc((wcnt + 1) * sizeof(wchar_t));
    if (!out) {
      free(buf);
      return -1;
    }
    // 在小端平台 (Windows) 直接 memcpy 即可
    memcpy(out, buf + 2, bytes);
    out[wcnt] = L'\0';
    free(buf);
    *src = out;
    return 0;
  } else if (size >= 2 && (unsigned char)buf[0] == 0xFE &&
             (unsigned char)buf[1] == 0xFF) {
    // UTF-16 BE BOM: 需要字节交换后复制
    size_t bytes = size - 2;
    if (bytes % 2 != 0) {
      free(buf);
      return -1;
    }
    size_t wcnt = bytes / 2;
    wchar_t* out = (wchar_t*)malloc((wcnt + 1) * sizeof(wchar_t));
    if (!out) {
      free(buf);
      return -1;
    }
    // 逐字交换字节对
    for (size_t i = 0; i < wcnt; ++i) {
      unsigned char hi = (unsigned char)buf[2 + i * 2];
      unsigned char lo = (unsigned char)buf[2 + i * 2 + 1];
      uint16_t v = (uint16_t)((hi << 8) | lo);
      // convert big-endian 16-bit to host endian (assume little-endian host on
      // Windows)
      uint16_t v_le = (uint16_t)((v >> 8) | (v << 8));
      out[i] = (wchar_t)v_le;
    }
    out[wcnt] = L'\0';
    free(buf);
    *src = out;
    return 0;
  } else {
    // No BOM: assume UTF-8 by default
    int rc = utf8_to_wcs(buf, size, src);
    free(buf);
    return rc;
  }
}
#else
int readSourceFile(char* path, wchar_t** src) {
  if (!path || !src) return -1;
  FILE* fp = fopen(path, "r");
  if (fp == NULL) return -1;
  unsigned int index = 0;
  unsigned int size = 16;
  *src = (wchar_t*)malloc(size * sizeof(wchar_t));
  if (!*src) {
    fclose(fp);
    return -1;
  }
  wchar_t ch;
  while ((ch = fgetwc(fp)) != WEOF) {
    if (index + 1 >= size) {  // +1 留出 '\0'
      size *= 2;
      wchar_t* temp = (wchar_t*)realloc(*src, size * sizeof(wchar_t));
      if (!temp) {
        free(*src);
        fclose(fp);
        return -1;
      }
      *src = temp;
    }
    (*src)[index++] = ch;
  }

  (*src)[index] = L'\0';
  fclose(fp);
  return 0;
}
#endif
#endif