/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024, halal-beef <halalbeef.stuff@gmail.com>
 */

unsigned long strnlen (const char *str, unsigned long n)
{
  const char *start = str;

  while (n-- > 0 && *str)
    str++;

  return str - start;
}
